.globl irq_vector_init
irq_vector_init:
    adr x0, vectors
    msr vbar_el1, x0 // Vector Base Address Register
    ret

// Enable/Disable interrupts
// daif in daifclr and daifset stand for the type of exception to mask:
// D: Debug Exceptions (special type of synchronous exceptions).
// A: Asynchronous Exceptions (masks System Errors (or SErrors)).
// I: IRQs (Interrupt Request).
// F: FIQs (Fast Interrupt Request).

// Enable IRQs (the #2 is because we only want to enable IRQs (b0010))
.globl enable_irq
enable_irq:
    msr    daifclr, #2 
    ret

// Disable IRQ (the #2 is because we only want to disable IRQs (b0010))
.globl disable_irq
disable_irq:
    msr daifset, #2
    ret