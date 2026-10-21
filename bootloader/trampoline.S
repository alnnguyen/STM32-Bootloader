.syntax unified
.cpu cortex-m4
.thumb
.global jump_trampoline
.type jump_trampoline,%function
.thumb_func
jump_trampoline:
    movs r2, #0
    msr control, r2
    msr basepri, r2
    msr faultmask, r2
    isb
    msr msp, r0
    bx r1
.size jump_trampoline, .-jump_trampoline
