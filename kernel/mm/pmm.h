#ifndef KALU_PMM_H
#define KALU_PMM_H

#include <stdint.h>
#include <lib/multiboot.h>

#define PMM_FRAME_SIZE 4096u

int pmm_init(const struct multiboot_info_t *info, uint32_t magic);
uint32_t pmm_alloc_frame(void);
int pmm_free_frame(uint32_t address);
uint32_t pmm_free_frames(void);

#endif
