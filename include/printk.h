/*
 * OpenFeatureOS - Kernel Printf & Console Header
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#ifndef PRINTK_H_
#define PRINTK_H_

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

void kputc(char c);
void kputs(const char *s);
int  kprintf(const char *fmt, ...);
int  kvprintf(const char *fmt, va_list ap);

#endif /* PRINTK_H_ */
