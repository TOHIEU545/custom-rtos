.syntax unified
.cpu cortex-m3
.thumb
/* SVC_Handler: chuyen sang naked C function tai port/cortex_m3/ntos_port.c (Chang 3). */
.global PendSV_Handler
.type PendSV_Handler, %function
PendSV_Handler:
    /* TODO(stage 4): save/restore task context. */
    bx lr
