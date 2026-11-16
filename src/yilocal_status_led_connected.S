/*
 * YiLocal v1.0.0 status LED helper
 *
 * Uses the already-running vendor dispatch POSIX queue instead of touching
 * CPLD registers directly. Reverse-engineered dispatch command IDs:
 *   167 = DISPATCH_FACTORY_SET_BLUE_LED
 *   168 = DISPATCH_FACTORY_SET_YELLOW_LED
 * Parameter 1 maps to the vendor ON command; parameter 0 maps to OFF.
 *
 * Sends: yellow OFF, then blue ON.
 * RAM/runtime only; no flash writes.
 */
    .syntax unified
    .arch armv7-a
    .text
    .global _start
    .type _start, %function

_start:
    ldr r0, =queue_name
    mov r1, #2              /* O_RDWR */
    mov r2, #0
    mov r3, #0
    ldr r7, =274            /* mq_open */
    svc #0
    cmp r0, #0
    blt open_failed
    mov r5, r0

    /* yellow LED OFF: dispatch cmd 168, arg 0 */
    mov r0, r5
    ldr r1, =msg_yellow_off
    mov r2, #20
    mov r3, #0
    mov r4, #0
    ldr r7, =276            /* mq_timedsend */
    svc #0
    cmp r0, #0
    blt send_failed

    /* blue LED ON: dispatch cmd 167, arg 1 */
    mov r0, r5
    ldr r1, =msg_blue_on
    mov r2, #20
    mov r3, #0
    mov r4, #0
    ldr r7, =276
    svc #0
    cmp r0, #0
    blt send_failed

    mov r0, r5
    mov r7, #6
    svc #0

    mov r0, #1
    ldr r1, =ok
    mov r2, #24
    mov r7, #4
    svc #0
    mov r0, #0
    mov r7, #1
    svc #0

open_failed:
    mov r0, #2
    ldr r1, =open_fail
    mov r2, #24
    mov r7, #4
    svc #0
    mov r0, #10
    mov r7, #1
    svc #0

send_failed:
    mov r0, r5
    mov r7, #6
    svc #0
    mov r0, #2
    ldr r1, =send_fail
    mov r2, #25
    mov r7, #4
    svc #0
    mov r0, #11
    mov r7, #1
    svc #0

    .section .rodata
    .align 2
queue_name:
    .asciz "ipc_dispatch"

/* Header/layout follows the proven 20-byte /ipc_dispatch helper already used
 * by YiLocal. Command is duplicated as little-endian u16 at bytes 8 and 10;
 * 32-bit argument begins at byte 12. */
msg_yellow_off:
    .byte 0x01,0x00,0x00,0x00, 0x04,0x00,0x00,0x00
    .byte 0xa8,0x00,0xa8,0x00, 0x00,0x00,0x00,0x00
    .byte 0xa1,0x0e,0x9a,0x5e
msg_blue_on:
    .byte 0x01,0x00,0x00,0x00, 0x04,0x00,0x00,0x00
    .byte 0xa7,0x00,0xa7,0x00, 0x01,0x00,0x00,0x00
    .byte 0xa1,0x0e,0x9a,0x5e

ok:        .ascii "status LED command sent\n"
open_fail: .ascii "status LED mq_open fail\n"
send_fail: .ascii "status LED mq_send failed\n"
