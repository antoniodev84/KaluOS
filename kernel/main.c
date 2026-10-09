#include <lib/multiboot.h>
#include <vga.h>
#include <mm/pmm.h>

__attribute__((section(".multiboot")))
struct multiboot_header_t mboot_header = {
    .magic = MULTIBOOT_MAGIC,
    .flags = MULTIBOOT_FLAGS,
    .checksum = -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)
};
#include "arch/i386/idt.h"
#include "arch/i386/isr.h"

static volatile unsigned int ticks = 0;

void timer_handler(interrupt_frame_t *frame) {
    (void)frame;
    ticks++;
    vga_write_color("Tick!\n", VGA_LIGHT_CYAN, VGA_BLACK);
}


void _main(struct multiboot_info_t *mboot_info, uint32_t mboot_magic) {
	vga_init();
	vga_write_color("KaluOS v0.01\n", VGA_LIGHT_GREEN, VGA_BLACK);
	if (!pmm_init(mboot_info, mboot_magic)) {
		vga_write_color("PMM: invalid Multiboot 1 memory map\n", VGA_LIGHT_RED, VGA_BLACK);
		for (;;) {
			__asm__ volatile ("cli; hlt");
		}
	}
	vga_write("PMM: bitmap ready, 4 KiB frames\n");
	vga_write("Reserved: first MiB, kernel and Multiboot data\n");
	vga_write("Free frames: ");
	vga_write_dec(pmm_free_frames());
	vga_putchar('\n');
	idt_init();
	isr_set_irq_handler(0, timer_handler);
	pic_unmask(0);
	idt_enable();
	for (;;) {
		__asm__ volatile ("hlt");
	}

}
