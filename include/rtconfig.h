/*
 * OpenFeatureOS - RT-Thread Kernel Configuration for Unisoc T107 (Cortex-A7)
 * Target: Unisoc T107 (UMS9117) ARM Cortex-A7 (Nokia 2660 Flip TA-1480)
 *
 * Sourced from:
 *   - RT-Thread Architecture Guidelines (libcpu/arm/cortex-a)
 *   - Nokia-2660-RE docs/kernel-architecture.md
 */

#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* RT-Thread Kernel Core */
#define RT_NAME_MAX 16
#define RT_ALIGN_SIZE 8
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 100
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_HOOK_USING_FUNC_PTR
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 2048

/* Inter-Thread Communication */
#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

/* Memory Management */
#define RT_USING_HEAP
#define RT_USING_SMALL_MEM
#define RT_USING_SMALL_MEM_AS_HEAP

/* Kernel Software Timers */
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO 4
#define RT_TIMER_THREAD_STACK_SIZE 2048

/* Debugging & Console */
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 256
#define RT_BACKTRACE_LEVEL_MAX_NR 32
#define RT_VER_NUM 0x50200

/* Components & Auto-Initialization */
#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 4096
#define RT_MAIN_THREAD_PRIORITY 10

/* Architecture: ARM Cortex-A7 (ARMv7-A) UP */
#define ARCH_ARM
#define ARCH_ARM_CORTEX_A
#define ARCH_ARM_CORTEX_A7
#define RT_CPUS_NR 1

/* Standard Libc / klibc */
#define RT_KLIBC_USING_VSNPRINTF_TINY

#endif /* RT_CONFIG_H__ */
