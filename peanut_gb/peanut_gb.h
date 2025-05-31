/**
 * MIT License
 *
 * Copyright (c) 2018-2022 Mahyar Koshkouei
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Please note that at least three parts of source code within this project was
 * taken from the SameBoy project at https://github.com/LIJI32/SameBoy/ which at
 * the time of this writing is released under the MIT License. Occurrences of
 * this code is marked as being taken from SameBoy with a comment.
 * SameBoy, and code marked as being taken from SameBoy,
 * is Copyright (c) 2015-2019 Lior Halphon.
 */

#ifndef PEANUT_GB_H
#define PEANUT_GB_H

#ifdef TARGET_SIMULATOR
#define CPU_VALIDATE 1
#endif

#include <stdint.h> /* Required for int types */
#include <stdlib.h> /* Required for qsort */
#include <string.h> /* Required for memset */
#include <time.h>   /* Required for tm struct */

#include "../src/app.h"
#include "version.all" /* Version information */

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;

#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

/**
 * Sound support must be provided by an external library. When audio_read() and
 * audio_write() functions are provided, define ENABLE_SOUND to a non-zero value
 * before including peanut_gb.h in order for these functions to be used.
 */
#ifndef ENABLE_SOUND
#define ENABLE_SOUND 1
#endif

/* Enable LCD drawing. On by default. May be turned off for testing purposes. */
#ifndef ENABLE_LCD
#define ENABLE_LCD 1
#endif

/* Adds more code to improve LCD rendering accuracy. */
#ifndef PEANUT_GB_HIGH_LCD_ACCURACY
#define PEANUT_GB_HIGH_LCD_ACCURACY 0
#endif

/* Interrupt masks */
#define VBLANK_INTR 0x01
#define LCDC_INTR 0x02
#define TIMER_INTR 0x04
#define SERIAL_INTR 0x08
#define CONTROL_INTR 0x10
#define ANY_INTR 0x1F

/* Memory section sizes for DMG */
#define WRAM_SIZE 0x2000
#define VRAM_SIZE 0x2000
#define HRAM_SIZE 0x0100
#define OAM_SIZE 0x00A0

/* Memory addresses */
#define ROM_0_ADDR 0x0000
#define ROM_N_ADDR 0x4000
#define VRAM_ADDR 0x8000
#define CART_RAM_ADDR 0xA000
#define WRAM_0_ADDR 0xC000
#define WRAM_1_ADDR 0xD000
#define ECHO_ADDR 0xE000
#define OAM_ADDR 0xFE00
#define UNUSED_ADDR 0xFEA0
#define IO_ADDR 0xFF00
#define HRAM_ADDR 0xFF80
#define INTR_EN_ADDR 0xFFFF

/* Cart section sizes */
#define ROM_BANK_SIZE 0x4000
#define WRAM_BANK_SIZE 0x1000
#define CRAM_BANK_SIZE 0x2000
#define VRAM_BANK_SIZE 0x2000

/* DIV Register is incremented at rate of 16384Hz.
 * 4194304 / 16384 = 256 clock cycles for one increment. */
#define DIV_CYCLES 256

/* Serial clock locked to 8192Hz on DMG.
 * 4194304 / (8192 / 8) = 4096 clock cycles for sending 1 byte. */
#define SERIAL_CYCLES 4096

/* Calculating VSYNC. */
#define DMG_CLOCK_FREQ 4194304.0
#define SCREEN_REFRESH_CYCLES 70224.0
#define VERTICAL_SYNC (DMG_CLOCK_FREQ / SCREEN_REFRESH_CYCLES)

/* SERIAL SC register masks. */
#define SERIAL_SC_TX_START 0x80
#define SERIAL_SC_CLOCK_SRC 0x01

/* STAT register masks */
#define STAT_LYC_INTR 0x40
#define STAT_MODE_2_INTR 0x20
#define STAT_MODE_1_INTR 0x10
#define STAT_MODE_0_INTR 0x08
#define STAT_LYC_COINC 0x04
#define STAT_MODE 0x03
#define STAT_USER_BITS 0xF8

/* LCDC control masks */
#define LCDC_ENABLE 0x80
#define LCDC_WINDOW_MAP 0x40
#define LCDC_WINDOW_ENABLE 0x20
#define LCDC_TILE_SELECT 0x10
#define LCDC_BG_MAP 0x08
#define LCDC_OBJ_SIZE 0x04
#define LCDC_OBJ_ENABLE 0x02
#define LCDC_BG_ENABLE 0x01

/* LCD characteristics */
#define LCD_LINE_CYCLES 456
#define LCD_MODE_0_CYCLES 0
#define LCD_MODE_2_CYCLES 204
#define LCD_MODE_3_CYCLES 284
#define LCD_VERT_LINES 154
#define LCD_WIDTH 160
#define LCD_PACKING 4 /* pixels per byte */
#define LCD_BITS_PER_PIXEL (8 / LCD_PACKING)
#define LCD_WIDTH_PACKED (LCD_WIDTH / LCD_PACKING)
#define LCD_HEIGHT 144

/* VRAM Locations */
#define VRAM_TILES_1 (0x8000 - VRAM_ADDR)
#define VRAM_TILES_2 (0x8800 - VRAM_ADDR)
#define VRAM_BMAP_1 (0x9800 - VRAM_ADDR)
#define VRAM_BMAP_2 (0x9C00 - VRAM_ADDR)
#define VRAM_TILES_3 (0x8000 - VRAM_ADDR + VRAM_BANK_SIZE)
#define VRAM_TILES_4 (0x8800 - VRAM_ADDR + VRAM_BANK_SIZE)

/* Interrupt jump addresses */
#define VBLANK_INTR_ADDR 0x0040
#define LCDC_INTR_ADDR 0x0048
#define TIMER_INTR_ADDR 0x0050
#define SERIAL_INTR_ADDR 0x0058
#define CONTROL_INTR_ADDR 0x0060

/* SPRITE controls */
#define NUM_SPRITES 0x28
#define MAX_SPRITES_LINE 0x0A
#define OBJ_PRIORITY 0x80
#define OBJ_FLIP_Y 0x40
#define OBJ_FLIP_X 0x20
#define OBJ_PALETTE 0x10

#define ROM_HEADER_CHECKSUM_LOC 0x014D

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define PEANUT_GB_ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))

struct cpu_registers_s
{
    union
    {
        struct
        {
            uint8_t c;
            uint8_t b;
        };
        uint16_t bc;
    };

    union
    {
        struct
        {
            uint8_t e;
            uint8_t d;
        };
        uint16_t de;
    };

    union
    {
        struct
        {
            uint8_t l;
            uint8_t h;
        };
        uint16_t hl;
    };

    /* Combine A and F registers. */
    union
    {
        struct
        {
            uint8_t a;
            /* Define specific bits of Flag register. */
            union
            {
                struct
                {
                    uint8_t unused : 4;
                    uint8_t c : 1; /* Carry flag. */
                    uint8_t h : 1; /* Half carry flag. */
                    uint8_t n : 1; /* Add/sub flag. */
                    uint8_t z : 1; /* Zero flag. */
                } f_bits;
                uint8_t f;
            };
        };
        uint16_t af;
    };

    uint16_t sp; /* Stack pointer */
    uint16_t pc; /* Program counter */
};

struct count_s
{
    uint_fast16_t lcd_count;    /* LCD Timing */
    uint_fast16_t div_count;    /* Divider Register Counter */
    uint_fast16_t tima_count;   /* Timer Counter */
    uint_fast16_t serial_count; /* Serial Counter */
};

struct gb_registers_s
{
    /* TODO: Sort variables in address order. */
    uint16_t tac_cycles;
    uint8_t tac_cycles_shift;

    /* Timing */
    uint8_t TIMA, TMA, DIV;
    union
    {
        struct
        {
            uint8_t tac_rate : 2;   /* Input clock select */
            uint8_t tac_enable : 1; /* Timer enable */
            uint8_t unused : 5;
        };
        uint8_t TAC;
    };

    /* LCD */
    uint8_t LCDC;
    uint8_t STAT;
    uint8_t SCY;
    uint8_t SCX;
    uint8_t LY;
    uint8_t LYC;
    uint8_t DMA;
    uint8_t BGP;
    uint8_t OBP0;
    uint8_t OBP1;
    uint8_t WY;
    uint8_t WX;

    /* Joypad info. */
    uint8_t P1;

    /* Serial data. */
    uint8_t SB;
    uint8_t SC;

    /* Interrupt flag. */
    uint8_t IF;

    /* Interrupt enable. */
    uint8_t IE;
};

#if ENABLE_LCD
/* Bit mask for the shade of pixel to display */
#define LCD_COLOUR 0x03
/**
 * Bit mask for whether a pixel is OBJ0, OBJ1, or BG. Each may have a different
 * palette when playing a DMG game on CGB.
 */
#define LCD_PALETTE_OBJ 0x4
#define LCD_PALETTE_BG 0x8
/**
 * Bit mask for the two bits listed above.
 * LCD_PALETTE_ALL == 0b00 --> OBJ0
 * LCD_PALETTE_ALL == 0b01 --> OBJ1
 * LCD_PALETTE_ALL == 0b10 --> BG
 * LCD_PALETTE_ALL == 0b11 --> NOT POSSIBLE
 */
#define LCD_PALETTE_ALL 0x30
#endif

/**
 * Errors that may occur during emulation.
 */
enum gb_error_e
{
    GB_UNKNOWN_ERROR,
    GB_INVALID_OPCODE,
    GB_INVALID_READ,
    GB_INVALID_WRITE,

    GB_INVALID_MAX
};

/**
 * Errors that may occur during library initialisation.
 */
enum gb_init_error_e
{
    GB_INIT_NO_ERROR,
    GB_INIT_CARTRIDGE_UNSUPPORTED,
    GB_INIT_INVALID_CHECKSUM
};

/**
 * Return codes for serial receive function, mainly for clarity.
 */
enum gb_serial_rx_ret_e
{
    GB_SERIAL_RX_SUCCESS = 0,
    GB_SERIAL_RX_NO_CONNECTION = 1
};

/**
 * Emulator context.
 *
 * Only values within the `direct` struct may be modified directly by the
 * front-end implementation. Other variables must not be modified.
 */
struct gb_s
{
    uint8_t *gb_rom;
    uint8_t *gb_cart_ram;

    /**
     * Notify front-end of error.
     *
     * \param gb_s          emulator context
     * \param gb_error_e    error code
     * \param val           arbitrary value related to error
     */
    void (*gb_error)(struct gb_s *, const enum gb_error_e, const uint16_t val);

    /* Transmit one byte and return the received byte. */
    void (*gb_serial_tx)(struct gb_s *, const uint8_t tx);
    enum gb_serial_rx_ret_e (*gb_serial_rx)(struct gb_s *, uint8_t *rx);

    // shortcut to swappable bank (addr - 0x4000 offset built in)
    uint8_t *selected_bank_addr;

    struct
    {
        uint8_t gb_halt : 1;
        uint8_t gb_ime : 1;
        uint8_t gb_bios_enable : 1;
        uint8_t gb_frame : 1; /* New frame drawn. */

#define LCD_HBLANK 0
#define LCD_VBLANK 1
#define LCD_SEARCH_OAM 2
#define LCD_TRANSFER 3
        uint8_t lcd_mode : 2;
        uint8_t lcd_blank : 1;
        uint8_t lcd_master_enable : 1;
    };

    /* Cartridge information:
     * Memory Bank Controller (MBC) type. */
    uint8_t mbc;
    /* Whether the MBC has internal RAM. */
    uint8_t cart_ram;
    /* Number of ROM banks in cartridge. */
    uint16_t num_rom_banks_mask;
    /* Number of RAM banks in cartridge. */
    uint8_t num_ram_banks;

    uint16_t selected_rom_bank;
    /* WRAM and VRAM bank selection not available. */
    uint8_t cart_ram_bank;
    uint8_t enable_cart_ram;
    /* Cartridge ROM/RAM mode select. */
    uint8_t cart_mode_select;
    union
    {
        struct
        {
            uint8_t sec;
            uint8_t min;
            uint8_t hour;
            uint8_t yday;
            uint8_t high;
        } rtc_bits;
        uint8_t cart_rtc[5];
    };

    union
    {
        struct cpu_registers_s cpu_reg;
        uint8_t cpu_reg_raw[12];
        uint16_t cpu_reg_raw16[6];
    };
    struct gb_registers_s gb_reg;
    struct count_s counter;

    /* TODO: Allow implementation to allocate WRAM, VRAM and Frame Buffer. */
    uint8_t *wram;  // wram[WRAM_SIZE];
    uint8_t *vram;  // vram[VRAM_SIZE];
    uint8_t hram[HRAM_SIZE];
    uint8_t oam[OAM_SIZE];
    uint8_t *lcd;

    struct
    {
        /**
         * Draw line on screen.
         *
         * \param gb_s      emulator context
         * \param pixels    The 160 pixels to draw.
         *                  Bits 1-0 are the colour to draw.
         *                  Bits 5-4 are the palette, where:
         *                      OBJ0 = 0b00,
         *                      OBJ1 = 0b01,
         *                      BG = 0b10
         *                  Other bits are undefined.
         *                  Bits 5-4 are only required by front-ends
         *                  which want to use a different colour for
         *                  different object palettes. This is what
         *                  the Game Boy Color (CGB) does to DMG
         *                  games.
         * \param line      Line to draw pixels on. This is
         *                  guaranteed to be between 0-144 inclusive.
         */

        /* Palettes */
        uint8_t bg_palette[4];
        uint8_t sp_palette[8];

        uint8_t window_clear;
        uint8_t WY;

        /* Only support 30fps frame skip. */
        uint8_t frame_skip_count : 1;

        /* Playdate custom implementation */
        uint8_t back_fb_enabled : 1;

        uint32_t line_priority[(LCD_WIDTH + 31) / 32];
    } display;

    /**
     * Variables that may be modified directly by the front-end.
     * This method seems to be easier and possibly less overhead than
     * calling a function to modify these variables each time.
     *
     * None of this is thread-safe.
     */
    struct
    {
        /* Set to enable interlacing. Interlacing will start immediately
         * (at the next line drawing).
         */
        uint8_t frame_skip : 1;
        uint8_t sound : 1;

        union
        {
            struct
            {
                uint8_t a : 1;
                uint8_t b : 1;
                uint8_t select : 1;
                uint8_t start : 1;
                uint8_t right : 1;
                uint8_t left : 1;
                uint8_t up : 1;
                uint8_t down : 1;
            } joypad_bits;
            uint8_t joypad;
        };

        /* Implementation defined data. Set to NULL if not required. */
        void *priv;
    } direct;
};

/**
 * Tick the internal RTC by one second.
 * This was taken from SameBoy, which is released under MIT Licence.
 */
__section__(".text.pgb")
void gb_tick_rtc(struct gb_s *gb)
{
    /* is timer running? */
    if ((gb->cart_rtc[4] & 0x40) == 0)
    {
        if (++gb->rtc_bits.sec == 60)
        {
            gb->rtc_bits.sec = 0;

            if (++gb->rtc_bits.min == 60)
            {
                gb->rtc_bits.min = 0;

                if (++gb->rtc_bits.hour == 24)
                {
                    gb->rtc_bits.hour = 0;

                    if (++gb->rtc_bits.yday == 0)
                    {
                        if (gb->rtc_bits.high & 1) /* Bit 8 of days*/
                        {
                            gb->rtc_bits.high |= 0x80; /* Overflow bit */
                        }

                        gb->rtc_bits.high ^= 1;
                    }
                }
            }
        }
    }
}

/**
 * Set initial values in RTC.
 * Should be called after gb_init().
 */
__section__(".text.pgb")
void gb_set_rtc(struct gb_s *gb, const struct tm *const time)
{
    gb->cart_rtc[0] = time->tm_sec;
    gb->cart_rtc[1] = time->tm_min;
    gb->cart_rtc[2] = time->tm_hour;
    gb->cart_rtc[3] = time->tm_yday & 0xFF; /* Low 8 bits of day counter. */
    gb->cart_rtc[4] = time->tm_yday >> 8;   /* High 1 bit of day counter. */
}

__section__(".text.pgb")
static void __gb_update_tac(struct gb_s *gb)
{
    static const uint8_t TAC_CYCLES[4] = {10, 4, 6, 8};

    // subtract 1 so it can be used as a mask for quick modulo.
    gb->gb_reg.tac_cycles_shift = TAC_CYCLES[gb->gb_reg.tac_rate];
    gb->gb_reg.tac_cycles = (1 << (int)TAC_CYCLES[gb->gb_reg.tac_rate]) - 1;
}

__section__(".text.pgb")
static void __gb_update_selected_bank_addr(struct gb_s *gb)
{
    int32_t offset;
    if (gb->mbc == 1 && gb->cart_mode_select)
        offset = (gb->selected_rom_bank & 0x1F) - 1;
    else
        offset = gb->selected_rom_bank - 1;
    offset *= ROM_BANK_SIZE;

    gb->selected_bank_addr = gb->gb_rom + offset;
}

/**
 * Internal function used to read bytes.
 */
__shell uint8_t __gb_read_full(struct gb_s *gb, const uint_fast16_t addr)
{
    switch (addr >> 12)
    {
    case 0x0:

    /* TODO: BIOS support. */
    case 0x1:
    case 0x2:
    case 0x3:
        return gb->gb_rom[addr];

    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        return gb->selected_bank_addr[addr];

    case 0x8:
    case 0x9:
        return gb->vram[addr - VRAM_ADDR];

    case 0xA:
    case 0xB:
        if (gb->cart_ram && gb->enable_cart_ram)
        {
            if (gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
                return gb->cart_rtc[gb->cart_ram_bank - 0x08];
            else if ((gb->cart_mode_select || gb->mbc != 1) &&
                     gb->cart_ram_bank < gb->num_ram_banks)
            {
                return gb->gb_cart_ram[addr - CART_RAM_ADDR +
                                       (gb->cart_ram_bank * CRAM_BANK_SIZE)];
            }
            else
                return gb->gb_cart_ram[addr - CART_RAM_ADDR];
        }

        return 0xFF;

    case 0xC:
        return gb->wram[addr - WRAM_0_ADDR];

    case 0xD:
        return gb->wram[addr - WRAM_0_ADDR];

    case 0xE:
        return gb->wram[addr - ECHO_ADDR];

    case 0xF:
        if (addr < OAM_ADDR)
            return gb->wram[addr - ECHO_ADDR];

        if (addr < UNUSED_ADDR)
            return gb->oam[addr - OAM_ADDR];

        /* Unusable memory area. Reading from this area returns 0.*/
        if (addr < IO_ADDR)
            return 0xFF;

        /* HRAM */
        if (HRAM_ADDR <= addr && addr < INTR_EN_ADDR)
            return gb->hram[addr - IO_ADDR];

        /* APU registers. */
        if ((addr >= 0xFF10) && (addr <= 0xFF3F))
        {
            if (gb->direct.sound)
            {
                return audio_read(addr);
            }
            else
            { /* clang-format off */
                static const uint8_t ortab[] = {
                    0x80, 0x3f, 0x00, 0xff, 0xbf,
                    0xff, 0x3f, 0x00, 0xff, 0xbf,
                    0x7f, 0xff, 0x9f, 0xff, 0xbf,
                    0xff, 0xff, 0x00, 0x00, 0xbf,
                    0x00, 0x00, 0x70,
                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                };
                /* clang-format on */
                return gb->hram[addr - IO_ADDR] | ortab[addr - IO_ADDR];
            }
        }

        /* IO and Interrupts. */
        switch (addr & 0xFF)
        {
        /* IO Registers */
        case 0x00:
            return 0xC0 | gb->gb_reg.P1;

        case 0x01:
            return gb->gb_reg.SB;

        case 0x02:
            return gb->gb_reg.SC;

        /* Timer Registers */
        case 0x04:
            return gb->gb_reg.DIV;

        case 0x05:
            return gb->gb_reg.TIMA;

        case 0x06:
            return gb->gb_reg.TMA;

        case 0x07:
            return gb->gb_reg.TAC;

        /* Interrupt Flag Register */
        case 0x0F:
            return gb->gb_reg.IF;

        /* LCD Registers */
        case 0x40:
            return gb->gb_reg.LCDC;

        case 0x41:
            return (gb->gb_reg.STAT & STAT_USER_BITS) |
                   (gb->gb_reg.LCDC & LCDC_ENABLE ? gb->lcd_mode : LCD_VBLANK);

        case 0x42:
            return gb->gb_reg.SCY;

        case 0x43:
            return gb->gb_reg.SCX;

        case 0x44:
            return gb->gb_reg.LY;

        case 0x45:
            return gb->gb_reg.LYC;

        /* DMA Register */
        case 0x46:
            return gb->gb_reg.DMA;

        /* DMG Palette Registers */
        case 0x47:
            return gb->gb_reg.BGP;

        case 0x48:
            return gb->gb_reg.OBP0;

        case 0x49:
            return gb->gb_reg.OBP1;

        /* Window Position Registers */
        case 0x4A:
            return gb->gb_reg.WY;

        case 0x4B:
            return gb->gb_reg.WX;

        /* Interrupt Enable Register */
        case 0xFF:
            return gb->gb_reg.IE;

        /* Unused registers return 1 */
        default:
            return 0xFF;
        }
    }

    (gb->gb_error)(gb, GB_INVALID_READ, addr);
    return 0xFF;
}

/**
 * Internal function used to write bytes.
 */
__shell void __gb_write_full(struct gb_s *gb, const uint_fast16_t addr,
                             const uint8_t val)
{
    switch (addr >> 12)
    {
    case 0x0:
    case 0x1:
        if (gb->mbc == 2 && addr & 0x10)
            return;
        else if (gb->mbc > 0 && gb->cart_ram)
            gb->enable_cart_ram = ((val & 0x0F) == 0x0A);

        return;

    case 0x2:
        if (gb->mbc == 5)
        {
            gb->selected_rom_bank = (gb->selected_rom_bank & 0x100) | val;
            gb->selected_rom_bank =
                gb->selected_rom_bank & gb->num_rom_banks_mask;
            __gb_update_selected_bank_addr(gb);
            return;
        }

        /* Intentional fall through. */

    case 0x3:
        if (gb->mbc == 1)
        {
            // selected_rom_bank = val & 0x7;
            gb->selected_rom_bank =
                (val & 0x1F) | (gb->selected_rom_bank & 0x60);

            if ((gb->selected_rom_bank & 0x1F) == 0x00)
                gb->selected_rom_bank++;
        }
        else if (gb->mbc == 2 && addr & 0x10)
        {
            gb->selected_rom_bank = val & 0x0F;

            if (!gb->selected_rom_bank)
                gb->selected_rom_bank++;
        }
        else if (gb->mbc == 3)
        {
            gb->selected_rom_bank = val & 0x7F;

            if (!gb->selected_rom_bank)
                gb->selected_rom_bank++;
        }
        else if (gb->mbc == 5)
            gb->selected_rom_bank =
                (val & 0x01) << 8 | (gb->selected_rom_bank & 0xFF);
        gb->selected_rom_bank = gb->selected_rom_bank & gb->num_rom_banks_mask;
        __gb_update_selected_bank_addr(gb);
        return;

    case 0x4:
    case 0x5:
        if (gb->mbc == 1)
        {
            gb->cart_ram_bank = (val & 3);
            gb->selected_rom_bank =
                ((val & 3) << 5) | (gb->selected_rom_bank & 0x1F);
            gb->selected_rom_bank =
                gb->selected_rom_bank & gb->num_rom_banks_mask;
            __gb_update_selected_bank_addr(gb);
        }
        else if (gb->mbc == 3)
            gb->cart_ram_bank = val;
        else if (gb->mbc == 5)
            gb->cart_ram_bank = (val & 0x0F);

        return;

    case 0x6:
    case 0x7:
        gb->cart_mode_select = (val & 1);
        return;

    case 0x8:
    case 0x9:
        gb->vram[addr - VRAM_ADDR] = val;
        return;

    case 0xA:
    case 0xB:
        if (gb->cart_ram && gb->enable_cart_ram)
        {
            if (gb->mbc == 3 && gb->cart_ram_bank >= 0x08)
                gb->cart_rtc[gb->cart_ram_bank - 0x08] = val;
            else if (gb->cart_mode_select &&
                     gb->cart_ram_bank < gb->num_ram_banks)
            {
                gb->gb_cart_ram[addr - CART_RAM_ADDR +
                                (gb->cart_ram_bank * CRAM_BANK_SIZE)] = val;
            }
            else if (gb->num_ram_banks)
                gb->gb_cart_ram[addr - CART_RAM_ADDR] = val;
        }

        return;

    case 0xC:
        gb->wram[addr - WRAM_0_ADDR] = val;
        return;

    case 0xD:
        gb->wram[addr - WRAM_1_ADDR + WRAM_BANK_SIZE] = val;
        return;

    case 0xE:
        gb->wram[addr - ECHO_ADDR] = val;
        return;

    case 0xF:
        if (addr < OAM_ADDR)
        {
            gb->wram[addr - ECHO_ADDR] = val;
            return;
        }

        if (addr < UNUSED_ADDR)
        {
            gb->oam[addr - OAM_ADDR] = val;
            return;
        }

        /* Unusable memory area. */
        if (addr < IO_ADDR)
            return;

        if (HRAM_ADDR <= addr && addr < INTR_EN_ADDR)
        {
            gb->hram[addr - IO_ADDR] = val;
            return;
        }

        if ((addr >= 0xFF10) && (addr <= 0xFF3F))
        {
            if (gb->direct.sound)
            {
                audio_write(addr, val);
            }
            else
            {
                gb->hram[addr - IO_ADDR] = val;
            }
            return;
        }

        /* IO and Interrupts. */
        switch (addr & 0xFF)
        {
        /* Joypad */
        case 0x00:
            /* Only bits 5 and 4 are R/W.
             * The lower bits are overwritten later, and the two most
             * significant bits are unused. */
            gb->gb_reg.P1 = val;

            /* Direction keys selected */
            if ((gb->gb_reg.P1 & 0b010000) == 0)
                gb->gb_reg.P1 |= (gb->direct.joypad >> 4);
            /* Button keys selected */
            else
                gb->gb_reg.P1 |= (gb->direct.joypad & 0x0F);

            return;

        /* Serial */
        case 0x01:
            gb->gb_reg.SB = val;
            return;

        case 0x02:
            gb->gb_reg.SC = val;
            return;

        /* Timer Registers */
        case 0x04:
            gb->gb_reg.DIV = 0x00;
            return;

        case 0x05:
            gb->gb_reg.TIMA = val;
            return;

        case 0x06:
            gb->gb_reg.TMA = val;
            return;

        case 0x07:
            gb->gb_reg.TAC = val;
            __gb_update_tac(gb);
            return;

        /* Interrupt Flag Register */
        case 0x0F:
            gb->gb_reg.IF = (val | 0b11100000);
            return;

        /* LCD Registers */
        case 0x40:
            if (((gb->gb_reg.LCDC & LCDC_ENABLE) == 0) && (val & LCDC_ENABLE))
            {
                gb->counter.lcd_count = 0;
                gb->lcd_blank = 1;
            }

            gb->gb_reg.LCDC = val;

            /* LY fixed to 0 when LCD turned off. */
            if ((gb->gb_reg.LCDC & LCDC_ENABLE) == 0)
            {
                /* Do not turn off LCD outside of VBLANK. This may
                 * happen due to poor timing in this emulator. */
                if (gb->lcd_mode != LCD_VBLANK)
                {
                    gb->gb_reg.LCDC |= LCDC_ENABLE;
                    return;
                }

                gb->gb_reg.STAT = (gb->gb_reg.STAT & ~0x03) | LCD_VBLANK;
                gb->gb_reg.LY = 0;
                gb->counter.lcd_count = 0;
            }

            return;

        case 0x41:
            gb->gb_reg.STAT = (val & 0b01111000);
            return;

        case 0x42:
            gb->gb_reg.SCY = val;
            return;

        case 0x43:
            gb->gb_reg.SCX = val;
            return;

        /* LY (0xFF44) is read only. */
        case 0x45:
            gb->gb_reg.LYC = val;
            return;

        /* DMA Register */
        case 0x46:
            gb->gb_reg.DMA = (val % 0xF1);

            for (uint8_t i = 0; i < OAM_SIZE; i++)
                gb->oam[i] = __gb_read_full(gb, (gb->gb_reg.DMA << 8) + i);

            return;

        /* DMG Palette Registers */
        case 0x47:
            gb->gb_reg.BGP = val;
            gb->display.bg_palette[0] = (gb->gb_reg.BGP & 0x03);
            gb->display.bg_palette[1] = (gb->gb_reg.BGP >> 2) & 0x03;
            gb->display.bg_palette[2] = (gb->gb_reg.BGP >> 4) & 0x03;
            gb->display.bg_palette[3] = (gb->gb_reg.BGP >> 6) & 0x03;
            return;

        case 0x48:
            gb->gb_reg.OBP0 = val;
            gb->display.sp_palette[0] = (gb->gb_reg.OBP0 & 0x03);
            gb->display.sp_palette[1] = (gb->gb_reg.OBP0 >> 2) & 0x03;
            gb->display.sp_palette[2] = (gb->gb_reg.OBP0 >> 4) & 0x03;
            gb->display.sp_palette[3] = (gb->gb_reg.OBP0 >> 6) & 0x03;
            return;

        case 0x49:
            gb->gb_reg.OBP1 = val;
            gb->display.sp_palette[4] = (gb->gb_reg.OBP1 & 0x03);
            gb->display.sp_palette[5] = (gb->gb_reg.OBP1 >> 2) & 0x03;
            gb->display.sp_palette[6] = (gb->gb_reg.OBP1 >> 4) & 0x03;
            gb->display.sp_palette[7] = (gb->gb_reg.OBP1 >> 6) & 0x03;
            return;

        /* Window Position Registers */
        case 0x4A:
            gb->gb_reg.WY = val;
            return;

        case 0x4B:
            gb->gb_reg.WX = val;
            return;

        /* Turn off boot ROM */
        case 0x50:
            gb->gb_bios_enable = 0;
            return;

        /* Interrupt Enable Register */
        case 0xFF:
            gb->gb_reg.IE = val;
            return;
        }
    }

    (gb->gb_error)(gb, GB_INVALID_WRITE, addr);
}

__core_section("short") static uint8_t __gb_read(struct gb_s *gb, const uint16_t addr)
{
    if likely (addr < 0x4000)
    {
        return gb->gb_rom[addr];
    }
    if likely (addr < 0x8000)
    {
        // TODO: optimize
        return gb->selected_bank_addr[addr];
    }
    if likely (addr >= 0xC000 && addr < 0xE000)
    {
        return gb->wram[addr % WRAM_SIZE];
    }
    return __gb_read_full(gb, addr);
}

__core_section("short") static void __gb_write(struct gb_s *gb, const uint16_t addr, uint8_t v)
{
    if likely (addr >= 0xC000 && addr < 0xE000)
    {
        gb->wram[addr % WRAM_SIZE] = v;
        return;
    }
    __gb_write_full(gb, addr, v);
}

__core static uint16_t __gb_read16(struct gb_s *restrict gb, u16 addr)
{
    // TODO: optimize
    u16 v = __gb_read(gb, addr);
    v |= (u16)__gb_read(gb, addr + 1) << 8;
    return v;
}

__core_section("short") static void __gb_write16(struct gb_s *restrict gb, u16 addr, u16 v)
{
    // TODO: optimize
    __gb_write(gb, addr, v & 0xFF);
    __gb_write(gb, addr + 1, v >> 8);
}

__core_section("short") static uint8_t __gb_fetch8(struct gb_s *restrict gb)
{
    return __gb_read(gb, gb->cpu_reg.pc++);
}

__core_section("short") static uint16_t __gb_fetch16(struct gb_s *restrict gb)
{
    u16 v;
    u16 addr = gb->cpu_reg.pc;

    if likely (addr < 0x3FFF)
    {
        v = gb->gb_rom[addr];
        v |= gb->gb_rom[addr + 1] << 8;
    }
    else if likely (addr >= 0x4000 && addr < 0x7FFF)
    {
        v = gb->selected_bank_addr[addr];
        v |= gb->selected_bank_addr[addr + 1] << 8;
    }
    else
    {
        v = __gb_read16(gb, addr);
    }
    gb->cpu_reg.pc += 2;
    return v;
}

__core_section("short") static uint16_t __gb_pop16(struct gb_s *restrict gb)
{
    u16 v;
    if likely (gb->cpu_reg.sp >= HRAM_ADDR && gb->cpu_reg.sp < 0xFFFE)
    {
        v = gb->hram[gb->cpu_reg.sp - IO_ADDR];
        v |= gb->hram[gb->cpu_reg.sp - IO_ADDR + 1] << 8;
    }
    else
    {
        v = __gb_read16(gb, gb->cpu_reg.sp);
    }
    gb->cpu_reg.sp += 2;
    return v;
}

__core_section("short") static void __gb_push16(struct gb_s *restrict gb, u16 v)
{
    gb->cpu_reg.sp -= 2;

    if likely (gb->cpu_reg.sp >= HRAM_ADDR && gb->cpu_reg.sp < HRAM_ADDR + 0x7E)
    {
        gb->hram[gb->cpu_reg.sp - IO_ADDR] = v & 0xFF;
        gb->hram[gb->cpu_reg.sp - IO_ADDR + 1] = v >> 8;
        return;
    };

    __gb_write16(gb, gb->cpu_reg.sp, v);
}

__core static uint8_t __gb_execute_cb(struct gb_s *gb)
{
    uint8_t inst_cycles;
    uint8_t cbop = __gb_fetch8(gb);
    uint8_t r = (cbop & 0x7) ^ 1;
    uint8_t b = (cbop >> 3) & 0x7;
    uint8_t d = (cbop >> 3) & 0x1;
    uint8_t val;
    uint8_t writeback = 1;

    inst_cycles = 8;
    /* Add an additional 8 cycles to these sets of instructions. */
    switch (cbop & 0xC7)
    {
    case 0x06:
    case 0x86:
    case 0xC6:
        inst_cycles += 8;
        break;
    case 0x46:
        inst_cycles += 4;
        break;
    }

    if (r == 7)
    {
        val = __gb_read(gb, gb->cpu_reg.hl);
    }
    else
    {
        val = gb->cpu_reg_raw[r];
    }

    /* switch based on highest 2 bits */
    switch (cbop >> 6)
    {
    case 0x0:
        cbop = (cbop >> 4) & 0x3;

        switch (cbop)
        {
        case 0x0:  /* RdC R */
        case 0x1:  /* Rd R */
            if (d) /* RRC R / RR R */
            {
                uint8_t temp = val;
                val = (val >> 1);
                val |= cbop ? (gb->cpu_reg.f_bits.c << 7) : (temp << 7);
                gb->cpu_reg.f_bits.z = (val == 0x00);
                gb->cpu_reg.f_bits.n = 0;
                gb->cpu_reg.f_bits.h = 0;
                gb->cpu_reg.f_bits.c = (temp & 0x01);
            }
            else /* RLC R / RL R */
            {
                uint8_t temp = val;
                val = (val << 1);
                val |= cbop ? gb->cpu_reg.f_bits.c : (temp >> 7);
                gb->cpu_reg.f_bits.z = (val == 0x00);
                gb->cpu_reg.f_bits.n = 0;
                gb->cpu_reg.f_bits.h = 0;
                gb->cpu_reg.f_bits.c = (temp >> 7);
            }

            break;

        case 0x2:
            if (d) /* SRA R */
            {
                gb->cpu_reg.f_bits.c = val & 0x01;
                val = (val >> 1) | (val & 0x80);
                gb->cpu_reg.f_bits.z = (val == 0x00);
                gb->cpu_reg.f_bits.n = 0;
                gb->cpu_reg.f_bits.h = 0;
            }
            else /* SLA R */
            {
                gb->cpu_reg.f_bits.c = (val >> 7);
                val = val << 1;
                gb->cpu_reg.f_bits.z = (val == 0x00);
                gb->cpu_reg.f_bits.n = 0;
                gb->cpu_reg.f_bits.h = 0;
            }

            break;

        case 0x3:
            if (d) /* SRL R */
            {
                gb->cpu_reg.f_bits.c = val & 0x01;
                val = val >> 1;
                gb->cpu_reg.f_bits.z = (val == 0x00);
                gb->cpu_reg.f_bits.n = 0;
                gb->cpu_reg.f_bits.h = 0;
            }
            else /* SWAP R */
            {
                uint8_t temp = (val >> 4) & 0x0F;
                temp |= (val << 4) & 0xF0;
                val = temp;
                gb->cpu_reg.f_bits.z = (val == 0x00);
                gb->cpu_reg.f_bits.n = 0;
                gb->cpu_reg.f_bits.h = 0;
                gb->cpu_reg.f_bits.c = 0;
            }

            break;
        }

        break;

    case 0x1: /* BIT B, R */
        gb->cpu_reg.f_bits.z = !((val >> b) & 0x1);
        gb->cpu_reg.f_bits.n = 0;
        gb->cpu_reg.f_bits.h = 1;
        writeback = 0;
        break;

    case 0x2: /* RES B, R */
        val &= (0xFE << b) | (0xFF >> (8 - b));
        break;

    case 0x3: /* SET B, R */
        val |= (0x1 << b);
        break;
    }

    if (writeback)
    {
        if (r == 7)
        {
            __gb_write(gb, gb->cpu_reg.hl, val);
        }
        else
        {
            gb->cpu_reg_raw[r] = val;
        }
    }
    return inst_cycles;
}

#if ENABLE_LCD
struct sprite_data
{
    uint8_t sprite_number;
    uint8_t x;
};

#if PEANUT_GB_HIGH_LCD_ACCURACY
__section__(".text.pgb")
static int compare_sprites(const void *in1, const void *in2)
{
    const struct sprite_data *sd1 = in1, *sd2 = in2;
    int x_res = (int)sd1->x - (int)sd2->x;
    if (x_res != 0)
        return x_res;

    return (int)sd1->sprite_number - (int)sd2->sprite_number;
}
#endif

__core static void __gb_draw_pixel(uint8_t *line, u8 x, u8 v)
{
    u8 *pix = line + x / LCD_PACKING;
    x = (x % LCD_PACKING) * (8 / LCD_PACKING);
    *pix &= ~(((1 << LCD_BITS_PER_PIXEL) - 1) << x);
    *pix |= v << x;
}

__core static u8 __gb_get_pixel(uint8_t *line, u8 x)
{
    u8 *pix = line + x / LCD_PACKING;
    x = (x % LCD_PACKING) * LCD_BITS_PER_PIXEL;
    return (*pix >> x) % (1 << LCD_BITS_PER_PIXEL);
}

// renders one scanline
__core void __gb_draw_line(struct gb_s *gb)
{
    uint8_t *pixels = &gb->lcd[gb->gb_reg.LY * LCD_WIDTH_PACKED];

    __builtin_prefetch(gb->display.line_priority, 1);
    __builtin_prefetch(pixels, 1);

    for (int i = 0; i < PEANUT_GB_ARRAYSIZE(gb->display.line_priority); ++i)
        gb->display.line_priority[i] = 0;

    uint32_t priority_bits = 0;

    /* If background is enabled, draw it. */
    if (gb->gb_reg.LCDC & LCDC_BG_ENABLE)
    {
        /* Calculate current background line to draw. Constant because
         * this function draws only this one line each time it is
         * called. */
        const uint8_t bg_y = gb->gb_reg.LY + gb->gb_reg.SCY;

        /* Get selected background map address for first tile
         * corresponding to current line.
         * 0x20 (32) is the width of a background tile, and the bit
         * shift is to calculate the address. */
        const uint16_t bg_map =
            ((gb->gb_reg.LCDC & LCDC_BG_MAP) ? VRAM_BMAP_2 : VRAM_BMAP_1) +
            (bg_y >> 3) * 0x20;

        /* The displays (what the player sees) X coordinate, drawn right
         * to left. */
        uint8_t disp_x = LCD_WIDTH - 1;

        /* The X coordinate to begin drawing the background at. */
        uint8_t bg_x = disp_x + gb->gb_reg.SCX;

        /* Get tile index for current background tile. */
        uint8_t idx = gb->vram[bg_map + (bg_x >> 3)];
        /* Y coordinate of tile pixel to draw. */
        const uint8_t py = (bg_y & 0x07);
        /* X coordinate of tile pixel to draw. */
        uint8_t px = 7 - (bg_x & 0x07);

        uint16_t tile;

        /* Select addressing mode. */
        if (gb->gb_reg.LCDC & LCDC_TILE_SELECT)
            tile = VRAM_TILES_1 + idx * 0x10;
        else
            tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

        tile += 2 * py;

        /* fetch first tile */
        uint8_t t1 = gb->vram[tile] >> px;
        uint8_t t2 = gb->vram[tile + 1] >> px;

        for (; disp_x != 0xFF; disp_x--)
        {
            if (px == 8)
            {
                /* fetch next tile */
                px = 0;
                bg_x = disp_x + gb->gb_reg.SCX;
                idx = gb->vram[bg_map + (bg_x >> 3)];

                if (gb->gb_reg.LCDC & LCDC_TILE_SELECT)
                    tile = VRAM_TILES_1 + idx * 0x10;
                else
                    tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

                tile += 2 * py;
                t1 = gb->vram[tile];
                t2 = gb->vram[tile + 1];
            }

            /* copy background */
            uint8_t c = (t1 & 0x1) | ((t2 & 0x1) << 1);
            __gb_draw_pixel(pixels, disp_x,
                            gb->display.bg_palette[c] /*| LCD_PALETTE_BG*/);

            t1 >>= 1;
            t2 >>= 1;
            px++;
            priority_bits <<= 1;
            priority_bits |= (c == 0);
            if (disp_x % 32 == 0)
            {
                gb->display.line_priority[disp_x / 32] = priority_bits;
            }
        }
    }

    /* draw window */
    if (gb->gb_reg.LCDC & LCDC_WINDOW_ENABLE &&
        gb->gb_reg.LY >= gb->display.WY && gb->gb_reg.WX <= 166)
    {
        /* Calculate Window Map Address. */
        uint16_t win_line =
            (gb->gb_reg.LCDC & LCDC_WINDOW_MAP) ? VRAM_BMAP_2 : VRAM_BMAP_1;
        win_line += (gb->display.window_clear >> 3) * 0x20;

        uint8_t disp_x = LCD_WIDTH - 1;
        uint8_t win_x = disp_x - gb->gb_reg.WX + 7;

        // look up tile
        uint8_t py = gb->display.window_clear & 0x07;
        uint8_t px = 7 - (win_x & 0x07);
        uint8_t idx = gb->vram[win_line + (win_x >> 3)];

        uint16_t tile;

        if (gb->gb_reg.LCDC & LCDC_TILE_SELECT)
            tile = VRAM_TILES_1 + idx * 0x10;
        else
            tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

        tile += 2 * py;

        // fetch first tile
        uint8_t t1 = gb->vram[tile] >> px;
        uint8_t t2 = gb->vram[tile + 1] >> px;

        // loop & copy window
        uint8_t end = (gb->gb_reg.WX < 7 ? 0 : gb->gb_reg.WX - 7) - 1;

        for (; disp_x != end; disp_x--)
        {
            if (px == 8)
            {
                // fetch next tile
                px = 0;
                win_x = disp_x - gb->gb_reg.WX + 7;
                idx = gb->vram[win_line + (win_x >> 3)];

                if (gb->gb_reg.LCDC & LCDC_TILE_SELECT)
                    tile = VRAM_TILES_1 + idx * 0x10;
                else
                    tile = VRAM_TILES_2 + ((idx + 0x80) % 0x100) * 0x10;

                tile += 2 * py;
                t1 = gb->vram[tile];
                t2 = gb->vram[tile + 1];
            }

            // copy window
            uint8_t c = (t1 & 0x1) | ((t2 & 0x1) << 1);
            __gb_draw_pixel(pixels, disp_x,
                            gb->display.bg_palette[c] /*| LCD_PALETTE_BG*/);

            t1 >>= 1;
            t2 >>= 1;
            px++;

            priority_bits <<= 1;
            priority_bits |= (c == 0);
            if (disp_x % 32 == 0)
            {
                gb->display.line_priority[disp_x / 32] = priority_bits;
            }
        }

        // priority where window begins is a bit tricky
        priority_bits <<= (disp_x % 32);
        gb->display.line_priority[disp_x / 32] &= 0xFFFFFFFF << (disp_x % 32);
        gb->display.line_priority[disp_x / 32] |= priority_bits;

        gb->display.window_clear++;  // advance window line
    }

    // draw sprites
    if (gb->gb_reg.LCDC & LCDC_OBJ_ENABLE)
    {
#if PEANUT_GB_HIGH_LCD_ACCURACY
        uint8_t number_of_sprites = 0;
        struct sprite_data sprites_to_render[NUM_SPRITES];

        /* Record number of sprites on the line being rendered, limited
         * to the maximum number sprites that the Game Boy is able to
         * render on each line (10 sprites). */
        for (uint8_t sprite_number = 0;
             sprite_number < PEANUT_GB_ARRAYSIZE(sprites_to_render);
             sprite_number++)
        {
            /* Sprite Y position. */
            uint8_t OY = gb->oam[4 * sprite_number + 0];
            /* Sprite X position. */
            uint8_t OX = gb->oam[4 * sprite_number + 1];

            /* If sprite isn't on this line, continue. */
            if (gb->gb_reg.LY + (gb->gb_reg.LCDC & LCDC_OBJ_SIZE ? 0 : 8) >=
                    OY ||
                gb->gb_reg.LY + 16 < OY)
                continue;

            sprites_to_render[number_of_sprites].sprite_number = sprite_number;
            sprites_to_render[number_of_sprites].x = OX;
            number_of_sprites++;
        }

        /* If maximum number of sprites reached, prioritise X
         * coordinate and object location in OAM. */
        qsort(&sprites_to_render[0], number_of_sprites,
              sizeof(sprites_to_render[0]), compare_sprites);
        if (number_of_sprites > MAX_SPRITES_LINE)
            number_of_sprites = MAX_SPRITES_LINE;
#endif

        /* Render each sprite, from low priority to high priority. */
#if PEANUT_GB_HIGH_LCD_ACCURACY
        /* Render the top ten prioritised sprites on this scanline. */
        for (uint8_t sprite_number = number_of_sprites - 1;
             sprite_number != 0xFF; sprite_number--)
        {
            uint8_t s = sprites_to_render[sprite_number].sprite_number;
#else
        for (uint8_t sprite_number = NUM_SPRITES - 1; sprite_number != 0xFF;
             sprite_number--)
        {
#endif
            uint8_t s_4 = sprite_number * 4;

            /* Sprite Y position. */
            uint8_t OY = gb->oam[s_4];
            /* Sprite X position. */
            uint8_t OX = gb->oam[s_4 + 1];
            /* Sprite Tile/Pattern Number. */
            uint8_t OT = gb->oam[s_4 + 2] &
                         (gb->gb_reg.LCDC & LCDC_OBJ_SIZE ? 0xFE : 0xFF);
            /* Additional attributes. */
            uint8_t OF = gb->oam[s_4 + 3];

#if !PEANUT_GB_HIGH_LCD_ACCURACY
            /* If sprite isn't on this line, continue. */
            if (gb->gb_reg.LY + (gb->gb_reg.LCDC & LCDC_OBJ_SIZE ? 0 : 8) >=
                    OY ||
                gb->gb_reg.LY + 16 < OY)
                continue;
#endif

            /* Continue if sprite not visible. */
            if (OX == 0 || OX >= 168)
                continue;

            // y flip
            uint8_t py = gb->gb_reg.LY - OY + 16;

            if (OF & OBJ_FLIP_Y)
                py = (gb->gb_reg.LCDC & LCDC_OBJ_SIZE ? 15 : 7) - py;

            uint16_t t1_i = VRAM_TILES_1 + OT * 0x10 + 2 * py;

            // fetch the tile
            uint8_t t1 = gb->vram[t1_i];
            uint8_t t2 = gb->vram[t1_i + 1];

            // handle x flip
            uint8_t dir, start, end, shift;

            if (OF & OBJ_FLIP_X)
            {
                dir = 1;
                start = (OX < 8 ? 0 : OX - 8);
                end = MIN(OX, LCD_WIDTH);
                shift = 8 - OX + start;
            }
            else
            {
                dir = -1;
                start = MIN(OX, LCD_WIDTH) - 1;
                end = (OX < 8 ? 0 : OX - 8) - 1;
                shift = OX - (start + 1);
            }

            // copy tile
            t1 >>= shift;
            t2 >>= shift;

            uint8_t c_add = (OF & OBJ_PALETTE) ? 4 : 0;

            for (uint8_t disp_x = start; disp_x != end; disp_x += dir)
            {
                uint8_t c = (t1 & 0x1) | ((t2 & 0x1) << 1);
                // check transparency / sprite overlap / background overlap
                if (c != 0)  // Sprite palette index 0 is transparent
                {
                    int P_segment_index = disp_x / 32;
                    int P_bit_in_segment = disp_x % 32;

                    uint8_t background_pixel_is_transparent = 0;
                    if (P_segment_index >= 0 &&
                        P_segment_index <
                            PEANUT_GB_ARRAYSIZE(gb->display.line_priority))
                    {
                        background_pixel_is_transparent =
                            (gb->display.line_priority[P_segment_index] >>
                             P_bit_in_segment) &
                            1;
                    }

                    bool sprite_has_behind_bg_attr = (OF & OBJ_PRIORITY);
                    bool should_hide_sprite_pixel =
                        sprite_has_behind_bg_attr &&
                        !background_pixel_is_transparent;

                    if (!should_hide_sprite_pixel)
                    {
                        __gb_draw_pixel(pixels, disp_x,
                                        gb->display.sp_palette[c + c_add]);
                    }
                }

                t1 >>= 1;
                t2 >>= 1;
            }
        }
    }
}
#endif

__shell static unsigned __gb_run_instruction(struct gb_s *gb, uint8_t opcode)
{
    static const uint8_t op_cycles[0x100] = {
        /* clang-format off */
        /*  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F   */
            4,  12, 8,  8,  4,  4,  8,  4,  20, 8,  8,  8,  4,  4,  8,  4,  /* 0x00 */
            4,  12, 8,  8,  4,  4,  8,  4,  12, 8,  8,  8,  4,  4,  8,  4,  /* 0x10 */
            8,  12, 8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4,  /* 0x20 */
            8,  12, 8,  8,  12, 12, 12, 4,  8,  8,  8,  8,  4,  4,  8,  4,  /* 0x30 */

            4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  /* 0x40 */
            4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  /* 0x50 */
            4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  /* 0x60 */
            8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,  /* 0x70 */

            4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  /* 0x80 */
            4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  /* 0x90 */
            4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  /* 0xA0 */
            4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,  /* 0xB0 */

            8,  12, 12, 16, 12, 16, 8,  16, 8,  16, 12, 8,  12, 24, 8,  16, /* 0xC0 */
            8,  12, 12, 0,  12, 16, 8,  16, 8,  16, 12, 0,  12, 0,  8,  16, /* 0xD0 */
            12, 12, 8,  0,  0,  16, 8,  16, 16, 4,  16, 0,  0,  0,  8,  16, /* 0xE0 */
            12, 12, 8,  4,  0,  16, 8,  16, 12, 8,  16, 4,  0,  0,  8,  16  /* 0xF0 */
        /* clang-format on */
    };
    uint8_t inst_cycles = op_cycles[opcode];

    /* Execute opcode */

    static const void *op_table[256] = {
        &&exit,     &&_0x01,    &&_0x02,    &&_0x03,    &&_0x04,    &&_0x05,
        &&_0x06,    &&_0x07,    &&_0x08,    &&_0x09,    &&_0x0A,    &&_0x0B,
        &&_0x0C,    &&_0x0D,    &&_0x0E,    &&_0x0F,    &&_0x10,    &&_0x11,
        &&_0x12,    &&_0x13,    &&_0x14,    &&_0x15,    &&_0x16,    &&_0x17,
        &&_0x18,    &&_0x19,    &&_0x1A,    &&_0x1B,    &&_0x1C,    &&_0x1D,
        &&_0x1E,    &&_0x1F,    &&_0x20,    &&_0x21,    &&_0x22,    &&_0x23,
        &&_0x24,    &&_0x25,    &&_0x26,    &&_0x27,    &&_0x28,    &&_0x29,
        &&_0x2A,    &&_0x2B,    &&_0x2C,    &&_0x2D,    &&_0x2E,    &&_0x2F,
        &&_0x30,    &&_0x31,    &&_0x32,    &&_0x33,    &&_0x34,    &&_0x35,
        &&_0x36,    &&_0x37,    &&_0x38,    &&_0x39,    &&_0x3A,    &&_0x3B,
        &&_0x3C,    &&_0x3D,    &&_0x3E,    &&_0x3F,    &&_0x40,    &&_0x41,
        &&_0x42,    &&_0x43,    &&_0x44,    &&_0x45,    &&_0x46,    &&_0x47,
        &&_0x48,    &&_0x49,    &&_0x4A,    &&_0x4B,    &&_0x4C,    &&_0x4D,
        &&_0x4E,    &&_0x4F,    &&_0x50,    &&_0x51,    &&_0x52,    &&_0x53,
        &&_0x54,    &&_0x55,    &&_0x56,    &&_0x57,    &&_0x58,    &&_0x59,
        &&_0x5A,    &&_0x5B,    &&_0x5C,    &&_0x5D,    &&_0x5E,    &&_0x5F,
        &&_0x60,    &&_0x61,    &&_0x62,    &&_0x63,    &&_0x64,    &&_0x65,
        &&_0x66,    &&_0x67,    &&_0x68,    &&_0x69,    &&_0x6A,    &&_0x6B,
        &&_0x6C,    &&_0x6D,    &&_0x6E,    &&_0x6F,    &&_0x70,    &&_0x71,
        &&_0x72,    &&_0x73,    &&_0x74,    &&_0x75,    &&_0x76,    &&_0x77,
        &&_0x78,    &&_0x79,    &&_0x7A,    &&_0x7B,    &&_0x7C,    &&_0x7D,
        &&_0x7E,    &&_0x7F,    &&_0x80,    &&_0x81,    &&_0x82,    &&_0x83,
        &&_0x84,    &&_0x85,    &&_0x86,    &&_0x87,    &&_0x88,    &&_0x89,
        &&_0x8A,    &&_0x8B,    &&_0x8C,    &&_0x8D,    &&_0x8E,    &&_0x8F,
        &&_0x90,    &&_0x91,    &&_0x92,    &&_0x93,    &&_0x94,    &&_0x95,
        &&_0x96,    &&_0x97,    &&_0x98,    &&_0x99,    &&_0x9A,    &&_0x9B,
        &&_0x9C,    &&_0x9D,    &&_0x9E,    &&_0x9F,    &&_0xA0,    &&_0xA1,
        &&_0xA2,    &&_0xA3,    &&_0xA4,    &&_0xA5,    &&_0xA6,    &&_0xA7,
        &&_0xA8,    &&_0xA9,    &&_0xAA,    &&_0xAB,    &&_0xAC,    &&_0xAD,
        &&_0xAE,    &&_0xAF,    &&_0xB0,    &&_0xB1,    &&_0xB2,    &&_0xB3,
        &&_0xB4,    &&_0xB5,    &&_0xB6,    &&_0xB7,    &&_0xB8,    &&_0xB9,
        &&_0xBA,    &&_0xBB,    &&_0xBC,    &&_0xBD,    &&_0xBE,    &&_0xBF,
        &&_0xC0,    &&_0xC1,    &&_0xC2,    &&_0xC3,    &&_0xC4,    &&_0xC5,
        &&_0xC6,    &&_0xC7,    &&_0xC8,    &&_0xC9,    &&_0xCA,    &&_0xCB,
        &&_0xCC,    &&_0xCD,    &&_0xCE,    &&_0xCF,    &&_0xD0,    &&_0xD1,
        &&_0xD2,    &&_invalid, &&_0xD4,    &&_0xD5,    &&_0xD6,    &&_0xD7,
        &&_0xD8,    &&_0xD9,    &&_0xDA,    &&_invalid, &&_0xDC,    &&_invalid,
        &&_0xDE,    &&_0xDF,    &&_0xE0,    &&_0xE1,    &&_0xE2,    &&_invalid,
        &&_invalid, &&_0xE5,    &&_0xE6,    &&_0xE7,    &&_0xE8,    &&_0xE9,
        &&_0xEA,    &&_invalid, &&_invalid, &&_invalid, &&_0xEE,    &&_0xEF,
        &&_0xF0,    &&_0xF1,    &&_0xF2,    &&_0xF3,    &&_invalid, &&_0xF5,
        &&_0xF6,    &&_0xF7,    &&_0xF8,    &&_0xF9,    &&_0xFA,    &&_0xFB,
        &&_invalid, &&_invalid, &&_0xFE,    &&_0xFF};

    goto *op_table[opcode];

_0x00:
{ /* NOP */
    goto exit;
}

_0x01:
{ /* LD BC, imm */
    gb->cpu_reg.c = __gb_read_full(gb, gb->cpu_reg.pc++);
    gb->cpu_reg.b = __gb_read_full(gb, gb->cpu_reg.pc++);
    goto exit;
}

_0x02:
{ /* LD (BC), A */
    __gb_write_full(gb, gb->cpu_reg.bc, gb->cpu_reg.a);
    goto exit;
}

_0x03:
{ /* INC BC */
    gb->cpu_reg.bc++;
    goto exit;
}

_0x04:
{ /* INC B */
    gb->cpu_reg.b++;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.b == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.b & 0x0F) == 0x00);
    goto exit;
}

_0x05:
{ /* DEC B */
    gb->cpu_reg.b--;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.b == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.b & 0x0F) == 0x0F);
    goto exit;
}

_0x06:
{ /* LD B, imm */
    gb->cpu_reg.b = __gb_read_full(gb, gb->cpu_reg.pc++);
    goto exit;
}

_0x07:
{ /* RLCA */
    gb->cpu_reg.a = (gb->cpu_reg.a << 1) | (gb->cpu_reg.a >> 7);
    gb->cpu_reg.f_bits.z = 0;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = (gb->cpu_reg.a & 0x01);
    goto exit;
}

_0x08:
{ /* LD (imm), SP */
    uint16_t temp = __gb_read_full(gb, gb->cpu_reg.pc++);
    temp |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
    __gb_write_full(gb, temp++, gb->cpu_reg.sp & 0xFF);
    __gb_write_full(gb, temp, gb->cpu_reg.sp >> 8);
    goto exit;
}

_0x09:
{ /* ADD HL, BC */
    uint_fast32_t temp = gb->cpu_reg.hl + gb->cpu_reg.bc;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (temp ^ gb->cpu_reg.hl ^ gb->cpu_reg.bc) & 0x1000 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFFFF0000) ? 1 : 0;
    gb->cpu_reg.hl = (temp & 0x0000FFFF);
    goto exit;
}

_0x0A:
{ /* LD A, (BC) */
    gb->cpu_reg.a = __gb_read_full(gb, gb->cpu_reg.bc);
    goto exit;
}

_0x0B:
{ /* DEC BC */
    gb->cpu_reg.bc--;
    goto exit;
}

_0x0C:
{ /* INC C */
    gb->cpu_reg.c++;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.c == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.c & 0x0F) == 0x00);
    goto exit;
}

_0x0D:
{ /* DEC C */
    gb->cpu_reg.c--;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.c == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.c & 0x0F) == 0x0F);
    goto exit;
}

_0x0E:
{ /* LD C, imm */
    gb->cpu_reg.c = __gb_read_full(gb, gb->cpu_reg.pc++);
    goto exit;
}

_0x0F:
{ /* RRCA */
    gb->cpu_reg.f_bits.c = gb->cpu_reg.a & 0x01;
    gb->cpu_reg.a = (gb->cpu_reg.a >> 1) | (gb->cpu_reg.a << 7);
    gb->cpu_reg.f_bits.z = 0;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    goto exit;
}

_0x10:
{ /* STOP */
    // gb->gb_halt = 1;
    goto exit;
}

_0x11:
{ /* LD DE, imm */
    gb->cpu_reg.e = __gb_read_full(gb, gb->cpu_reg.pc++);
    gb->cpu_reg.d = __gb_read_full(gb, gb->cpu_reg.pc++);
    goto exit;
}

_0x12:
{ /* LD (DE), A */
    __gb_write_full(gb, gb->cpu_reg.de, gb->cpu_reg.a);
    goto exit;
}

_0x13:
{ /* INC DE */
    gb->cpu_reg.de++;
    goto exit;
}

_0x14:
{ /* INC D */
    gb->cpu_reg.d++;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.d == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.d & 0x0F) == 0x00);
    goto exit;
}

_0x15:
{ /* DEC D */
    gb->cpu_reg.d--;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.d == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.d & 0x0F) == 0x0F);
    goto exit;
}

_0x16:
{ /* LD D, imm */
    gb->cpu_reg.d = __gb_read_full(gb, gb->cpu_reg.pc++);
    goto exit;
}

_0x17:
{ /* RLA */
    uint8_t temp = gb->cpu_reg.a;
    gb->cpu_reg.a = (gb->cpu_reg.a << 1) | gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = 0;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = (temp >> 7) & 0x01;
    goto exit;
}

_0x18:
{ /* JR imm */
    int8_t temp = (int8_t)__gb_read_full(gb, gb->cpu_reg.pc++);
    gb->cpu_reg.pc += temp;
    goto exit;
}

_0x19:
{ /* ADD HL, DE */
    uint_fast32_t temp = gb->cpu_reg.hl + gb->cpu_reg.de;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (temp ^ gb->cpu_reg.hl ^ gb->cpu_reg.de) & 0x1000 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFFFF0000) ? 1 : 0;
    gb->cpu_reg.hl = (temp & 0x0000FFFF);
    goto exit;
}

_0x1A:
{ /* LD A, (DE) */
    gb->cpu_reg.a = __gb_read_full(gb, gb->cpu_reg.de);
    goto exit;
}

_0x1B:
{ /* DEC DE */
    gb->cpu_reg.de--;
    goto exit;
}

_0x1C:
{ /* INC E */
    gb->cpu_reg.e++;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.e == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.e & 0x0F) == 0x00);
    goto exit;
}

_0x1D:
{ /* DEC E */
    gb->cpu_reg.e--;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.e == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.e & 0x0F) == 0x0F);
    goto exit;
}

_0x1E:
{ /* LD E, imm */
    gb->cpu_reg.e = __gb_read_full(gb, gb->cpu_reg.pc++);
    goto exit;
}

_0x1F:
{ /* RRA */
    uint8_t temp = gb->cpu_reg.a;
    gb->cpu_reg.a = gb->cpu_reg.a >> 1 | (gb->cpu_reg.f_bits.c << 7);
    gb->cpu_reg.f_bits.z = 0;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = temp & 0x1;
    goto exit;
}

_0x20:
{ /* JP NZ, imm */
    if (!gb->cpu_reg.f_bits.z)
    {
        int8_t temp = (int8_t)__gb_read_full(gb, gb->cpu_reg.pc++);
        gb->cpu_reg.pc += temp;
        inst_cycles += 4;
    }
    else
        gb->cpu_reg.pc++;

    goto exit;
}

_0x21:
{ /* LD HL, imm */
    gb->cpu_reg.l = __gb_read_full(gb, gb->cpu_reg.pc++);
    gb->cpu_reg.h = __gb_read_full(gb, gb->cpu_reg.pc++);
    goto exit;
}

_0x22:
{ /* LDI (HL), A */
    __gb_write_full(gb, gb->cpu_reg.hl, gb->cpu_reg.a);
    gb->cpu_reg.hl++;
    goto exit;
}

_0x23:
{ /* INC HL */
    gb->cpu_reg.hl++;
    goto exit;
}

_0x24:
{ /* INC H */
    gb->cpu_reg.h++;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.h == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.h & 0x0F) == 0x00);
    goto exit;
}

_0x25:
{ /* DEC H */
    gb->cpu_reg.h--;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.h == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.h & 0x0F) == 0x0F);
    goto exit;
}

_0x26:
{ /* LD H, imm */
    gb->cpu_reg.h = __gb_read_full(gb, gb->cpu_reg.pc++);
    goto exit;
}

_0x27:
{ /* DAA */
    uint16_t a = gb->cpu_reg.a;

    if (gb->cpu_reg.f_bits.n)
    {
        if (gb->cpu_reg.f_bits.h)
            a = (a - 0x06) & 0xFF;

        if (gb->cpu_reg.f_bits.c)
            a -= 0x60;
    }
    else
    {
        if (gb->cpu_reg.f_bits.h || (a & 0x0F) > 9)
            a += 0x06;

        if (gb->cpu_reg.f_bits.c || a > 0x9F)
            a += 0x60;
    }

    if ((a & 0x100) == 0x100)
        gb->cpu_reg.f_bits.c = 1;

    gb->cpu_reg.a = a;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0);
    gb->cpu_reg.f_bits.h = 0;

    goto exit;
}

_0x28:
{ /* JP Z, imm */
    if (gb->cpu_reg.f_bits.z)
    {
        int8_t temp = (int8_t)__gb_read_full(gb, gb->cpu_reg.pc++);
        gb->cpu_reg.pc += temp;
        inst_cycles += 4;
    }
    else
        gb->cpu_reg.pc++;

    goto exit;
}

_0x29:
{ /* ADD HL, HL */
    uint_fast32_t temp = gb->cpu_reg.hl + gb->cpu_reg.hl;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = (temp & 0x1000) ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFFFF0000) ? 1 : 0;
    gb->cpu_reg.hl = (temp & 0x0000FFFF);
    goto exit;
}

_0x2A:
{ /* LD A, (HL+) */
    gb->cpu_reg.a = __gb_read_full(gb, gb->cpu_reg.hl++);
    goto exit;
}

_0x2B:
{ /* DEC HL */
    gb->cpu_reg.hl--;
    goto exit;
}

_0x2C:
{ /* INC L */
    gb->cpu_reg.l++;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.l == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.l & 0x0F) == 0x00);
    goto exit;
}

_0x2D:
{ /* DEC L */
    gb->cpu_reg.l--;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.l == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.l & 0x0F) == 0x0F);
    goto exit;
}

_0x2E:
{ /* LD L, imm */
    gb->cpu_reg.l = __gb_read_full(gb, gb->cpu_reg.pc++);
    goto exit;
}

_0x2F:
{ /* CPL */
    gb->cpu_reg.a = ~gb->cpu_reg.a;
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = 1;
    goto exit;
}

_0x30:
{ /* JP NC, imm */
    if (!gb->cpu_reg.f_bits.c)
    {
        int8_t temp = (int8_t)__gb_read_full(gb, gb->cpu_reg.pc++);
        gb->cpu_reg.pc += temp;
        inst_cycles += 4;
    }
    else
        gb->cpu_reg.pc++;

    goto exit;
}

_0x31:
{ /* LD SP, imm */
    gb->cpu_reg.sp = __gb_read_full(gb, gb->cpu_reg.pc++);
    gb->cpu_reg.sp |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
    goto exit;
}

_0x32:
{ /* LD (HL), A */
    __gb_write_full(gb, gb->cpu_reg.hl, gb->cpu_reg.a);
    gb->cpu_reg.hl--;
    goto exit;
}

_0x33:
{ /* INC SP */
    gb->cpu_reg.sp++;
    goto exit;
}

_0x34:
{ /* INC (HL) */
    uint8_t temp = __gb_read_full(gb, gb->cpu_reg.hl) + 1;
    gb->cpu_reg.f_bits.z = (temp == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = ((temp & 0x0F) == 0x00);
    __gb_write_full(gb, gb->cpu_reg.hl, temp);
    goto exit;
}

_0x35:
{ /* DEC (HL) */
    uint8_t temp = __gb_read_full(gb, gb->cpu_reg.hl) - 1;
    gb->cpu_reg.f_bits.z = (temp == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = ((temp & 0x0F) == 0x0F);
    __gb_write_full(gb, gb->cpu_reg.hl, temp);
    goto exit;
}

_0x36:
{ /* LD (HL), imm */
    __gb_write_full(gb, gb->cpu_reg.hl, __gb_read_full(gb, gb->cpu_reg.pc++));
    goto exit;
}

_0x37:
{ /* SCF */
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 1;
    goto exit;
}

_0x38:
{ /* JP C, imm */
    if (gb->cpu_reg.f_bits.c)
    {
        int8_t temp = (int8_t)__gb_read_full(gb, gb->cpu_reg.pc++);
        gb->cpu_reg.pc += temp;
        inst_cycles += 4;
    }
    else
        gb->cpu_reg.pc++;

    goto exit;
}

_0x39:
{ /* ADD HL, SP */
    uint_fast32_t temp = gb->cpu_reg.hl + gb->cpu_reg.sp;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        ((gb->cpu_reg.hl & 0xFFF) + (gb->cpu_reg.sp & 0xFFF)) & 0x1000 ? 1 : 0;
    gb->cpu_reg.f_bits.c = temp & 0x10000 ? 1 : 0;
    gb->cpu_reg.hl = (uint16_t)temp;
    goto exit;
}

_0x3A:
{ /* LD A, (HL) */
    gb->cpu_reg.a = __gb_read_full(gb, gb->cpu_reg.hl--);
    goto exit;
}

_0x3B:
{ /* DEC SP */
    gb->cpu_reg.sp--;
    goto exit;
}

_0x3C:
{ /* INC A */
    gb->cpu_reg.a++;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.a & 0x0F) == 0x00);
    goto exit;
}

_0x3D:
{ /* DEC A */
    gb->cpu_reg.a--;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.a & 0x0F) == 0x0F);
    goto exit;
}

_0x3E:
{ /* LD A, imm */
    gb->cpu_reg.a = __gb_read_full(gb, gb->cpu_reg.pc++);
    goto exit;
}

_0x3F:
{ /* CCF */
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = ~gb->cpu_reg.f_bits.c;
    goto exit;
}

_0x40:
{ /* LD B, B */
    goto exit;
}

_0x41:
{ /* LD B, C */
    gb->cpu_reg.b = gb->cpu_reg.c;
    goto exit;
}

_0x42:
{ /* LD B, D */
    gb->cpu_reg.b = gb->cpu_reg.d;
    goto exit;
}

_0x43:
{ /* LD B, E */
    gb->cpu_reg.b = gb->cpu_reg.e;
    goto exit;
}

_0x44:
{ /* LD B, H */
    gb->cpu_reg.b = gb->cpu_reg.h;
    goto exit;
}

_0x45:
{ /* LD B, L */
    gb->cpu_reg.b = gb->cpu_reg.l;
    goto exit;
}

_0x46:
{ /* LD B, (HL) */
    gb->cpu_reg.b = __gb_read_full(gb, gb->cpu_reg.hl);
    goto exit;
}

_0x47:
{ /* LD B, A */
    gb->cpu_reg.b = gb->cpu_reg.a;
    goto exit;
}

_0x48:
{ /* LD C, B */
    gb->cpu_reg.c = gb->cpu_reg.b;
    goto exit;
}

_0x49:
{ /* LD C, C */
    goto exit;
}

_0x4A:
{ /* LD C, D */
    gb->cpu_reg.c = gb->cpu_reg.d;
    goto exit;
}

_0x4B:
{ /* LD C, E */
    gb->cpu_reg.c = gb->cpu_reg.e;
    goto exit;
}

_0x4C:
{ /* LD C, H */
    gb->cpu_reg.c = gb->cpu_reg.h;
    goto exit;
}

_0x4D:
{ /* LD C, L */
    gb->cpu_reg.c = gb->cpu_reg.l;
    goto exit;
}

_0x4E:
{ /* LD C, (HL) */
    gb->cpu_reg.c = __gb_read_full(gb, gb->cpu_reg.hl);
    goto exit;
}

_0x4F:
{ /* LD C, A */
    gb->cpu_reg.c = gb->cpu_reg.a;
    goto exit;
}

_0x50:
{ /* LD D, B */
    gb->cpu_reg.d = gb->cpu_reg.b;
    goto exit;
}

_0x51:
{ /* LD D, C */
    gb->cpu_reg.d = gb->cpu_reg.c;
    goto exit;
}

_0x52:
{ /* LD D, D */
    goto exit;
}

_0x53:
{ /* LD D, E */
    gb->cpu_reg.d = gb->cpu_reg.e;
    goto exit;
}

_0x54:
{ /* LD D, H */
    gb->cpu_reg.d = gb->cpu_reg.h;
    goto exit;
}

_0x55:
{ /* LD D, L */
    gb->cpu_reg.d = gb->cpu_reg.l;
    goto exit;
}

_0x56:
{ /* LD D, (HL) */
    gb->cpu_reg.d = __gb_read_full(gb, gb->cpu_reg.hl);
    goto exit;
}

_0x57:
{ /* LD D, A */
    gb->cpu_reg.d = gb->cpu_reg.a;
    goto exit;
}

_0x58:
{ /* LD E, B */
    gb->cpu_reg.e = gb->cpu_reg.b;
    goto exit;
}

_0x59:
{ /* LD E, C */
    gb->cpu_reg.e = gb->cpu_reg.c;
    goto exit;
}

_0x5A:
{ /* LD E, D */
    gb->cpu_reg.e = gb->cpu_reg.d;
    goto exit;
}

_0x5B:
{ /* LD E, E */
    goto exit;
}

_0x5C:
{ /* LD E, H */
    gb->cpu_reg.e = gb->cpu_reg.h;
    goto exit;
}

_0x5D:
{ /* LD E, L */
    gb->cpu_reg.e = gb->cpu_reg.l;
    goto exit;
}

_0x5E:
{ /* LD E, (HL) */
    gb->cpu_reg.e = __gb_read_full(gb, gb->cpu_reg.hl);
    goto exit;
}

_0x5F:
{ /* LD E, A */
    gb->cpu_reg.e = gb->cpu_reg.a;
    goto exit;
}

_0x60:
{ /* LD H, B */
    gb->cpu_reg.h = gb->cpu_reg.b;
    goto exit;
}

_0x61:
{ /* LD H, C */
    gb->cpu_reg.h = gb->cpu_reg.c;
    goto exit;
}

_0x62:
{ /* LD H, D */
    gb->cpu_reg.h = gb->cpu_reg.d;
    goto exit;
}

_0x63:
{ /* LD H, E */
    gb->cpu_reg.h = gb->cpu_reg.e;
    goto exit;
}

_0x64:
{ /* LD H, H */
    goto exit;
}

_0x65:
{ /* LD H, L */
    gb->cpu_reg.h = gb->cpu_reg.l;
    goto exit;
}

_0x66:
{ /* LD H, (HL) */
    gb->cpu_reg.h = __gb_read_full(gb, gb->cpu_reg.hl);
    goto exit;
}

_0x67:
{ /* LD H, A */
    gb->cpu_reg.h = gb->cpu_reg.a;
    goto exit;
}

_0x68:
{ /* LD L, B */
    gb->cpu_reg.l = gb->cpu_reg.b;
    goto exit;
}

_0x69:
{ /* LD L, C */
    gb->cpu_reg.l = gb->cpu_reg.c;
    goto exit;
}

_0x6A:
{ /* LD L, D */
    gb->cpu_reg.l = gb->cpu_reg.d;
    goto exit;
}

_0x6B:
{ /* LD L, E */
    gb->cpu_reg.l = gb->cpu_reg.e;
    goto exit;
}

_0x6C:
{ /* LD L, H */
    gb->cpu_reg.l = gb->cpu_reg.h;
    goto exit;
}

_0x6D:
{ /* LD L, L */
    goto exit;
}

_0x6E:
{ /* LD L, (HL) */
    gb->cpu_reg.l = __gb_read_full(gb, gb->cpu_reg.hl);
    goto exit;
}

_0x6F:
{ /* LD L, A */
    gb->cpu_reg.l = gb->cpu_reg.a;
    goto exit;
}

_0x70:
{ /* LD (HL), B */
    __gb_write_full(gb, gb->cpu_reg.hl, gb->cpu_reg.b);
    goto exit;
}

_0x71:
{ /* LD (HL), C */
    __gb_write_full(gb, gb->cpu_reg.hl, gb->cpu_reg.c);
    goto exit;
}

_0x72:
{ /* LD (HL), D */
    __gb_write_full(gb, gb->cpu_reg.hl, gb->cpu_reg.d);
    goto exit;
}

_0x73:
{ /* LD (HL), E */
    __gb_write_full(gb, gb->cpu_reg.hl, gb->cpu_reg.e);
    goto exit;
}

_0x74:
{ /* LD (HL), H */
    __gb_write_full(gb, gb->cpu_reg.hl, gb->cpu_reg.h);
    goto exit;
}

_0x75:
{ /* LD (HL), L */
    __gb_write_full(gb, gb->cpu_reg.hl, gb->cpu_reg.l);
    goto exit;
}

_0x76:
{ /* HALT */
    /* TODO: Emulate HALT bug? */
    gb->gb_halt = 1;
    goto exit;
}

_0x77:
{ /* LD (HL), A */
    __gb_write_full(gb, gb->cpu_reg.hl, gb->cpu_reg.a);
    goto exit;
}

_0x78:
{ /* LD A, B */
    gb->cpu_reg.a = gb->cpu_reg.b;
    goto exit;
}

_0x79:
{ /* LD A, C */
    gb->cpu_reg.a = gb->cpu_reg.c;
    goto exit;
}

_0x7A:
{ /* LD A, D */
    gb->cpu_reg.a = gb->cpu_reg.d;
    goto exit;
}

_0x7B:
{ /* LD A, E */
    gb->cpu_reg.a = gb->cpu_reg.e;
    goto exit;
}

_0x7C:
{ /* LD A, H */
    gb->cpu_reg.a = gb->cpu_reg.h;
    goto exit;
}

_0x7D:
{ /* LD A, L */
    gb->cpu_reg.a = gb->cpu_reg.l;
    goto exit;
}

_0x7E:
{ /* LD A, (HL) */
    gb->cpu_reg.a = __gb_read_full(gb, gb->cpu_reg.hl);
    goto exit;
}

_0x7F:
{ /* LD A, A */
    goto exit;
}

_0x80:
{ /* ADD A, B */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.b;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.b ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x81:
{ /* ADD A, C */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.c ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x82:
{ /* ADD A, D */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.d;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.d ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x83:
{ /* ADD A, E */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.e;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.e ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x84:
{ /* ADD A, H */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.h;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.h ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x85:
{ /* ADD A, L */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.l;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.l ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x86:
{ /* ADD A, (HL) */
    uint8_t hl = __gb_read_full(gb, gb->cpu_reg.hl);
    uint16_t temp = gb->cpu_reg.a + hl;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = (gb->cpu_reg.a ^ hl ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x87:
{ /* ADD A, A */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.a;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = temp & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x88:
{ /* ADC A, B */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.b + gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.b ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x89:
{ /* ADC A, C */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.c + gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.c ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x8A:
{ /* ADC A, D */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.d + gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.d ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x8B:
{ /* ADC A, E */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.e + gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.e ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x8C:
{ /* ADC A, H */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.h + gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.h ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x8D:
{ /* ADC A, L */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.l + gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.l ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x8E:
{ /* ADC A, (HL) */
    uint8_t val = __gb_read_full(gb, gb->cpu_reg.hl);
    uint16_t temp = gb->cpu_reg.a + val + gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = (gb->cpu_reg.a ^ val ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x8F:
{ /* ADC A, A */
    uint16_t temp = gb->cpu_reg.a + gb->cpu_reg.a + gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    /* TODO: Optimisation here? */
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.a ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x90:
{ /* SUB B */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.b;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.b ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x91:
{ /* SUB C */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.c ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x92:
{ /* SUB D */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.d;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.d ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x93:
{ /* SUB E */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.e;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.e ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x94:
{ /* SUB H */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.h;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.h ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x95:
{ /* SUB L */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.l;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.l ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x96:
{ /* SUB (HL) */
    uint8_t val = __gb_read_full(gb, gb->cpu_reg.hl);
    uint16_t temp = gb->cpu_reg.a - val;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = (gb->cpu_reg.a ^ val ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x97:
{ /* SUB A */
    gb->cpu_reg.a = 0;
    gb->cpu_reg.f_bits.z = 1;
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0x98:
{ /* SBC A, B */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.b - gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.b ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x99:
{ /* SBC A, C */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.c - gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.c ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x9A:
{ /* SBC A, D */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.d - gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.d ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x9B:
{ /* SBC A, E */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.e - gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.e ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x9C:
{ /* SBC A, H */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.h - gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.h ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x9D:
{ /* SBC A, L */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.l - gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.l ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x9E:
{ /* SBC A, (HL) */
    uint8_t val = __gb_read_full(gb, gb->cpu_reg.hl);
    uint16_t temp = gb->cpu_reg.a - val - gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = (gb->cpu_reg.a ^ val ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0x9F:
{ /* SBC A, A */
    gb->cpu_reg.a = gb->cpu_reg.f_bits.c ? 0xFF : 0x00;
    gb->cpu_reg.f_bits.z = !gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = gb->cpu_reg.f_bits.c;
    goto exit;
}

_0xA0:
{ /* AND B */
    gb->cpu_reg.a = gb->cpu_reg.a & gb->cpu_reg.b;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 1;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xA1:
{ /* AND C */
    gb->cpu_reg.a = gb->cpu_reg.a & gb->cpu_reg.c;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 1;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xA2:
{ /* AND D */
    gb->cpu_reg.a = gb->cpu_reg.a & gb->cpu_reg.d;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 1;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xA3:
{ /* AND E */
    gb->cpu_reg.a = gb->cpu_reg.a & gb->cpu_reg.e;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 1;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xA4:
{ /* AND H */
    gb->cpu_reg.a = gb->cpu_reg.a & gb->cpu_reg.h;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 1;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xA5:
{ /* AND L */
    gb->cpu_reg.a = gb->cpu_reg.a & gb->cpu_reg.l;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 1;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xA6:
{ /* AND B */
    gb->cpu_reg.a = gb->cpu_reg.a & __gb_read_full(gb, gb->cpu_reg.hl);
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 1;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xA7:
{ /* AND A */
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 1;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xA8:
{ /* XOR B */
    gb->cpu_reg.a = gb->cpu_reg.a ^ gb->cpu_reg.b;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xA9:
{ /* XOR C */
    gb->cpu_reg.a = gb->cpu_reg.a ^ gb->cpu_reg.c;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xAA:
{ /* XOR D */
    gb->cpu_reg.a = gb->cpu_reg.a ^ gb->cpu_reg.d;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xAB:
{ /* XOR E */
    gb->cpu_reg.a = gb->cpu_reg.a ^ gb->cpu_reg.e;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xAC:
{ /* XOR H */
    gb->cpu_reg.a = gb->cpu_reg.a ^ gb->cpu_reg.h;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xAD:
{ /* XOR L */
    gb->cpu_reg.a = gb->cpu_reg.a ^ gb->cpu_reg.l;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xAE:
{ /* XOR (HL) */
    gb->cpu_reg.a = gb->cpu_reg.a ^ __gb_read_full(gb, gb->cpu_reg.hl);
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xAF:
{ /* XOR A */
    gb->cpu_reg.a = 0x00;
    gb->cpu_reg.f_bits.z = 1;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xB0:
{ /* OR B */
    gb->cpu_reg.a = gb->cpu_reg.a | gb->cpu_reg.b;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xB1:
{ /* OR C */
    gb->cpu_reg.a = gb->cpu_reg.a | gb->cpu_reg.c;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xB2:
{ /* OR D */
    gb->cpu_reg.a = gb->cpu_reg.a | gb->cpu_reg.d;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xB3:
{ /* OR E */
    gb->cpu_reg.a = gb->cpu_reg.a | gb->cpu_reg.e;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xB4:
{ /* OR H */
    gb->cpu_reg.a = gb->cpu_reg.a | gb->cpu_reg.h;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xB5:
{ /* OR L */
    gb->cpu_reg.a = gb->cpu_reg.a | gb->cpu_reg.l;
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xB6:
{ /* OR (HL) */
    gb->cpu_reg.a = gb->cpu_reg.a | __gb_read_full(gb, gb->cpu_reg.hl);
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xB7:
{ /* OR A */
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xB8:
{ /* CP B */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.b;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.b ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    goto exit;
}

_0xB9:
{ /* CP C */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.c;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.c ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    goto exit;
}

_0xBA:
{ /* CP D */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.d;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.d ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    goto exit;
}

_0xBB:
{ /* CP E */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.e;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.e ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    goto exit;
}

_0xBC:
{ /* CP H */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.h;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.h ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    goto exit;
}

_0xBD:
{ /* CP L */
    uint16_t temp = gb->cpu_reg.a - gb->cpu_reg.l;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h =
        (gb->cpu_reg.a ^ gb->cpu_reg.l ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    goto exit;
}

/* TODO: Optimsation by combining similar opcode routines. */
_0xBE:
{ /* CP B */
    uint8_t val = __gb_read_full(gb, gb->cpu_reg.hl);
    uint16_t temp = gb->cpu_reg.a - val;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = (gb->cpu_reg.a ^ val ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    goto exit;
}

_0xBF:
{ /* CP A */
    gb->cpu_reg.f_bits.z = 1;
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xC0:
{ /* RET NZ */
    if (!gb->cpu_reg.f_bits.z)
    {
        gb->cpu_reg.pc = __gb_read_full(gb, gb->cpu_reg.sp++);
        gb->cpu_reg.pc |= __gb_read_full(gb, gb->cpu_reg.sp++) << 8;
        inst_cycles += 12;
    }

    goto exit;
}

_0xC1:
{ /* POP BC */
    gb->cpu_reg.c = __gb_read_full(gb, gb->cpu_reg.sp++);
    gb->cpu_reg.b = __gb_read_full(gb, gb->cpu_reg.sp++);
    goto exit;
}

_0xC2:
{ /* JP NZ, imm */
    if (!gb->cpu_reg.f_bits.z)
    {
        uint16_t temp = __gb_read_full(gb, gb->cpu_reg.pc++);
        temp |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
        gb->cpu_reg.pc = temp;
        inst_cycles += 4;
    }
    else
        gb->cpu_reg.pc += 2;

    goto exit;
}

_0xC3:
{ /* JP imm */
    uint16_t temp = __gb_read_full(gb, gb->cpu_reg.pc++);
    temp |= __gb_read_full(gb, gb->cpu_reg.pc) << 8;
    gb->cpu_reg.pc = temp;
    goto exit;
}

_0xC4:
{ /* CALL NZ imm */
    if (!gb->cpu_reg.f_bits.z)
    {
        uint16_t temp = __gb_read_full(gb, gb->cpu_reg.pc++);
        temp |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
        __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
        __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
        gb->cpu_reg.pc = temp;
        inst_cycles += 12;
    }
    else
        gb->cpu_reg.pc += 2;

    goto exit;
}

_0xC5:
{ /* PUSH BC */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.b);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.c);
    goto exit;
}

_0xC6:
{ /* ADD A, imm */
    /* Taken from SameBoy, which is released under MIT Licence. */
    uint8_t value = __gb_read_full(gb, gb->cpu_reg.pc++);
    uint16_t calc = gb->cpu_reg.a + value;
    gb->cpu_reg.f_bits.z = ((uint8_t)calc == 0) ? 1 : 0;
    gb->cpu_reg.f_bits.h =
        ((gb->cpu_reg.a & 0xF) + (value & 0xF) > 0x0F) ? 1 : 0;
    gb->cpu_reg.f_bits.c = calc > 0xFF ? 1 : 0;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.a = (uint8_t)calc;
    goto exit;
}

_0xC7:
{ /* RST 0x0000 */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
    gb->cpu_reg.pc = 0x0000;
    goto exit;
}

_0xC8:
{ /* RET Z */
    if (gb->cpu_reg.f_bits.z)
    {
        uint16_t temp = __gb_read_full(gb, gb->cpu_reg.sp++);
        temp |= __gb_read_full(gb, gb->cpu_reg.sp++) << 8;
        gb->cpu_reg.pc = temp;
        inst_cycles += 12;
    }

    goto exit;
}

_0xC9:
{ /* RET */
    uint16_t temp = __gb_read_full(gb, gb->cpu_reg.sp++);
    temp |= __gb_read_full(gb, gb->cpu_reg.sp++) << 8;
    gb->cpu_reg.pc = temp;
    goto exit;
}

_0xCA:
{ /* JP Z, imm */
    if (gb->cpu_reg.f_bits.z)
    {
        uint16_t temp = __gb_read_full(gb, gb->cpu_reg.pc++);
        temp |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
        gb->cpu_reg.pc = temp;
        inst_cycles += 4;
    }
    else
        gb->cpu_reg.pc += 2;

    goto exit;
}

_0xCB:
{ /* CB INST */
    inst_cycles = __gb_execute_cb(gb);
    goto exit;
}

_0xCC:
{ /* CALL Z, imm */
    if (gb->cpu_reg.f_bits.z)
    {
        uint16_t temp = __gb_read_full(gb, gb->cpu_reg.pc++);
        temp |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
        __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
        __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
        gb->cpu_reg.pc = temp;
        inst_cycles += 12;
    }
    else
        gb->cpu_reg.pc += 2;

    goto exit;
}

_0xCD:
{ /* CALL imm */
    uint16_t addr = __gb_read_full(gb, gb->cpu_reg.pc++);
    addr |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
    gb->cpu_reg.pc = addr;
    goto exit;
}

_0xCE:
{ /* ADC A, imm */
    uint8_t value, a, carry;
    value = __gb_read_full(gb, gb->cpu_reg.pc++);
    a = gb->cpu_reg.a;
    carry = gb->cpu_reg.f_bits.c;
    gb->cpu_reg.a = a + value + carry;

    gb->cpu_reg.f_bits.z = gb->cpu_reg.a == 0 ? 1 : 0;
    gb->cpu_reg.f_bits.h = ((a & 0xF) + (value & 0xF) + carry > 0x0F) ? 1 : 0;
    gb->cpu_reg.f_bits.c =
        (((uint16_t)a) + ((uint16_t)value) + carry > 0xFF) ? 1 : 0;
    gb->cpu_reg.f_bits.n = 0;
    goto exit;
}

_0xCF:
{ /* RST 0x0008 */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
    gb->cpu_reg.pc = 0x0008;
    goto exit;
}

_0xD0:
{ /* RET NC */
    if (!gb->cpu_reg.f_bits.c)
    {
        uint16_t temp = __gb_read_full(gb, gb->cpu_reg.sp++);
        temp |= __gb_read_full(gb, gb->cpu_reg.sp++) << 8;
        gb->cpu_reg.pc = temp;
        inst_cycles += 12;
    }

    goto exit;
}

_0xD1:
{ /* POP DE */
    gb->cpu_reg.e = __gb_read_full(gb, gb->cpu_reg.sp++);
    gb->cpu_reg.d = __gb_read_full(gb, gb->cpu_reg.sp++);
    goto exit;
}

_0xD2:
{ /* JP NC, imm */
    if (!gb->cpu_reg.f_bits.c)
    {
        uint16_t temp = __gb_read_full(gb, gb->cpu_reg.pc++);
        temp |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
        gb->cpu_reg.pc = temp;
        inst_cycles += 4;
    }
    else
        gb->cpu_reg.pc += 2;

    goto exit;
}

_0xD4:
{ /* CALL NC, imm */
    if (!gb->cpu_reg.f_bits.c)
    {
        uint16_t temp = __gb_read_full(gb, gb->cpu_reg.pc++);
        temp |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
        __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
        __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
        gb->cpu_reg.pc = temp;
        inst_cycles += 12;
    }
    else
        gb->cpu_reg.pc += 2;

    goto exit;
}

_0xD5:
{ /* PUSH DE */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.d);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.e);
    goto exit;
}

_0xD6:
{ /* SUB imm */
    uint8_t val = __gb_read_full(gb, gb->cpu_reg.pc++);
    uint16_t temp = gb->cpu_reg.a - val;
    gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = (gb->cpu_reg.a ^ val ^ temp) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp & 0xFF);
    goto exit;
}

_0xD7:
{ /* RST 0x0010 */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
    gb->cpu_reg.pc = 0x0010;
    goto exit;
}

_0xD8:
{ /* RET C */
    if (gb->cpu_reg.f_bits.c)
    {
        uint16_t temp = __gb_read_full(gb, gb->cpu_reg.sp++);
        temp |= __gb_read_full(gb, gb->cpu_reg.sp++) << 8;
        gb->cpu_reg.pc = temp;
        inst_cycles += 12;
    }

    goto exit;
}

_0xD9:
{ /* RETI */
    uint16_t temp = __gb_read_full(gb, gb->cpu_reg.sp++);
    temp |= __gb_read_full(gb, gb->cpu_reg.sp++) << 8;
    gb->cpu_reg.pc = temp;
    gb->gb_ime = 1;
    goto exit;
}

_0xDA:
{ /* JP C, imm */
    if (gb->cpu_reg.f_bits.c)
    {
        uint16_t addr = __gb_read_full(gb, gb->cpu_reg.pc++);
        addr |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
        gb->cpu_reg.pc = addr;
        inst_cycles += 4;
    }
    else
        gb->cpu_reg.pc += 2;

    goto exit;
}

_0xDC:
{ /* CALL C, imm */
    if (gb->cpu_reg.f_bits.c)
    {
        uint16_t temp = __gb_read_full(gb, gb->cpu_reg.pc++);
        temp |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
        __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
        __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
        gb->cpu_reg.pc = temp;
        inst_cycles += 12;
    }
    else
        gb->cpu_reg.pc += 2;

    goto exit;
}

_0xDE:
{ /* SBC A, imm */
    uint8_t temp_8 = __gb_read_full(gb, gb->cpu_reg.pc++);
    uint16_t temp_16 = gb->cpu_reg.a - temp_8 - gb->cpu_reg.f_bits.c;
    gb->cpu_reg.f_bits.z = ((temp_16 & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = (gb->cpu_reg.a ^ temp_8 ^ temp_16) & 0x10 ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp_16 & 0xFF00) ? 1 : 0;
    gb->cpu_reg.a = (temp_16 & 0xFF);
    goto exit;
}

_0xDF:
{ /* RST 0x0018 */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
    gb->cpu_reg.pc = 0x0018;
    goto exit;
}

_0xE0:
{ /* LD (0xFF00+imm), A */
    __gb_write_full(gb, 0xFF00 | __gb_read_full(gb, gb->cpu_reg.pc++),
                    gb->cpu_reg.a);
    goto exit;
}

_0xE1:
{ /* POP HL */
    gb->cpu_reg.l = __gb_read_full(gb, gb->cpu_reg.sp++);
    gb->cpu_reg.h = __gb_read_full(gb, gb->cpu_reg.sp++);
    goto exit;
}

_0xE2:
{ /* LD (C), A */
    __gb_write_full(gb, 0xFF00 | gb->cpu_reg.c, gb->cpu_reg.a);
    goto exit;
}

_0xE5:
{ /* PUSH HL */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.h);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.l);
    goto exit;
}

_0xE6:
{ /* AND imm */
    /* TODO: Optimisation? */
    gb->cpu_reg.a = gb->cpu_reg.a & __gb_read_full(gb, gb->cpu_reg.pc++);
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 1;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xE7:
{ /* RST 0x0020 */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
    gb->cpu_reg.pc = 0x0020;
    goto exit;
}

_0xE8:
{ /* ADD SP, imm */
    int8_t offset = (int8_t)__gb_read_full(gb, gb->cpu_reg.pc++);
    /* TODO: Move flag assignments for optimisation. */
    gb->cpu_reg.f_bits.z = 0;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        ((gb->cpu_reg.sp & 0xF) + (offset & 0xF) > 0xF) ? 1 : 0;
    gb->cpu_reg.f_bits.c = ((gb->cpu_reg.sp & 0xFF) + (offset & 0xFF) > 0xFF);
    gb->cpu_reg.sp += offset;
    goto exit;
}

_0xE9:
{ /* JP (HL) */
    gb->cpu_reg.pc = gb->cpu_reg.hl;
    goto exit;
}

_0xEA:
{ /* LD (imm), A */
    uint16_t addr = __gb_read_full(gb, gb->cpu_reg.pc++);
    addr |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
    __gb_write_full(gb, addr, gb->cpu_reg.a);
    goto exit;
}

_0xEE:
{ /* XOR imm */
    gb->cpu_reg.a = gb->cpu_reg.a ^ __gb_read_full(gb, gb->cpu_reg.pc++);
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xEF:
{ /* RST 0x0028 */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
    gb->cpu_reg.pc = 0x0028;
    goto exit;
}

_0xF0:
{ /* LD A, (0xFF00+imm) */
    gb->cpu_reg.a =
        __gb_read_full(gb, 0xFF00 | __gb_read_full(gb, gb->cpu_reg.pc++));
    goto exit;
}

_0xF1:
{ /* POP AF */
    uint8_t temp_8 = __gb_read_full(gb, gb->cpu_reg.sp++);
    gb->cpu_reg.f_bits.z = (temp_8 >> 7) & 1;
    gb->cpu_reg.f_bits.n = (temp_8 >> 6) & 1;
    gb->cpu_reg.f_bits.h = (temp_8 >> 5) & 1;
    gb->cpu_reg.f_bits.c = (temp_8 >> 4) & 1;
    gb->cpu_reg.a = __gb_read_full(gb, gb->cpu_reg.sp++);
    goto exit;
}

_0xF2:
{ /* LD A, (C) */
    gb->cpu_reg.a = __gb_read_full(gb, 0xFF00 | gb->cpu_reg.c);
    goto exit;
}

_0xF3:
{ /* DI */
    gb->gb_ime = 0;
    goto exit;
}

_0xF5:
{ /* PUSH AF */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.a);
    __gb_write_full(gb, --gb->cpu_reg.sp,
                    gb->cpu_reg.f_bits.z << 7 | gb->cpu_reg.f_bits.n << 6 |
                        gb->cpu_reg.f_bits.h << 5 | gb->cpu_reg.f_bits.c << 4);
    goto exit;
}

_0xF6:
{ /* OR imm */
    gb->cpu_reg.a = gb->cpu_reg.a | __gb_read_full(gb, gb->cpu_reg.pc++);
    gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0x00);
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = 0;
    gb->cpu_reg.f_bits.c = 0;
    goto exit;
}

_0xF7:
{ /* PUSH AF */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
    gb->cpu_reg.pc = 0x0030;
    goto exit;
}

_0xF8:
{ /* LD HL, SP+/-imm */
    /* Taken from SameBoy, which is released under MIT Licence. */
    int8_t offset = (int8_t)__gb_read_full(gb, gb->cpu_reg.pc++);
    gb->cpu_reg.hl = gb->cpu_reg.sp + offset;
    gb->cpu_reg.f_bits.z = 0;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h =
        ((gb->cpu_reg.sp & 0xF) + (offset & 0xF) > 0xF) ? 1 : 0;
    gb->cpu_reg.f_bits.c =
        ((gb->cpu_reg.sp & 0xFF) + (offset & 0xFF) > 0xFF) ? 1 : 0;
    goto exit;
}

_0xF9:
{ /* LD SP, HL */
    gb->cpu_reg.sp = gb->cpu_reg.hl;
    goto exit;
}

_0xFA:
{ /* LD A, (imm) */
    uint16_t addr = __gb_read_full(gb, gb->cpu_reg.pc++);
    addr |= __gb_read_full(gb, gb->cpu_reg.pc++) << 8;
    gb->cpu_reg.a = __gb_read_full(gb, addr);
    goto exit;
}

_0xFB:
{ /* EI */
    gb->gb_ime = 1;
    goto exit;
}

_0xFE:
{ /* CP imm */
    uint8_t temp_8 = __gb_read_full(gb, gb->cpu_reg.pc++);
    uint16_t temp_16 = gb->cpu_reg.a - temp_8;
    gb->cpu_reg.f_bits.z = ((temp_16 & 0xFF) == 0x00);
    gb->cpu_reg.f_bits.n = 1;
    gb->cpu_reg.f_bits.h = ((gb->cpu_reg.a ^ temp_8 ^ temp_16) & 0x10) ? 1 : 0;
    gb->cpu_reg.f_bits.c = (temp_16 & 0xFF00) ? 1 : 0;
    goto exit;
}

_0xFF:
{ /* RST 0x0038 */
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
    __gb_write_full(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);
    gb->cpu_reg.pc = 0x0038;
    goto exit;
}

_invalid:
{
    (gb->gb_error)(gb, GB_INVALID_OPCODE, opcode);
    // Early exit
    gb->gb_frame = 1;
}

exit:
    return inst_cycles;
}

__core static bool __gb_get_op_flag(struct gb_s *restrict gb, uint8_t op8)
{
    op8 %= 4;
    bool flag = (op8 <= 1) ? gb->cpu_reg.f_bits.z : gb->cpu_reg.f_bits.c;
    flag ^= (op8 % 2);
    return flag;
}

__core static u16 __gb_add16(struct gb_s *restrict gb, u16 a, u16 b)
{
    unsigned temp = a + b;
    gb->cpu_reg.f_bits.n = 0;
    gb->cpu_reg.f_bits.h = ((temp ^ a ^ b) >> 12) & 1;
    gb->cpu_reg.f_bits.c = temp >> 16;
    return temp;
}

__shell static u8 __gb_rare_instruction(struct gb_s *restrict gb,
                                        uint8_t opcode);

__core static unsigned __gb_run_instruction_micro(struct gb_s *gb)
{
#define FETCH8(gb) __gb_fetch8(gb)

#define FETCH16(gb) __gb_fetch16(gb)

    u8 opcode = FETCH8(gb);
    const u8 op8 = ((opcode & ~0xC0) / 8) ^ 1;
    float cycles = 1;  // use fpu register, save space
    unsigned src;
    u8 srcidx;

    switch (opcode >> 6)
    {
    case 0:
    {
        int reg8 = 2 * (opcode / 16) | (op8 & 1);  // i.e. b, c, d, e, ...
        int reg16 = reg8 / 2;                      // i.e. bc, de, hl...
        if (reg16 == 3)
            reg16 = 4;  // hack for SP
        switch (opcode % 16)
        {
        case 0:
        case 8:
            if (opcode == 0)
                break;  // nop
            if (opcode < 0x18)
                return __gb_rare_instruction(gb, opcode);
            {
                // jr
                cycles = 2;
                bool flag = __gb_get_op_flag(gb, op8);
                if (opcode == 0x18)
                    flag = 1;
                if (flag)
                {
                    cycles = 3;
                    gb->cpu_reg.pc += (s8)FETCH8(gb);
                }
                else
                {
                    gb->cpu_reg.pc++;
                }
            }
            break;
        case 1:
            // LD r16, d16
            cycles = 3;
            gb->cpu_reg_raw16[reg16] = FETCH16(gb);
            break;
        case 2:
        case 10:
            // TODO
            cycles = 2;
            if (reg16 == 4)
                reg16 = 2;

            if (op8 % 2 == 1)
            {
                // ld (r16), a
                __gb_write(gb, gb->cpu_reg_raw16[reg16], gb->cpu_reg.a);
            }
            else
            {
                // ld a, (r16)
                gb->cpu_reg.a = __gb_read(gb, gb->cpu_reg_raw16[reg16]);
            }

            goto inc_dec_hl;
            break;
        case 3:
        case 11:
        {
            // inc r16
            // dec r16
            s16 offset = (op8 % 2 == 1) ? 1 : -1;
            gb->cpu_reg_raw16[reg16] += offset;
            cycles = 2;
        }
        break;

        case 4:
        case 5:
        case 12:
        case 13:
        {
            // FIXME -- optimize?
            // inc r8
            // dec r8
            s8 offset = (opcode % 8 == 4) ? 1 : -1;
            u8 src = (reg8 == 7) ? __gb_read(gb, gb->cpu_reg.hl)
                                 : gb->cpu_reg_raw[reg8];
            u8 tmp = src + offset;
            gb->cpu_reg.f_bits.z = tmp == 0;
            if (offset == 1)
            {
                gb->cpu_reg.f_bits.n = 0;
                gb->cpu_reg.f_bits.h = (tmp & 0xF) == 0;
            }
            else
            {
                gb->cpu_reg.f_bits.n = 1;
                gb->cpu_reg.f_bits.h = (tmp & 0xF) == 0xF;
            }
            if (reg8 == 7)
            {
                cycles = 3;
                __gb_write(gb, gb->cpu_reg.hl, tmp);
            }
            else
            {
                gb->cpu_reg_raw[reg8] = tmp;
            }
        }
        break;

        case 6:
        case 14:
            srcidx = 0;
            src = FETCH8(gb);
            cycles = 2;
            goto ld_x_x;
            break;

        case 7:
        case 15:
            // misc flag ops
            if (opcode < 0x20)
            {
                // rlca
                // rrca
                // rla
                // rra
                u32 v = gb->cpu_reg.a << 8;
                if (op8 & 2)
                {
                    // carry bit will rotate into a
                    u32 c = gb->cpu_reg.f_bits.c;
                    v |= (c << 7) | (c << 16);
                }
                else
                {
                    // opposite bit will rotate into a
                    v = v | (v << 8);
                    v = v | (v >> 8);
                }
                if (op8 & 1)
                {
                    v <<= 1;
                }
                else
                {
                    v >>= 1;
                }
                gb->cpu_reg.f = 0;
                gb->cpu_reg.f_bits.c = (v >> (7 + 9 * (op8 & 1))) & 1;
                gb->cpu_reg.a = (v >> 8) & 0xFF;
            }
            else if unlikely (opcode == 0x27)
                return __gb_rare_instruction(gb, opcode);
            else if (opcode == 0x2F)
            {
                gb->cpu_reg.a ^= 0xFF;
                gb->cpu_reg.f_bits.n = 1;
                gb->cpu_reg.f_bits.h = 1;
            }
            else if (op8 % 2 == 1)
            {
                gb->cpu_reg.f_bits.c = 1;
                gb->cpu_reg.f_bits.n = 0;
                gb->cpu_reg.f_bits.h = 0;
            }
            else if (op8 % 2 == 0)
            {
                gb->cpu_reg.f_bits.c ^= 1;
                gb->cpu_reg.f_bits.n = 0;
                gb->cpu_reg.f_bits.h = 0;
            }
            break;

        case 9:
            // add hl, r16
            cycles = 2;
            gb->cpu_reg.hl =
                __gb_add16(gb, gb->cpu_reg.hl, gb->cpu_reg_raw16[reg16]);
            break;

        default:
            __builtin_unreachable();
        }
    }
    break;
    case 1:
    case 2:
    {
        srcidx = (opcode % 8) ^ 1;
        if (srcidx == 7)
        {
            src = __gb_read(gb, gb->cpu_reg.hl);
            cycles = 2;
        }
        else
            src = gb->cpu_reg_raw[srcidx];

        switch (opcode >> 6)
        {
        case 1:
            // LD x, x
        ld_x_x:
        {
            u8 dstidx = op8;
            if (dstidx == 7)
            {
                if (srcidx == 7)
                {
                    return __gb_rare_instruction(gb, opcode);
                }
                else
                {
                    cycles++;
                    __gb_write(gb, gb->cpu_reg.hl, src);
                }
            }
            else
            {
                gb->cpu_reg_raw[dstidx] = src;
            }
        }
        break;
        case 2:
        arithmetic:
            switch (op8)
            {
            case 0:  // ADC
            case 1:  // ADD
            case 2:  // SBC
            case 3:  // SUB
            case 6:  // CP
            {
                // carry bit
                unsigned v = src;
                if (op8 % 2 == 0 && op8 != 6)
                {
                    v += gb->cpu_reg.f_bits.c;
                }

                // subtraction
                gb->cpu_reg.f_bits.n = 0;
                if (op8 & 2)
                {
                    v = -v;
                    gb->cpu_reg.f_bits.n = 1;
                }

                // adder
                const u16 temp = gb->cpu_reg.a + v;
                gb->cpu_reg.f_bits.z = ((temp & 0xFF) == 0x00);
                gb->cpu_reg.f_bits.h = ((gb->cpu_reg.a ^ src ^ temp) >> 4) & 1;
                gb->cpu_reg.f_bits.c = temp >> 8;

                if (op8 != 6)
                {
                    gb->cpu_reg.a = temp & 0xFF;
                }
            }
            break;
            case 4:  // XOR
                gb->cpu_reg.a ^= src;
                gb->cpu_reg.f = 0;
                gb->cpu_reg.f_bits.z = gb->cpu_reg.a == 0;
                break;
            case 5:  // AND
                gb->cpu_reg.a &= src;
                gb->cpu_reg.f = 0;
                gb->cpu_reg.f_bits.h = 1;
                gb->cpu_reg.f_bits.z = gb->cpu_reg.a == 0;
                break;
            case 7:  // OR
                gb->cpu_reg.a |= src;
                gb->cpu_reg.f = 0;
                gb->cpu_reg.f_bits.z = gb->cpu_reg.a == 0;
                break;
            default:
                __builtin_unreachable();
            }
            break;
        }
    }
    break;
    case 3:
    {
        bool flag = __gb_get_op_flag(gb, op8);
        if (opcode % 8 == 3)
            flag = 1;
        switch ((opcode % 16) | ((opcode & 0x20) >> 1))
        {
        case 0x00:
        case 0x08:  // ret [flag]
            cycles = 2;
            if (flag)
            {
                goto ret;
            }
            break;
        case 0x01:
        case 0x11:  // pop
            cycles = 3;
            src = __gb_pop16(gb);
            if (op8 / 2 == 3)
            {
                gb->cpu_reg.a = src >> 8;
                gb->cpu_reg.f = src & 0xF0;
            }
            else
            {
                gb->cpu_reg_raw16[op8 / 2] = src;
            }
            break;
        case 0x02:
        case 0xA:  // jp [flag]
            cycles = 3;
            if (flag)
            {
                goto jp;
            }
            gb->cpu_reg.pc += 2;
            break;
        case 0x03:  // jp
            if unlikely (opcode == 0xD3)
            {
                return __gb_rare_instruction(gb, opcode);
            }
        jp:
            cycles = 4;
            gb->cpu_reg.pc = FETCH16(gb);
            break;
        case 0x04:
        case 0x0C:  // call [flag]
            cycles = 3;
            if (flag)
            {
                goto call;
            }
            gb->cpu_reg.pc += 2;
            break;
        case 0x05:
        case 0x15:  // push
            cycles = 4;
            src = gb->cpu_reg_raw16[op8 / 2];
            if (op8 / 2 == 3)
            {
                src = (gb->cpu_reg.a << 8) | (gb->cpu_reg.f & 0xF0);
            }
            __gb_push16(gb, src);
            break;
        case 0x06:
        case 0x0E:
        case 0x16:
        case 0x1E:  // arith d8
            cycles = 2;
            src = FETCH8(gb);
            goto arithmetic;
            break;
        case 0x07:
        case 0x0F:
        case 0x17:
        case 0x1F:  // rst
            cycles = 4;
            __gb_push16(gb, gb->cpu_reg.pc);
            gb->cpu_reg.pc = 8 * (op8 ^ 1);
            break;
        case 0x09:  // ret, reti
            if unlikely (opcode == 0xD9)
            {
                gb->gb_ime = 1;
            }
        ret:
            cycles += 3;
            gb->cpu_reg.pc = __gb_pop16(gb);
            break;
        case 0x0B:  // CB opcodes
            return __gb_execute_cb(gb);
            break;
        case 0x0D:  // call
            if unlikely (op8 & 2)
            {
                return __gb_rare_instruction(gb, opcode);
            }
        call:
            cycles = 6;
            {
                u16 tmp = FETCH16(gb);
                __gb_push16(gb, gb->cpu_reg.pc);
                gb->cpu_reg.pc = tmp;
            }
            break;
        case 0x10:  // ld (a8)
        case 0x12:  // ld (C)
        case 0x13:
        case 0x1B:  // di/ei
        case 0x14:
        case 0x1C:
        case 0x1D:  // illegal
        case 0x18:  // SP+8
        case 0x19:  // pc/sp hl
            return __gb_rare_instruction(gb, opcode);
            break;
        case 0x1A:  // ld (a16)
        {
            cycles = 4;
            u16 v = FETCH16(gb);
            if (op8 & 2)
            {
                gb->cpu_reg.a = __gb_read(gb, v);
            }
            else
            {
                __gb_write(gb, v, gb->cpu_reg.a);
            }
        }
        break;
        default:
            __builtin_unreachable();
        }
    }
    break;
    default:
        __builtin_unreachable();
    }

    if (false)
    {
    inc_dec_hl:
        gb->cpu_reg.hl += (opcode >= 0x20);
        gb->cpu_reg.hl -= 2 * (opcode >= 0x30);
    }
    return cycles * 4;
}

__shell static void __gb_interrupt(struct gb_s *gb)
{
    gb->gb_halt = 0;

    if (gb->gb_ime)
    {
        /* Disable interrupts */
        gb->gb_ime = 0;

        /* Push Program Counter */
        __gb_write(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc >> 8);
        __gb_write(gb, --gb->cpu_reg.sp, gb->cpu_reg.pc & 0xFF);

        /* Call interrupt handler if required. */
        if (gb->gb_reg.IF & gb->gb_reg.IE & VBLANK_INTR)
        {
            gb->cpu_reg.pc = VBLANK_INTR_ADDR;
            gb->gb_reg.IF ^= VBLANK_INTR;
        }
        else if (gb->gb_reg.IF & gb->gb_reg.IE & LCDC_INTR)
        {
            gb->cpu_reg.pc = LCDC_INTR_ADDR;
            gb->gb_reg.IF ^= LCDC_INTR;
        }
        else if (gb->gb_reg.IF & gb->gb_reg.IE & TIMER_INTR)
        {
            gb->cpu_reg.pc = TIMER_INTR_ADDR;
            gb->gb_reg.IF ^= TIMER_INTR;
        }
        else if (gb->gb_reg.IF & gb->gb_reg.IE & SERIAL_INTR)
        {
            gb->cpu_reg.pc = SERIAL_INTR_ADDR;
            gb->gb_reg.IF ^= SERIAL_INTR;
        }
        else if (gb->gb_reg.IF & gb->gb_reg.IE & CONTROL_INTR)
        {
            gb->cpu_reg.pc = CONTROL_INTR_ADDR;
            gb->gb_reg.IF ^= CONTROL_INTR;
        }
    }
}

__shell static uint16_t __gb_calc_halt_cycles(struct gb_s *gb)
{
    int src[] = {512, 512, 512};

#if 0
    // TODO: optimize serial
    if(gb->gb_reg.SC & SERIAL_SC_TX_START) return 16;
#endif

    if (gb->gb_reg.tac_enable)
    {
        src[1] = gb->gb_reg.tac_cycles + 1 - gb->counter.tima_count +
                 ((0x100 - gb->gb_reg.TIMA) << gb->gb_reg.tac_cycles_shift);
    }

    src[2] = LCD_LINE_CYCLES - gb->counter.lcd_count;
    if (gb->lcd_mode == LCD_HBLANK)
    {
        src[2] = LCD_MODE_2_CYCLES - gb->counter.lcd_count;
    }
    else if (gb->lcd_mode == LCD_SEARCH_OAM)
    {
        src[2] = LCD_MODE_3_CYCLES - gb->counter.lcd_count;
    }

    // return max{16, min(src...)}
    int cycles = src[0];
    if (src[1] < cycles)
        cycles = src[1];
    if (src[2] < cycles)
        cycles = src[2];

    // ensure positive
    cycles = (cycles < 16) ? 16 : cycles;

    return cycles;
}

__core static unsigned __gb_tima_overflow(struct gb_s *gb, unsigned tima)
{
    gb->gb_reg.IF |= TIMER_INTR;
    tima -= 0x100;
    unsigned div = 0x100 - (unsigned)gb->gb_reg.TMA;
    tima %= div;
    tima += (unsigned)gb->gb_reg.TMA;
    return tima;
}

/**
 * Internal function used to step the CPU.
 */
__core void __gb_step_cpu(struct gb_s *gb)
{
    unsigned inst_cycles = 16;

    /* Handle interrupts */
    if unlikely ((gb->gb_ime || gb->gb_halt) &&
                 (gb->gb_reg.IF & gb->gb_reg.IE & ANY_INTR))
    {
        __gb_interrupt(gb);
    }

    if unlikely (gb->gb_halt)
    {
        inst_cycles = __gb_calc_halt_cycles(gb);
        goto done_instr;
    }

#ifndef CPU_VALIDATE

    inst_cycles = __gb_run_instruction_micro(gb);
#else
    // run once as each, verify

    static u8 _wram[2][WRAM_SIZE];
    static u8 _vram[2][VRAM_SIZE];
    static struct gb_s _gb[2];

    memcpy(_wram[0], gb->wram, WRAM_SIZE);
    memcpy(_vram[0], gb->vram, VRAM_SIZE);
    memcpy(&_gb[0], gb, sizeof(_gb));

    uint8_t opcode = (gb->gb_halt ? 0 : __gb_fetch8(gb));
    inst_cycles = __gb_run_instruction(gb, opcode);

    gb->cpu_reg.f_bits.unused = 0;

    memcpy(_wram[1], gb->wram, WRAM_SIZE);
    memcpy(_vram[1], gb->vram, VRAM_SIZE);
    memcpy(&_gb[1], gb, sizeof(struct gb_s));

    memcpy(gb->wram, _wram[0], WRAM_SIZE);
    memcpy(gb->vram, _vram[0], VRAM_SIZE);
    memcpy(gb, &_gb[0], sizeof(struct gb_s));

    uint8_t inst_cycles_m = __gb_run_instruction_micro(gb);

    gb->cpu_reg.f_bits.unused = 0;

    if (memcmp(gb->wram, _wram[1], WRAM_SIZE))
    {
        gb->gb_frame = 1;
        playdate->system->error("difference in wram on opcode %x", opcode);
    }
    if (memcmp(gb->vram, _vram[1], VRAM_SIZE))
    {
        gb->gb_frame = 1;
        playdate->system->error("difference in vram on opcode %x", opcode);
    }

    if (memcmp(&gb->cpu_reg, &_gb[1].cpu_reg, sizeof(struct cpu_registers_s)))
    {
        gb->gb_frame = 1;
        playdate->system->error("difference in CPU regs on opcode %x", opcode);
        if (gb->cpu_reg.af != _gb[1].cpu_reg.af)
        {
            playdate->system->error("AF, was %x, expected %x", gb->cpu_reg.af,
                                    _gb[1].cpu_reg.af);
        }
        if (gb->cpu_reg.bc != _gb[1].cpu_reg.bc)
        {
            playdate->system->error("BC, was %x, expected %x", gb->cpu_reg.bc,
                                    _gb[1].cpu_reg.bc);
        }
        if (gb->cpu_reg.de != _gb[1].cpu_reg.de)
        {
            playdate->system->error("DE, was %x, expected %x", gb->cpu_reg.de,
                                    _gb[1].cpu_reg.de);
        }
        if (gb->cpu_reg.hl != _gb[1].cpu_reg.hl)
        {
            playdate->system->error("HL, was %x, expected %x", gb->cpu_reg.hl,
                                    _gb[1].cpu_reg.hl);
        }
        if (gb->cpu_reg.sp != _gb[1].cpu_reg.sp)
        {
            playdate->system->error("SP, was %x, expected %x", gb->cpu_reg.sp,
                                    _gb[1].cpu_reg.sp);
        }
        if (gb->cpu_reg.pc != _gb[1].cpu_reg.pc)
        {
            playdate->system->error("PC, was %x, expected %x", gb->cpu_reg.pc,
                                    _gb[1].cpu_reg.pc);
        }
    }
    else if (memcmp(gb, &_gb[1], sizeof(struct gb_s)))
    {
        gb->gb_frame = 1;
        playdate->system->error("difference in gb struct on opcode %x", opcode);
    }

    if (inst_cycles != inst_cycles_m)
    {
        gb->gb_frame = 1;
        playdate->system->error(
            "cycle difference on opcode %x (expected %d, was %d)", opcode,
            inst_cycles, inst_cycles_m);
    }
#endif

done_instr:
{
#if 0
        /* Check serial transmission. */
        if(gb->gb_reg.SC & SERIAL_SC_TX_START)
        {
            /* If new transfer, call TX function. */
            if(gb->counter.serial_count == 0 && gb->gb_serial_tx != NULL)
                (gb->gb_serial_tx)(gb, gb->gb_reg.SB);

            gb->counter.serial_count += inst_cycles;

            /* If it's time to receive byte, call RX function. */
            if(gb->counter.serial_count >= SERIAL_CYCLES)
            {
                /* If RX can be done, do it. */
                /* If RX failed, do not change SB if using external
                 * clock, or set to 0xFF if using internal clock. */
                uint8_t rx;

                if(gb->gb_serial_rx != NULL &&
                   (gb->gb_serial_rx(gb, &rx) ==
                    GB_SERIAL_RX_SUCCESS))
                {
                    gb->gb_reg.SB = rx;

                    /* Inform game of serial TX/RX completion. */
                    gb->gb_reg.SC &= 0x01;
                    gb->gb_reg.IF |= SERIAL_INTR;
                }
                else if(gb->gb_reg.SC & SERIAL_SC_CLOCK_SRC)
                {
                    /* If using internal clock, and console is not
                     * attached to any external peripheral, shifted
                     * bits are replaced with logic 1. */
                    gb->gb_reg.SB = 0xFF;

                    /* Inform game of serial TX/RX completion. */
                    gb->gb_reg.SC &= 0x01;
                    gb->gb_reg.IF |= SERIAL_INTR;
                }
                else
                {
                    /* If using external clock, and console is not
                     * attached to any external peripheral, bits are
                     * not shifted, so SB is not modified. */
                }

                gb->counter.serial_count = 0;
            }
        }
#endif

    /* TIMA register timing */
    /* TODO: Change tac_enable to struct of TAC timer control bits. */
    if (gb->gb_reg.tac_enable)
    {
        gb->counter.tima_count += inst_cycles;
        unsigned tima = (unsigned)gb->gb_reg.TIMA +
                        (gb->counter.tima_count >> gb->gb_reg.tac_cycles_shift);
        gb->counter.tima_count &= gb->gb_reg.tac_cycles;
        if (tima >= 0x100)
        {
            tima = __gb_tima_overflow(gb, tima);
        }
        gb->gb_reg.TIMA = tima;
    }

    /* DIV register timing */
    // update DIV timer
    gb->counter.div_count += inst_cycles;
    gb->gb_reg.DIV += gb->counter.div_count / DIV_CYCLES;
    gb->counter.div_count %= DIV_CYCLES;

    /* TODO Check behaviour of LCD during LCD power off state. */
    /* If LCD is off, don't update LCD state. */
    if ((gb->gb_reg.LCDC & LCDC_ENABLE) == 0)
        return;

    /* LCD Timing */
    gb->counter.lcd_count += inst_cycles;

    /* New Scanline */
    if (gb->counter.lcd_count > LCD_LINE_CYCLES)
    {
        gb->counter.lcd_count -= LCD_LINE_CYCLES;

        /* LYC Update */
        if (gb->gb_reg.LY == gb->gb_reg.LYC)
        {
            gb->gb_reg.STAT |= STAT_LYC_COINC;

            if (gb->gb_reg.STAT & STAT_LYC_INTR)
                gb->gb_reg.IF |= LCDC_INTR;
        }
        else
            gb->gb_reg.STAT &= 0xFB;

        /* Next line */
        uint16_t LY_1 = gb->gb_reg.LY + 1;
        gb->gb_reg.LY = (LY_1 >= LCD_VERT_LINES) ? LY_1 - LCD_VERT_LINES : LY_1;

        /* VBLANK Start */
        if (gb->gb_reg.LY == LCD_HEIGHT)
        {
            gb->lcd_mode = LCD_VBLANK;
            gb->gb_frame = 1;
            gb->gb_reg.IF |= VBLANK_INTR;
            gb->lcd_blank = 0;

            if (gb->gb_reg.STAT & STAT_MODE_1_INTR)
                gb->gb_reg.IF |= LCDC_INTR;

#if ENABLE_LCD

            /* If frame skip is activated, check if we need to draw
             * the frame or skip it. */
            if (gb->direct.frame_skip)
            {
                gb->display.frame_skip_count = !gb->display.frame_skip_count;
            }

            if (!gb->direct.frame_skip || !gb->display.frame_skip_count)
            {
                gb->display.back_fb_enabled = !gb->display.back_fb_enabled;
            }
#endif
        }
        /* Normal Line */
        else if (gb->gb_reg.LY < LCD_HEIGHT)
        {
            if (gb->gb_reg.LY == 0)
            {
                /* Clear Screen */
                gb->display.WY = gb->gb_reg.WY;
                gb->display.window_clear = 0;
            }

            gb->lcd_mode = LCD_HBLANK;

            if (gb->gb_reg.STAT & STAT_MODE_0_INTR)
                gb->gb_reg.IF |= LCDC_INTR;
        }
    }
    /* OAM access */
    else if (gb->lcd_mode == LCD_HBLANK &&
             gb->counter.lcd_count >= LCD_MODE_2_CYCLES)
    {
        gb->lcd_mode = LCD_SEARCH_OAM;

        if (gb->gb_reg.STAT & STAT_MODE_2_INTR)
            gb->gb_reg.IF |= LCDC_INTR;
    }
    /* Update LCD */
    else if (gb->lcd_mode == LCD_SEARCH_OAM &&
             gb->counter.lcd_count >= LCD_MODE_3_CYCLES)
    {
        gb->lcd_mode = LCD_TRANSFER;
#if ENABLE_LCD
        if (gb->lcd_master_enable && !gb->lcd_blank &&
            !(gb->direct.frame_skip && !gb->display.frame_skip_count))
            __gb_draw_line(gb);
#endif
    }
}
}

__core void gb_run_frame(struct gb_s *gb)
{
    gb->gb_frame = 0;

    while (!gb->gb_frame)
    {
        __gb_step_cpu(gb);
    }
}

/**
 * Gets the size of the save file required for the ROM.
 */
uint_fast32_t gb_get_save_size(struct gb_s *gb)
{
    const uint_fast16_t ram_size_location = 0x0149;
    const uint_fast32_t ram_sizes[] = {0x00, 0x800, 0x2000, 0x8000, 0x20000};
    uint8_t ram_size = gb->gb_rom[ram_size_location];
    return ram_sizes[ram_size];
}

/**
 * Set the function used to handle serial transfer in the front-end. This is
 * optional.
 * gb_serial_transfer takes a byte to transmit and returns the received byte. If
 * no cable is connected to the console, return 0xFF.
 */
void gb_init_serial(struct gb_s *gb,
                    void (*gb_serial_tx)(struct gb_s *, const uint8_t),
                    enum gb_serial_rx_ret_e (*gb_serial_rx)(struct gb_s *,
                                                            uint8_t *))
{
    gb->gb_serial_tx = gb_serial_tx;
    gb->gb_serial_rx = gb_serial_rx;
}

uint8_t gb_colour_hash(struct gb_s *gb)
{
#define ROM_TITLE_START_ADDR 0x0134
#define ROM_TITLE_END_ADDR 0x0143

    uint8_t x = 0;

    for (uint16_t i = ROM_TITLE_START_ADDR; i <= ROM_TITLE_END_ADDR; i++)
        x += gb->gb_rom[i];

    return x;
}

/**
 * Resets the context, and initialises startup values.
 */
void gb_reset(struct gb_s *gb)
{
    gb->gb_halt = 0;
    gb->gb_ime = 1;
    gb->gb_bios_enable = 0;
    gb->lcd_mode = LCD_HBLANK;

    /* Initialise MBC values. */
    gb->selected_rom_bank = 1;
    gb->cart_ram_bank = 0;
    gb->enable_cart_ram = 0;
    gb->cart_mode_select = 0;
    __gb_update_selected_bank_addr(gb);

    /* Initialise CPU registers as though a DMG. */
    gb->cpu_reg.af = 0x01B0;
    gb->cpu_reg.bc = 0x0013;
    gb->cpu_reg.de = 0x00D8;
    gb->cpu_reg.hl = 0x014D;
    gb->cpu_reg.sp = 0xFFFE;
    /* TODO: Add BIOS support. */
    gb->cpu_reg.pc = 0x0100;

    gb->counter.lcd_count = 0;
    gb->counter.div_count = 0;
    gb->counter.tima_count = 0;
    gb->counter.serial_count = 0;

    gb->gb_reg.TIMA = 0x00;
    gb->gb_reg.TMA = 0x00;
    gb->gb_reg.TAC = 0xF8;
    gb->gb_reg.DIV = 0xAC;

    __gb_update_tac(gb);

    gb->gb_reg.IF = 0xE1;

    gb->gb_reg.LCDC = 0x91;
    gb->gb_reg.SCY = 0x00;
    gb->gb_reg.SCX = 0x00;
    gb->gb_reg.LYC = 0x00;

    /* Appease valgrind for invalid reads and unconditional jumps. */
    gb->gb_reg.SC = 0x7E;
    gb->gb_reg.STAT = 0;
    gb->gb_reg.LY = 0;

    __gb_write(gb, 0xFF47, 0xFC);  // BGP
    __gb_write(gb, 0xFF48, 0xFF);  // OBJP0
    __gb_write(gb, 0xFF49, 0x0F);  // OBJP1
    gb->gb_reg.WY = 0x00;
    gb->gb_reg.WX = 0x00;
    gb->gb_reg.IE = 0x00;

    gb->direct.joypad = 0xFF;
    gb->gb_reg.P1 = 0xCF;

    memset(gb->vram, 0x00, VRAM_SIZE);
    memset(gb->wram, 0x00, WRAM_SIZE);
}

/**
 * Initialise the emulator context. gb_reset() is also called to initialise
 * the CPU.
 */
enum gb_init_error_e gb_init(struct gb_s *gb, uint8_t *wram, uint8_t *vram,
                             uint8_t *lcd, uint8_t *gb_rom,
                             void (*gb_error)(struct gb_s *,
                                              const enum gb_error_e,
                                              const uint16_t),
                             void *priv)
{
    const uint16_t mbc_location = 0x0147;
    const uint16_t bank_count_location = 0x0148;
    const uint16_t ram_size_location = 0x0149;
    /**
     * Table for cartridge type (MBC). -1 if invalid.
     * TODO: MMM01 is untested.
     * TODO: MBC6 is untested.
     * TODO: MBC7 is unsupported.
     * TODO: POCKET CAMERA is unsupported.
     * TODO: BANDAI TAMA5 is unsupported.
     * TODO: HuC3 is unsupported.
     * TODO: HuC1 is unsupported.
     **/
    /* clang-format off */
    const uint8_t cart_mbc[] =
    {
        0, 1, 1, 1, -1,  2,  2, -1,  0, 0, -1, 0, 0, 0, -1,  3,
        3, 3, 3, 3, -1, -1, -1, -1, -1, 5,  5, 5, 5, 5,  5, -1
    };
    const uint8_t cart_ram[] =
    {
        0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
        1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0
    };
    const uint16_t num_rom_banks_mask[] =
    {
        2, 4, 8, 16, 32, 64, 128, 256, 512
    };
    const uint8_t num_ram_banks[] =
    {
        0, 1, 1, 4, 16, 8
    };
    /* clang-format on */

    gb->wram = wram;
    gb->vram = vram;
    gb->lcd = lcd;
    gb->gb_rom = gb_rom;
    gb->gb_error = gb_error;
    gb->direct.priv = priv;

    /* Initialise serial transfer function to NULL. If the front-end does
     * not provide serial support, Peanut-GB will emulate no cable connected
     * automatically. */
    gb->gb_serial_tx = NULL;
    gb->gb_serial_rx = NULL;

    /* Check valid ROM using checksum value. */
    {
        uint8_t x = 0;

        for (uint16_t i = 0x0134; i <= 0x014C; i++)
            x = x - gb->gb_rom[i] - 1;

        if (x != gb->gb_rom[ROM_HEADER_CHECKSUM_LOC])
            return GB_INIT_INVALID_CHECKSUM;
    }

    /* Check if cartridge type is supported, and set MBC type. */
    {
        const uint8_t mbc_value = gb->gb_rom[mbc_location];

        if (mbc_value > sizeof(cart_mbc) - 1 ||
            (gb->mbc = cart_mbc[mbc_value]) == 255u)
            return GB_INIT_CARTRIDGE_UNSUPPORTED;
    }

    gb->cart_ram = cart_ram[gb->gb_rom[mbc_location]];
    gb->num_rom_banks_mask =
        num_rom_banks_mask[gb->gb_rom[bank_count_location]] - 1;
    gb->num_ram_banks = num_ram_banks[gb->gb_rom[ram_size_location]];

    gb->lcd_blank = 0;

    gb->direct.sound = ENABLE_SOUND;

    gb_reset(gb);

    return GB_INIT_NO_ERROR;
}

/**
 * Returns the title of ROM.
 *
 * \param gb        Initialised context.
 * \param title_str Allocated string at least 16 characters.
 * \returns         Pointer to start of string, null terminated.
 */
const char *gb_get_rom_name(struct gb_s *gb, char *title_str)
{
    uint_fast16_t title_loc = 0x134;
    /* End of title may be 0x13E for newer games. */
    const uint_fast16_t title_end = 0x143;
    const char *title_start = title_str;

    for (; title_loc <= title_end; title_loc++)
    {
        const char title_char = gb->gb_rom[title_loc];

        if (title_char >= ' ' && title_char <= '_')
        {
            *title_str = title_char;
            title_str++;
        }
        else
            break;
    }

    *title_str = '\0';
    return title_start;
}

#if ENABLE_LCD

void gb_init_lcd(struct gb_s *gb)
{
    gb->direct.frame_skip = 0;
    gb->display.frame_skip_count = 0;

    gb->display.back_fb_enabled = 0;

    gb->display.window_clear = 0;
    gb->display.WY = 0;
    gb->lcd_master_enable = 1;

    return;
}

#else

void gb_init_lcd(struct gb_s *gb)
{
}

#endif

__shell static u8 __gb_rare_instruction(struct gb_s *restrict gb,
                                        uint8_t opcode)
{
    switch (opcode)
    {
    case 0x08:  // ld (a16), SP
        __gb_write16(gb, __gb_fetch16(gb), gb->cpu_reg.sp);
        return 5 * 4;
    case 0x10:  // stop
        gb->gb_ime = 0;
        gb->gb_halt = 1;
        playdate->system->logToConsole("'stop' instr");
        return 1 * 4;
    case 0x27:  // daa
    {
        uint16_t a = gb->cpu_reg.a;

        if (gb->cpu_reg.f_bits.n)
        {
            if (gb->cpu_reg.f_bits.h)
                a = (a - 0x06) & 0xFF;

            if (gb->cpu_reg.f_bits.c)
                a -= 0x60;
        }
        else
        {
            if (gb->cpu_reg.f_bits.h || (a & 0x0F) > 9)
                a += 0x06;

            if (gb->cpu_reg.f_bits.c || a > 0x9F)
                a += 0x60;
        }

        if ((a & 0x100) == 0x100)
            gb->cpu_reg.f_bits.c = 1;

        gb->cpu_reg.a = a;
        gb->cpu_reg.f_bits.z = (gb->cpu_reg.a == 0);
        gb->cpu_reg.f_bits.h = 0;
    }
        return 1 * 4;
    case 0x76:  // halt
    {
        gb->gb_halt = 1;
    }
        return 1 * 4;
    case 0xE0:
        __gb_write(gb, 0xFF00 | __gb_read(gb, gb->cpu_reg.pc++), gb->cpu_reg.a);
        return 3 * 4;
    case 0xE2:
        __gb_write(gb, 0xFF00 | gb->cpu_reg.c, gb->cpu_reg.a);
        return 2 * 4;
    case 0xE8:
    {
        int16_t offset = (int8_t)__gb_read(gb, gb->cpu_reg.pc++);
        gb->cpu_reg.f = 0;
        gb->cpu_reg.sp = __gb_add16(gb, gb->cpu_reg.sp, offset);
    }
        return 4 * 4;
    case 0xE9:
        gb->cpu_reg.pc = gb->cpu_reg.hl;
        return 4;
    case 0xF0:
        gb->cpu_reg.a = __gb_read(gb, 0xFF00 | __gb_read(gb, gb->cpu_reg.pc++));
        return 3 * 4;
    case 0xF2:
        gb->cpu_reg.a = __gb_read(gb, 0xFF00 | gb->cpu_reg.c);
        return 2 * 4;
    case 0xF3:
        gb->gb_ime = 0;
        return 1 * 4;
    case 0xF8:
    {
        int16_t offset = (int8_t)__gb_read(gb, gb->cpu_reg.pc++);
        gb->cpu_reg.f = 0;
        gb->cpu_reg.hl = __gb_add16(gb, gb->cpu_reg.sp, offset);
        return 3 * 4;
    }
        return 3 * 4;
    case 0xF9:
        gb->cpu_reg.sp = gb->cpu_reg.hl;
        return 2 * 4;
    case 0xFB:
        gb->gb_ime = 1;
        return 1 * 4;
    default:
        (gb->gb_error)(gb, GB_INVALID_OPCODE, opcode);
        gb->gb_frame = 1;
        return 1 * 4;  // ?
    }
}

#endif  // PEANUT_GB_H
