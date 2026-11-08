/*
 * OpenFeatureOS - RT-Thread libcpu Port for ARM Cortex-A7 (Unisoc T107)
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Sourced from:
 *   - RT-Thread libcpu/arm/cortex-a/stack.c
 *   - RT-Thread libcpu/arm/cortex-a/cpuport.c
 */

#include <rtthread.h>
#include <rthw.h>
#include "cpuport.h"

/* Context switch tracking variables used by irq_handler in startup.S */
volatile rt_ubase_t rt_thread_switch_interrupt_flag = 0;
volatile rt_ubase_t rt_interrupt_from_thread = 0;
volatile rt_ubase_t rt_interrupt_to_thread = 0;

/**
 * @brief Initialize thread execution context stack frame.
 *
 * Stack frame layout (high memory to low memory, 16 32-bit registers = 64 bytes):
 *   [sp + 60]: PC (_thread_start trampoline)
 *   [sp + 56]: LR (texit / rt_thread_exit)
 *   [sp + 52]: R12
 *   [sp + 48]: R11
 *   [sp + 44]: R10
 *   [sp + 40]: R9
 *   [sp + 36]: R8
 *   [sp + 32]: R7
 *   [sp + 28]: R6
 *   [sp + 24]: R5
 *   [sp + 20]: R4
 *   [sp + 16]: R3
 *   [sp + 12]: R2
 *   [sp + 8] : R1 (tentry function pointer)
 *   [sp + 4] : R0 (parameter)
 *   [sp + 0] : CPSR (SVCMODE, IRQ/FIQ unmasked)
 */
rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter,
                             rt_uint8_t *stack_addr, void *texit)
{
    rt_uint32_t *stk;

    /* AAPCS requirement: 8-byte stack alignment */
    stack_addr += sizeof(rt_uint32_t);
    stack_addr  = (rt_uint8_t *)RT_ALIGN_DOWN((rt_uint32_t)stack_addr, 8);
    stk         = (rt_uint32_t *)stack_addr;

    *(--stk) = (rt_uint32_t)_thread_start;  /* Initial PC: entry trampoline */
    *(--stk) = (rt_uint32_t)texit;          /* LR: thread exit trampoline */
    *(--stk) = 0x00000012U;                 /* R12 */
    *(--stk) = 0x00000011U;                 /* R11 */
    *(--stk) = 0x00000010U;                 /* R10 */
    *(--stk) = 0x00000009U;                 /* R9 */
    *(--stk) = 0x00000008U;                 /* R8 */
    *(--stk) = 0x00000007U;                 /* R7 */
    *(--stk) = 0x00000006U;                 /* R6 */
    *(--stk) = 0x00000005U;                 /* R5 */
    *(--stk) = 0x00000004U;                 /* R4 */
    *(--stk) = 0x00000003U;                 /* R3 */
    *(--stk) = 0x00000002U;                 /* R2 */
    *(--stk) = (rt_uint32_t)tentry;         /* R1: thread entry function */
    *(--stk) = (rt_uint32_t)parameter;      /* R0: thread parameter */

    /* CPSR: Supervisor mode (0x13), IRQs and FIQs enabled */
    if ((rt_uint32_t)tentry & 0x01U) {
        *(--stk) = SVCMODE | T_Bit;         /* Thumb mode entry */
    } else {
        *(--stk) = SVCMODE;                 /* ARM mode entry */
    }

    return (rt_uint8_t *)stk;
}

void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to,
                                   rt_thread_t from_thread, rt_thread_t to_thread)
{
    (void)from_thread;
    (void)to_thread;

    if (rt_thread_switch_interrupt_flag == 0) {
        rt_thread_switch_interrupt_flag = 1;
        rt_interrupt_from_thread = from;
    }
    rt_interrupt_to_thread = to;
}

void rt_hw_cpu_reset(void)
{
    while (1) {
        __asm__ volatile ("wfi");
    }
}

void rt_hw_cpu_shutdown(void)
{
    while (1) {
        __asm__ volatile ("wfi");
    }
}

const char *rt_hw_cpu_arch(void)
{
    return "Cortex-A7";
}

/* Standard freestanding C memory primitives mapped to RT-Thread klibc */
void *memset(void *s, int c, size_t n)
{
    return rt_memset(s, c, n);
}

void *memcpy(void *dest, const void *src, size_t n)
{
    return rt_memcpy(dest, src, n);
}

void *memmove(void *dest, const void *src, size_t n)
{
    return rt_memmove(dest, src, n);
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    return rt_memcmp(s1, s2, n);
}

size_t strlen(const char *s)
{
    return rt_strlen(s);
}

char *strcpy(char *dest, const char *src)
{
    return rt_strcpy(dest, src);
}

char *strncpy(char *dest, const char *src, size_t n)
{
    return rt_strncpy(dest, src, n);
}

char *strcat(char *dest, const char *src)
{
    char *p = dest + rt_strlen(dest);
    while (*src) {
        *p++ = *src++;
    }
    *p = '\0';
    return dest;
}

int strcmp(const char *s1, const char *s2)
{
    return rt_strcmp(s1, s2);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    return rt_strncmp(s1, s2, n);
}

char *strchr(const char *s, int c)
{
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return (c == 0) ? (char *)s : NULL;
}

char *strstr(const char *haystack, const char *needle)
{
    return rt_strstr(haystack, needle);
}

void *malloc(size_t size)
{
    return rt_malloc(size);
}

void free(void *ptr)
{
    rt_free(ptr);
}

void *realloc(void *ptr, size_t size)
{
    return rt_realloc(ptr, size);
}

int abs(int j)
{
    return (j < 0) ? -j : j;
}

long labs(long j)
{
    return (j < 0) ? -j : j;
}

int rt_vsscanf(const char *buf, const char *fmt, va_list args)
{
    (void)buf;
    (void)fmt;
    (void)args;
    return 0;
}
