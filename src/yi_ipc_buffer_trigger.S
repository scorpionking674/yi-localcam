/*
 * yi_ipc_buffer_trigger.S
 * Minimal freestanding ARM EABI helper for Yi y211ga-family cameras.
 * Sends the 20-byte IPC_XXX_0 message to the existing /ipc_dispatch
 * POSIX message queue. No libc and no vendor SDK required.
 *
 * Intended purpose: trigger the stock media pipeline/circular frame buffer
 * after dispatch/rmm are running, without starting the vendor cloud client.
 *
 * Linux ARM EABI syscalls used:
 *   write=4, close=6, exit=1, mq_open=274, mq_timedsend=276
 */

    .syntax unified
    .arch armv7-a
    .text
    .global _start
    .type _start, %function

_start:
    /* Kernel mq_open expects the queue name without the leading slash.
       Equivalent userspace call: mq_open("/ipc_dispatch", O_RDWR). */
    ldr     r0, =queue_name
    mov     r1, #2              /* O_RDWR */
    mov     r2, #0              /* mode, unused without O_CREAT */
    mov     r3, #0              /* attr, unused without O_CREAT */
    ldr     r7, =274            /* __NR_mq_open */
    svc     #0
    cmp     r0, #0
    blt     open_failed
    mov     r5, r0              /* save mqd */

    /* mq_timedsend(mqd, msg, 20, prio=0, abs_timeout=NULL) */
    mov     r0, r5
    ldr     r1, =ipc_xxx_0
    mov     r2, #20
    mov     r3, #0
    mov     r4, #0
    ldr     r7, =276            /* __NR_mq_timedsend */
    svc     #0
    cmp     r0, #0
    blt     send_failed

    mov     r0, r5
    mov     r7, #6              /* close */
    svc     #0

    mov     r0, #1              /* stdout */
    ldr     r1, =msg_ok
    mov     r2, #17
    mov     r7, #4              /* write */
    svc     #0
    mov     r0, #0
    mov     r7, #1              /* exit */
    svc     #0

open_failed:
    mov     r0, #2              /* stderr */
    ldr     r1, =msg_open_fail
    mov     r2, #15
    mov     r7, #4
    svc     #0
    mov     r0, #10
    mov     r7, #1
    svc     #0

send_failed:
    mov     r0, r5
    mov     r7, #6
    svc     #0
    mov     r0, #2
    ldr     r1, =msg_send_fail
    mov     r2, #20
    mov     r7, #4
    svc     #0
    mov     r0, #11
    mov     r7, #1
    svc     #0

    .section .rodata
    .align 2
queue_name:
    .asciz "ipc_dispatch"

/* IPC_XXX_0 from yi-hack-Allwinner-v2 ipc_cmd.h */
ipc_xxx_0:
    .byte 0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00
    .byte 0x71,0x00,0x71,0x00,0x00,0x00,0x00,0x00
    .byte 0xa1,0x0e,0x9a,0x5e

msg_ok:
    .ascii "IPC trigger sent\n"
msg_open_fail:
    .ascii "mq_open failed\n"
msg_send_fail:
    .ascii "mq_timedsend failed\n"
