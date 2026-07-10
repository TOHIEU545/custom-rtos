.syntax unified
.cpu cortex-m3
.thumb
.global SVC_Handler
.type SVC_Handler, %function
SVC_Handler:
    /* TODO(stage 3): restore first task. */
    bx lr
.global PendSV_Handler
.type PendSV_Handler, %function
PendSV_Handler:
    /* TODO(stage 4): save/restore task context. */
    bx lr
