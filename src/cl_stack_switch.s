@ CHESS LEGENDS: ARCANE WARS - run a function on a different stack.
@
@ void cl_call_with_stack(void* stack_top, void (*function)(void*), void* argument)
@
@ The AI search recurses deeply; it runs on a dedicated stack in EWRAM so the scarce 32 KB IWRAM can hold the
@ hottest engine code instead of a large stack. Interrupts keep using their own IRQ-mode stack.
@ stack_top must be 8-byte aligned (AAPCS).

    .section .text.cl_call_with_stack, "ax", %progbits
    .arm
    .align 2
    .global cl_call_with_stack
    .type cl_call_with_stack, %function

cl_call_with_stack:
    push    {r4, lr}
    mov     r4, sp              @ remember the caller's stack
    mov     sp, r0              @ switch to the new stack
    mov     r0, r2              @ argument
    mov     lr, pc              @ return address (ARM, the instruction after bx)
    bx      r1                  @ call function(argument), ARM or Thumb
    mov     sp, r4              @ back to the caller's stack
    pop     {r4, lr}
    bx      lr

    .size cl_call_with_stack, . - cl_call_with_stack
