#include "vga.h"

static volatile u16 *const vga_buffer = (volatile u16 *)0xB8000;

static u16 cursor_x = 0;
static u16 cursor_y = 0;
static u8 current_color = 0x0F;

static inline void outb(u16 port, u8 value) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline u8 inb(u16 port) {
    u8 value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static u8 make_color(u8 fg, u8 bg) {
    return (fg & 0x0F) | ((bg & 0x0F) << 4);
}

static u16 make_entry(char c, u8 color) {
    return (u16)(u8)c | ((u16)color << 8);
}

static void update_cursor(void) {
    u16 pos = cursor_y * VGA_WIDTH + cursor_x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}

void vga_set_cursor(u16 x, u16 y) {
    if (x >= VGA_WIDTH)
        x = VGA_WIDTH - 1;

    if (y >= VGA_HEIGHT)
        y = VGA_HEIGHT - 1;

    cursor_x = x;
    cursor_y = y;

    update_cursor();
}

void vga_enable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 13);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);

    update_cursor();
}

void vga_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_set_color(u8 fg, u8 bg) {
    current_color = make_color(fg, bg);
}

void vga_put_at(char c, u16 x, u16 y, u8 fg, u8 bg) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT)
        return;

    u16 index = y * VGA_WIDTH + x;

    vga_buffer[index] = make_entry(c, make_color(fg, bg));
}

void vga_clear(void) {
    for (u16 y = 0; y < VGA_HEIGHT; y++) {
        for (u16 x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] =
                make_entry(' ', current_color);
        }
    }

    cursor_x = 0;
    cursor_y = 0;

    update_cursor();
}

void vga_scroll(void) {
    for (u16 y = 1; y < VGA_HEIGHT; y++) {
        for (u16 x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y - 1) * VGA_WIDTH + x] =
                vga_buffer[y * VGA_WIDTH + x];
        }
    }

    for (u16 x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            make_entry(' ', current_color);
    }

    cursor_y = VGA_HEIGHT - 1;

    update_cursor();
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    }
    else if (c == '\r') {
        cursor_x = 0;
    }
    else if (c == '\t') {
        u16 spaces = 4 - (cursor_x % 4);

        while (spaces--)
            vga_putchar(' ');

        return;
    }
    else if (c == '\b') {
        vga_backspace();
        return;
    }
    else if ((u8)c >= 32) {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] =
            make_entry(c, current_color);

        cursor_x++;

        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (cursor_y >= VGA_HEIGHT)
        vga_scroll();

    update_cursor();
}

void vga_write(const char *str) {
    if (!str)
        return;

    while (*str)
        vga_putchar(*str++);
}

void vga_write_color(const char *str, u8 fg, u8 bg) {
    u8 old_color = current_color;

    current_color = make_color(fg, bg);

    vga_write(str);

    current_color = old_color;
}

void vga_backspace(void) {
    if (cursor_x == 0 && cursor_y == 0)
        return;

    if (cursor_x > 0) {
        cursor_x--;
    } else {
        cursor_y--;
        cursor_x = VGA_WIDTH - 1;
    }

    vga_buffer[cursor_y * VGA_WIDTH + cursor_x] =
        make_entry(' ', current_color);

    update_cursor();
}

void vga_write_hex(u32 value) {
    const char *digits = "0123456789ABCDEF";

    vga_write("0x");

    for (int i = 7; i >= 0; i--) {
        u8 digit = (value >> (i * 4)) & 0x0F;

        vga_putchar(digits[digit]);
    }
}

void vga_write_dec(u32 value) {
    char buffer[11];
    int i = 0;

    if (value == 0) {
        vga_putchar('0');
        return;
    }

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0)
        vga_putchar(buffer[--i]);
}

void vga_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    current_color = make_color(VGA_WHITE, VGA_BLACK);

    vga_clear();
    vga_enable_cursor();
}
