.syntax unified
.cpu cortex-m3
.thumb

/*
 * TEST-ONLY HELPER - khong phai kernel API. Chi bien dich va link vao
 * firmware test_register_preservation, khong dung o noi khac.
 *
 * uint32_t test_yield_check_r4_r11(uint32_t base):
 *   - Nap R4..R11 = base+4 .. base+11 (pattern rieng cho tung lan goi/task).
 *   - Goi ntos_yield() - co the that su context-switch qua PendSV o day.
 *   - Sau khi task duoc PendSV restore lai (hoac khong switch di dau ca),
 *     so sanh R4..R11 hien tai voi pattern da nap truoc do.
 *   - Return 1 (r0=1) neu tat ca dung, 0 (r0=0) neu bat ky mismatch.
 *
 * Ham nay chay o Thread mode (trong task), nen SP dang dung la PSP, khong
 * phai MSP. AAPCS bat buoc SP align 8 byte tai moi diem goi ham cong khai -
 * push {r3,r4..r11,lr} = 10 word = 40 byte, giu nguyen align 8.
 */
.global test_yield_check_r4_r11
.type test_yield_check_r4_r11, %function
test_yield_check_r4_r11:
    /* r4-r11 la callee-saved theo AAPCS: ham nay sap ghi de nen phai tu
     * luu ho caller. Luu them lr vi sap "bl ntos_yield" se ghi de lr. */
    push    {r3, r4, r5, r6, r7, r8, r9, r10, r11, lr}

    /* Nap pattern: R4..R11 = base+4 .. base+11. base dang o r0 (tham so dau). */
    add     r4,  r0, #4
    add     r5,  r0, #5
    add     r6,  r0, #6
    add     r7,  r0, #7
    add     r8,  r0, #8
    add     r9,  r0, #9
    add     r10, r0, #10
    add     r11, r0, #11

    /* Luu lai "base" xuong stack vi ntos_yield() duoc phep dung tuy y r0-r3. */
    push    {r0}

    bl      ntos_yield          /* co the that su context-switch qua PendSV o day */

    pop     {r0}                 /* r0 = base, khoi phuc lai sau khi ntos_yield() tra ve */

    movs    r1, #1               /* r1 = ket qua, mac dinh pass */

    add     r2, r0, #4
    cmp     r4, r2
    bne     .Lcheck_fail

    add     r2, r0, #5
    cmp     r5, r2
    bne     .Lcheck_fail

    add     r2, r0, #6
    cmp     r6, r2
    bne     .Lcheck_fail

    add     r2, r0, #7
    cmp     r7, r2
    bne     .Lcheck_fail

    add     r2, r0, #8
    cmp     r8, r2
    bne     .Lcheck_fail

    add     r2, r0, #9
    cmp     r9, r2
    bne     .Lcheck_fail

    add     r2, r0, #10
    cmp     r10, r2
    bne     .Lcheck_fail

    add     r2, r0, #11
    cmp     r11, r2
    bne     .Lcheck_fail

    b       .Lcheck_done

.Lcheck_fail:
    movs    r1, #0

.Lcheck_done:
    mov     r0, r1                              /* return value trong r0 theo AAPCS */
    pop     {r3, r4, r5, r6, r7, r8, r9, r10, r11, lr} /* khoi phuc r4-r11 cua caller */
    bx      lr
