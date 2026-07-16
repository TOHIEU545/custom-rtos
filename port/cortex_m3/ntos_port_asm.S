.syntax unified
.cpu cortex-m3
.thumb
/* SVC_Handler va PendSV_Handler: da chuyen sang naked C function tai
 * port/cortex_m3/ntos_port.c (Chang 3, Chang 4). File nay hien khong con
 * handler nao - giu lai cho cac stage sau (SysTick_Handler, stage 5). */
