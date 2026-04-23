/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Interrupt handling functions
 */
#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <spede/machine/asmacros.h>

// IRQ Definitions
#define IRQ_TIMER    0x20       // PIC IRQ 0 (Timer)
#define IRQ_KEYBOARD 0x21       // PIC IRQ 1 (Keyboard)
#define IRQ_IDE     0x2E       // PIC IRQ 14 (IDE driver)
#define IRQ_SYSCALL  0x80       // System call IRQ


#ifndef ASSEMBLER
/**
 * General interrupt enablement
 */
void interrupts_init(void);

/**
 * Enable interrupts with the CPU
 */
void interrupts_enable(void);

/**
 * Disable interrupts with the CPU
 */
void interrupts_disable(void);

/**
 * Registers an ISR in the IDT and IRQ handler for processing interrupts
 * @param irq - IRQ number
 * @param entry - function pointer to be registered in the IDT
 * @param handler - function pointer to be called when the specified IRQ occurs
 */
void interrupts_irq_register(int irq, void (*entry)(), void (*handler)());

/**
 * Interrupt service routine handler
 * @param irq - IRQ number
 */
void interrupts_irq_handler(int irq);

/**
 * Enables the specified IRQ in the PIC
 * @param irq - IRQ number
 */
void pic_irq_enable(int irq);

/**
 * Disables the specified IRQ in the PIC
 * @param irq - IRQ number
 */
void pic_irq_disable(int irq);

/**
 * Queries if the given IRQ is enabled in the PIC
 * @param irq - IRQ number
 * @return - 1 if enabled, 0 if disabled
 */
int pic_irq_enabled(int irq);

/**
 * Dismisses the specified IRQ in the PIC
 * @param irq - IRQ number
 */
void pic_irq_dismiss(int irq);


__BEGIN_DECLS

extern void isr_entry_timer();
extern void isr_entry_keyboard();
extern void isr_entry_syscall();
extern void isr_entry_ide();
__END_DECLS
#endif
#endif
