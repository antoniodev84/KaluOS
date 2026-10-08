#ifndef IDT_H
#define IDT_H

typedef unsigned char idt_u8;
typedef unsigned short idt_u16;
typedef unsigned int idt_u32;

#define IDT_ENTRIES 256
#define IDT_GATE_INTERRUPT 0x8E

struct idt_entry {
    idt_u16 offset_low;
    idt_u16 selector;
    idt_u8 zero;
    idt_u8 flags;
    idt_u16 offset_high;
} __attribute__((packed));

struct idt_ptr {
    idt_u16 limit;
    idt_u32 base;
} __attribute__((packed));

void idt_init(void);
void idt_set_gate(idt_u8 vector, idt_u32 handler, idt_u8 flags);
void idt_enable(void);
void idt_disable(void);
void pic_remap(void);
void pic_send_eoi(idt_u8 irq);
void pic_mask(idt_u8 irq);
void pic_unmask(idt_u8 irq);

#endif
