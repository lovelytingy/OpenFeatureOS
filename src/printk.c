/*
 * OpenFeatureOS - Lightweight Kernel Printf Implementation
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Implemented cleanly in C99 without heap allocations or standard library dependencies.
 */

#include "printk.h"
#include "t107_regs.h"

/* src: Nokia-2660-RE docs/uart-serial-interface.md */
void kputc(char c)
{
    /* Poll until TX FIFO is not full with timeout protection (never hang if UART unplugged) */
    uint32_t timeout = 50000U;
    while (((UART1_STS1 & 0xFF00U) >= 0x1000U) && --timeout) {
        /* Busy wait with timeout */
    }
    UART1_TXD = (uint32_t)c;
}

void kputs(const char *s)
{
    if (!s) {
        s = "(null)";
    }
    while (*s) {
        if (*s == '\n') {
            kputc('\r');
        }
        kputc(*s++);
    }
}

static void print_num(uint32_t num, unsigned base, int uppercase, int min_width, char pad_char)
{
    char buf[32];
    int i = 0;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (num == 0) {
        buf[i++] = '0';
    } else {
        while (num > 0) {
            buf[i++] = digits[num % base];
            num /= base;
        }
    }

    while (i < min_width && i < (int)(sizeof(buf) - 1)) {
        buf[i++] = pad_char;
    }

    while (i > 0) {
        kputc(buf[--i]);
    }
}

int kvprintf(const char *fmt, va_list ap)
{
    int count = 0;

    while (*fmt) {
        if (*fmt != '%') {
            if (*fmt == '\n') {
                kputc('\r');
                count++;
            }
            kputc(*fmt++);
            count++;
            continue;
        }

        fmt++; /* Skip '%' */

        /* Zero padding & width flag */
        char pad_char = ' ';
        int width = 0;

        if (*fmt == '0') {
            pad_char = '0';
            fmt++;
        }

        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            kputs(s);
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            kputc(c);
            count++;
            break;
        }
        case 'd':
        case 'i': {
            int32_t d = va_arg(ap, int32_t);
            if (d < 0) {
                kputc('-');
                count++;
                d = -d;
            }
            print_num((uint32_t)d, 10, 0, width, pad_char);
            break;
        }
        case 'u': {
            uint32_t u = va_arg(ap, uint32_t);
            print_num(u, 10, 0, width, pad_char);
            break;
        }
        case 'x': {
            uint32_t x = va_arg(ap, uint32_t);
            print_num(x, 16, 0, width, pad_char);
            break;
        }
        case 'X': {
            uint32_t x = va_arg(ap, uint32_t);
            print_num(x, 16, 1, width, pad_char);
            break;
        }
        case 'p': {
            uint32_t p = (uint32_t)va_arg(ap, uintptr_t);
            kputs("0x");
            print_num(p, 16, 0, 8, '0');
            break;
        }
        case '%': {
            kputc('%');
            count++;
            break;
        }
        default:
            kputc('%');
            kputc(*fmt);
            count += 2;
            break;
        }

        if (*fmt) {
            fmt++;
        }
    }

    return count;
}

int kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = kvprintf(fmt, ap);
    va_end(ap);
    return ret;
}
