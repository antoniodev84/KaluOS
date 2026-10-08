#include "idt.h"

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtr;

extern void idt_flush(struct idt_ptr *);
extern void *isr_stub_table[48];

static inline void outb(idt_u16 port, idt_u8 value) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline idt_u8 inb(idt_u16 port) {
    idt_u8 value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

void idt_enable(void) {
    __asm__ volatile ("sti" ::: "memory");
}

void idt_disable(void) {
    __asm__ volatile ("cli" ::: "memory");
}

void idt_set_gate(idt_u8 vector, idt_u32 handler, idt_u8 flags) {
    idt[vector].offset_low = handler & 0xFFFF;
    idt[vector].selector = 0x10;
    idt[vector].zero = 0;
    idt[vector].flags = flags;
    idt[vector].offset_high = (handler >> 16) & 0xFFFF;
}

void pic_remap(void) {
    idt_u8 master_mask = inb(0x21);
    idt_u8 slave_mask = inb(0xA1);

    outb(0x20, 0x11);
    io_wait();

    outb(0xA0, 0x11);
    io_wait();

    outb(0x21, 0x20);
    io_wait();

    outb(0xA1, 0x28);
    io_wait();

    outb(0x21, 0x04);
    io_wait();

    outb(0xA1, 0x02);
    io_wait();

    outb(0x21, 0x01);
    io_wait();

    outb(0xA1, 0x01);
    io_wait();

    outb(0x21, master_mask);
    outb(0xA1, slave_mask);
}

void pic_send_eoi(idt_u8 irq) {
    if (irq >= 16)
        return;

    if (irq >= 8)
        outb(0xA0, 0x20);

    outb(0x20, 0x20);
}

void pic_mask(idt_u8 irq) {
    if (irq >= 16)
        return;

    idt_u16 port = irq < 8 ? 0x21 : 0xA1;
    idt_u8 bit = irq < 8 ? irq : irq - 8;
    idt_u8 value = inb(port);

    outb(port, value | (1u << bit));
}

void pic_unmask(idt_u8 irq) {
    if (irq >= 16)
        return;

    if (irq >= 8)
        pic_unmask(2);

    idt_u16 port = irq < 8 ? 0x21 : 0xA1;
    idt_u8 bit = irq < 8 ? irq : irq - 8;
    idt_u8 value = inb(port);

    outb(port, value & ~(1u << bit));
}

void idt_init(void) {
    idt_disable();

    for (idt_u32 i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].flags = 0;
        idt[i].offset_high = 0;
    }

    for (idt_u32 i = 0; i < 48; i++) {
        idt_set_gate(
            (idt_u8)i,
            (idt_u32)isr_stub_table[i],
            IDT_GATE_INTERRUPT
        );
    }

    idtr.limit = sizeof(idt) - 1;
    idtr.base = (idt_u32)&idt[0];

    pic_remap();

    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    idt_flush(&idtr);
}
