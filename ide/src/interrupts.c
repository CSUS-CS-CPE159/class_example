/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Interrupt handling functions
 */
#include <spede/machine/io.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/string.h>

#include "kernel.h"
#include "interrupts.h"

// Maximum number of ISR handlers
#define IRQ_MAX      0xf0

// PIC Definitions
#define PIC1_BASE   0x20            // base address for PIC primary controller
#define PIC2_BASE   0xa0            // base address for PIC secondary controller
#define PIC1_CMD    PIC1_BASE       // address for issuing commands to PIC1
#define PIC1_DATA   (PIC1_BASE+1)   // address for setting data for PIC1
#define PIC2_CMD    PIC2_BASE       // address for issuing commands to PIC2
#define PIC2_DATA   (PIC2_BASE+1)   // address for setting data for PIC2

#define PIC_EOI     0x20            // PIC End-of-Interrupt command

// Interrupt descriptor table
struct i386_gate *idt = NULL;

// Interrupt handler table
// Contains an array of function pointers associated with
// the various interrupts to be handled
void (*irq_handlers[IRQ_MAX])();

/**
 * Enable interrupts with the CPU
 */
void interrupts_enable(void) {
    kernel_log_trace("interrupts: enabling");
    asm("sti");
}

/**
 * Disable interrupts with the CPU
 */
void interrupts_disable(void) {
    kernel_log_trace("interrupts: disabling");
    asm("cli");
}

/**
 * Handles the specified interrupt by dispatching to the registered function
 * @param interrupt - interrupt number
 */
void interrupts_irq_handler(int irq) {
    if (irq < 0 || irq >= IRQ_MAX) {
        kernel_panic("interrupts: Invalid IRQ %d (0x%02x)", irq, irq);
        return;
    }

    if (irq_handlers[irq] == NULL) {
        kernel_panic("interrupts: No handler registered for IRQ %d (0x%02x)", irq, irq);
        return;
    }

    irq_handlers[irq]();

    /* If the IRQ originates from the PIC, dismiss the IRQ */
    if (irq >= 0x20 && irq <= 0x2F) {
        pic_irq_dismiss(irq - 0x20);
    }
}

/*
 * Registers the appropriate IDT entry and handler function for the
 * specified interrupt.
 *
 * @param interrupt - interrupt number
 * @param entry - the function to run when the interrupt occurs
 * @param handler - the function to be called to process the the interrupt
 */
void interrupts_irq_register(int irq, void (*entry)(), void (*handler)()) {
    if (irq < 0 || irq >= IRQ_MAX) {
        kernel_panic("interrupts: Invalid IRQ %d (0x%02x)", irq, irq);
        return;
    }

    if (!entry) {
        kernel_panic("interrupts: Invalid IDT entry for IRQ %d (0x%02x)", irq, irq);
        return;
    }

    if (!handler) {
        kernel_panic("interrupts: Invalid handler for IRQ %d (0x%02x)", irq, irq);
        return;
    }

    // Add the entry to the IDT
    fill_gate(&idt[irq], (int)entry, get_cs(), ACC_INTR_GATE, 0);
    kernel_log_debug("interrupts: IRQ %d (0x%02x) IDT entry added", irq, irq);

    /* Add the ISR handler to the table */
    irq_handlers[irq]=handler;
    kernel_log_debug("interrupts: IRQ %d (0x%02x) handler added", irq, irq);

    /* If the interrupt originates from the PIC, enable IRQs */
    if (irq >= 0x20 && irq <= 0x2F) {
        pic_irq_enable(irq);
    }

    kernel_log_info("interrupts: IRQ %d (0x%02x) registered)", irq, irq);
}

/**
 * Enables the specified IRQ on the PIC
 *
 * @param irq - IRQ that should be enabled
 * @note IRQs > 0xf will be remapped
 */
void pic_irq_enable(int irq) {
    int port = PIC1_DATA;
    int mask;

    // Isolate only the first nibble; handles remapping
    irq &= 0xf;

    // Select the secondary PIC if the IRQ is associated with it
    if (irq >= 0x8) {
        port = PIC2_DATA;
        irq -= 0x8;
    }

    // Read the current mask
    mask = inportb(port);

    // Set the IRQ
    mask &= ~(1 << irq);

    outportb(port, mask);

    // Write the mask out to the PIC
    outportb(port, mask);

    kernel_log_trace("interrupts: Enabled IRQ %d (0x%02x) via PIC port=0x%02x, mask=0x%02x", irq, irq, port, mask);
}

/**
 * Disables the specified IRQ via the PIC
 *
 * @param irq - IRQ that should be disabled
 */
void pic_irq_disable(int irq) {
    int port = PIC1_DATA;
    int mask;

    // We only care about bits 0-7, this allows
    // us to handle remapped IRQs
    irq &= 0xf;

    // Determine the PIC to be used and adjust the irq number
    if (irq >= 0x8) {
        port = PIC2_DATA;
        irq -= 0x8;
    }

    // Read the current mask
    mask = inportb(port);

    // Set the bit in the mask to disable the IRQ
    mask = 0xff & (mask ^ (1 << irq));

    // Write the mask back to the PIC
    outportb(port, mask);

    kernel_log_trace("interrupts: Disabled IRQ %d (0x%02x) via PIC port=0x%02x, mask=0x%02x", irq, irq, port, mask);
}

/**
 * Queries if the given IRQ is enabled on the PIC
 *
 * @param irq - IRQ to check
 * @return - 1 if enabled, 0 if disabled
 */
int pic_irq_enabled(int irq) {
    int port = PIC1_DATA;
    int mask;

    // We only care about bits 0-7, this allows
    // us to handle remapped IRQs
    irq &= 0xf;

    // Determine the PIC to be used and adjust the irq number
    if (irq >= 0x8) {
        port = PIC2_DATA;
        irq -= 0x8;
    }

    // Read the current mask
    mask = inportb(port);

    return (mask & (1 << irq)) ? 0 : 1;
}

/**
 * Dismisses an interrupt by sending the EOI command to the appropriate
 * PIC device(s). If the IRQ is assosciated with the secondary PIC, the
 * EOI command must be issued to both since the PICs are dasiy-chained.
 *
 * @param irq - IRQ to be dismissed
 */
void pic_irq_dismiss(int irq) {
    // We only care about bits 0-7, this allows
    // us to handle remapped IRQs
    irq &= 0xf;

    if (irq >= 0x8) {
        // Send EOI to the secondary PIC
        outportb(PIC2_CMD, PIC_EOI);
    }

    // Send EOI to the primary PIC
    outportb(PIC1_CMD, PIC_EOI);
}

/**
 * Interrupt initialization
 */
void interrupts_init() {
    // Obtain the IDT base address
    idt = get_idt_base();

    memset(irq_handlers, 0, sizeof(irq_handlers));
}

