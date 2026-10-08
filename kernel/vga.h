#ifndef VGA_H
#define VGA_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

enum VGAColor {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_PINK = 13,
    VGA_YELLOW = 14,
    VGA_WHITE = 15
};

void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_write(const char *str);
void vga_write_color(const char *str, u8 fg, u8 bg);
void vga_set_color(u8 fg, u8 bg);
void vga_set_cursor(u16 x, u16 y);
void vga_enable_cursor(void);
void vga_disable_cursor(void);
void vga_scroll(void);
void vga_put_at(char c, u16 x, u16 y, u8 fg, u8 bg);
void vga_backspace(void);
void vga_write_hex(u32 value);
void vga_write_dec(u32 value);

#endif
