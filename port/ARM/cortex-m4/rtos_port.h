#ifndef RTOS_PORT_H
#define RTOS_PORT_H

/**
 * @file rtos_port.h
 * @brief API port ARM Cortex-M4F: scheduler, context switch and interrupt control.
 */

#include "rtos.h"

#include <stddef.h>
#include <stdint.h>

/** R0-R3, R12, LR, PC and xPSR automatically stacked by the Cortex-M4. */
#define RTOS_PORT_BASIC_FRAME_WORDS ((size_t)8U)

/** Alignment padding, R4-R11 and EXC_RETURN saved by the port. */
#define RTOS_PORT_SOFTWARE_FRAME_WORDS ((size_t)10U)

/** Minimum frame created before a task runs for the first time. */
#define RTOS_PORT_INITIAL_FRAME_WORDS                                                    \
    (RTOS_PORT_BASIC_FRAME_WORDS + RTOS_PORT_SOFTWARE_FRAME_WORDS)

/**
 * Additional frame used after a task executes floating-point instructions:
 * S0-S15, FPSCR and the reserved word are hardware-stacked; S16-S31 are
 * software-stacked by PendSV.
 */
#define RTOS_PORT_FP_EXTENSION_WORDS ((size_t)34U)

/** Largest architecture context frame saved for a floating-point task. */
#define RTOS_PORT_MAX_CONTEXT_FRAME_WORDS                                               \
    (RTOS_PORT_INITIAL_FRAME_WORDS + RTOS_PORT_FP_EXTENSION_WORDS)

/** Saved PRIMASK value used by the critical-section API. */
typedef uint32_t rtos_irq_state_t;

/**
 * Build the initial Cortex-M4F context frame at the top of a task stack.
 *
 * A new task starts with a basic exception frame. Its saved EXC_RETURN marks
 * the frame as non-floating-point; the FP extension is added lazily only after
 * that task uses the FPU and is interrupted.
 */
uint32_t *rtos_port_stack_init(uint32_t *stack, size_t stack_words,
                               rtos_task_entry_t entry, void *argument);

/** Start the first task selected by the scheduler; never returns. */
void rtos_port_start_scheduler(void) __attribute__((noreturn));

/** Pend PendSV to request a context switch. */
void rtos_port_request_context_switch(void);

/** Configure exception priorities and start SysTick at RTOS_TICK_RATE_HZ. */
void rtos_port_start_tick(uint32_t cpu_clock_hz);

/** Stop execution after a fatal kernel invariant failure. */
void rtos_port_panic(void) __attribute__((noreturn));

/** Save PRIMASK and disable maskable interrupts. */
rtos_irq_state_t rtos_port_irq_save(void);

/** Restore the PRIMASK value returned by rtos_port_irq_save(). */
void rtos_port_irq_restore(rtos_irq_state_t state);

#endif
