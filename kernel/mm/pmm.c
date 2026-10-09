#include "pmm.h"

#define FRAME_COUNT (1u << 20)
#define BITMAP_WORDS (FRAME_COUNT / 32u)
#define PHYSICAL_LIMIT (1ull << 32)

static uint32_t used[BITMAP_WORDS];
static uint32_t available[BITMAP_WORDS];
static uint32_t free_count;
static uint32_t frame_limit;
static uint32_t next_frame;

extern unsigned char kernel_start[];
extern unsigned char end[];

static uint32_t lock(void) {
    uint32_t flags;
    __asm__ volatile ("pushfl; popl %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static void unlock(uint32_t flags) {
    __asm__ volatile ("pushl %0; popfl" :: "r"(flags) : "memory", "cc");
}

static uint64_t range_end(uint64_t start, uint64_t length) {
    if (start >= PHYSICAL_LIMIT)
        return PHYSICAL_LIMIT;
    if (length > PHYSICAL_LIMIT - start)
        return PHYSICAL_LIMIT;
    return start + length;
}

static void mark_range(uint64_t start, uint64_t finish, int usable) {
    if (start >= PHYSICAL_LIMIT || finish <= start)
        return;
    if (finish > PHYSICAL_LIMIT)
        finish = PHYSICAL_LIMIT;
    uint32_t first = usable ? (uint32_t)((start + 4095u) >> 12)
                            : (uint32_t)(start >> 12);
    uint32_t last = usable ? (uint32_t)(finish >> 12)
                           : (uint32_t)((finish + 4095u) >> 12);
    if (usable && last > frame_limit)
        frame_limit = last;
    for (uint32_t frame = first; frame < last; frame++) {
        uint32_t mask = 1u << (frame & 31u);
        uint32_t word = frame >> 5;
        if (usable) {
            if (!(available[word] & mask)) {
                available[word] |= mask;
                used[word] &= ~mask;
                free_count++;
            }
        } else if (available[word] & mask) {
            available[word] &= ~mask;
            used[word] |= mask;
            free_count--;
        }
    }
}

static int valid_map(const struct multiboot_info_t *info) {
    uint64_t cursor = info->mmap_addr;
    uint64_t finish = cursor + info->mmap_length;
    if (!cursor || !info->mmap_length || finish > PHYSICAL_LIMIT)
        return 0;
    while (cursor < finish) {
        if (finish - cursor < sizeof(uint32_t))
            return 0;
        const struct multiboot_memory_map_t *entry =
            (const void *)(uintptr_t)cursor;
        uint64_t step = (uint64_t)entry->size + sizeof(entry->size);
        if (entry->size < 20u || step > finish - cursor)
            return 0;
        cursor += step;
    }
    return 1;
}

static void apply_map(const struct multiboot_info_t *info, int usable) {
    uint64_t cursor = info->mmap_addr;
    uint64_t finish = cursor + info->mmap_length;
    while (cursor < finish) {
        const struct multiboot_memory_map_t *entry =
            (const void *)(uintptr_t)cursor;
        uint64_t start = ((uint64_t)entry->addr_high << 32) | entry->addr_low;
        uint64_t length = ((uint64_t)entry->len_high << 32) | entry->len_low;
        if ((entry->type == MULTIBOOT_MEMORY_AVAILABLE) == usable)
            mark_range(start, range_end(start, length), usable);
        cursor += (uint64_t)entry->size + sizeof(entry->size);
    }
}

static void reserve_string(uint32_t address) {
    if (!address)
        return;
    const char *string = (const void *)(uintptr_t)address;
    uint64_t finish = address;
    while (finish < PHYSICAL_LIMIT) {
        finish++;
        if (!*string++)
            break;
    }
    mark_range(address, finish, 0);
}

int pmm_init(const struct multiboot_info_t *info, uint32_t magic) {
    uint32_t flags = lock();
    free_count = 0;
    frame_limit = 0;
    next_frame = 0;
    for (uint32_t i = 0; i < BITMAP_WORDS; i++) {
        used[i] = ~0u;
        available[i] = 0;
    }
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || !info ||
        !(info->flags & MULTIBOOT_INFO_MEMORY_MAP) || !valid_map(info)) {
        unlock(flags);
        return 0;
    }
    if (info->flags & MULTIBOOT_INFO_MODULES) {
        uint64_t finish = (uint64_t)info->mods_addr +
            (uint64_t)info->mods_count * sizeof(struct multiboot_module_t);
        if ((info->mods_count && !info->mods_addr) || finish > PHYSICAL_LIMIT) {
            unlock(flags);
            return 0;
        }
        const struct multiboot_module_t *modules =
            (const void *)(uintptr_t)info->mods_addr;
        for (uint32_t i = 0; i < info->mods_count; i++) {
            if (modules[i].end < modules[i].start) {
                unlock(flags);
                return 0;
            }
        }
    }
    apply_map(info, 1);
    apply_map(info, 0);
    mark_range(0, 0x100000u, 0);
    mark_range((uintptr_t)kernel_start, (uintptr_t)end, 0);
    mark_range((uintptr_t)info, (uint64_t)(uintptr_t)info + sizeof(*info), 0);
    mark_range(info->mmap_addr, (uint64_t)info->mmap_addr + info->mmap_length, 0);
    if (info->flags & (1u << 2))
        reserve_string(info->cmdline);
    if (info->flags & (1u << 9))
        reserve_string(info->boot_loader_name);
    if (info->flags & (1u << 5))
        mark_range(info->elf_addr, range_end(info->elf_addr,
            (uint64_t)info->elf_num * info->elf_size), 0);
    if (info->flags & MULTIBOOT_INFO_MODULES) {
        const struct multiboot_module_t *modules =
            (const void *)(uintptr_t)info->mods_addr;
        mark_range(info->mods_addr, (uint64_t)info->mods_addr +
            (uint64_t)info->mods_count * sizeof(*modules), 0);
        for (uint32_t i = 0; i < info->mods_count; i++) {
            mark_range(modules[i].start, modules[i].end, 0);
            reserve_string(modules[i].string);
        }
    }
    unlock(flags);
    return 1;
}

uint32_t pmm_alloc_frame(void) {
    uint32_t flags = lock();
    if (!free_count) {
        unlock(flags);
        return 0;
    }
    for (uint32_t checked = 0; checked < frame_limit; checked++) {
        uint32_t frame = next_frame++;
        if (next_frame == frame_limit)
            next_frame = 0;
        uint32_t mask = 1u << (frame & 31u);
        if (!(used[frame >> 5] & mask)) {
            used[frame >> 5] |= mask;
            free_count--;
            unlock(flags);
            return frame << 12;
        }
    }
    unlock(flags);
    return 0;
}

int pmm_free_frame(uint32_t address) {
    uint32_t flags = lock();
    uint32_t frame = address >> 12;
    uint32_t mask = 1u << (frame & 31u);
    uint32_t word = frame >> 5;
    if ((address & (PMM_FRAME_SIZE - 1u)) || !(available[word] & mask) ||
        !(used[word] & mask)) {
        unlock(flags);
        return 0;
    }
    used[word] &= ~mask;
    free_count++;
    if (frame < next_frame)
        next_frame = frame;
    unlock(flags);
    return 1;
}

uint32_t pmm_free_frames(void) {
    uint32_t flags = lock();
    uint32_t count = free_count;
    unlock(flags);
    return count;
}
