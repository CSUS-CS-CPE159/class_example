/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Kernel functions
 */
#include <spede/flames.h>
#include <spede/stdarg.h>
#include <spede/stdio.h>
#include <spede/string.h>

#include "interrupts.h"
#include "kernel.h"

#ifndef KERNEL_LOG_LEVEL_DEFAULT
#define KERNEL_LOG_LEVEL_DEFAULT KERNEL_LOG_LEVEL_TRACE
#endif


// Current log level
int kernel_log_level = KERNEL_LOG_LEVEL_DEFAULT;

/**
 * Initializes any kernel internal data structures and variables
 */
void kernel_init(void) {
    // Display a welcome message on the host
    kernel_log_info("Welcome to %s!", OS_NAME);

    kernel_log_info("Initializing kernel...");
}

/**
 * Prints a kernel log message to the host with an error log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_error(char *msg, ...) {
    // Return if our log level is less than error
    if (kernel_log_level < KERNEL_LOG_LEVEL_ERROR) {
        return;
    }

    va_list args;

    printf("error: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with a warning log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_warn(char *msg, ...) {
    // Return if our log level is less than warn
    if (kernel_log_level < KERNEL_LOG_LEVEL_WARN) {
        return;
    }

    va_list args;

    printf("warn: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with an info log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_info(char *msg, ...) {
    // Return if our log level is less than info
    if (kernel_log_level < KERNEL_LOG_LEVEL_INFO) {
        return;
    }

    // Obtain the list of variable arguments
    va_list args;

    // Indicate this is an 'info' type of message
    printf("info: ");

    // Pass the message variable arguments to vprintf
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with a debug log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_debug(char *msg, ...) {
    // Return if our log level is less than debug
    if (kernel_log_level < KERNEL_LOG_LEVEL_DEBUG) {
        return;
    }

    va_list args;

    printf("debug: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with a trace log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_trace(char *msg, ...) {
    // Return if our log level is less than trace
    if (kernel_log_level < KERNEL_LOG_LEVEL_TRACE) {
        return;
    }

    va_list args;

    printf("trace: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Triggers a kernel panic that does the following:
 *   - Displays a panic message on the host console
 *   - Triggers a breakpiont (if running through GDB)
 *   - aborts/exits the operating system program
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_panic(char *msg, ...) {
    va_list args;

    printf("panic: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");

    breakpoint();
    exit(1);
}

/**
 * Returns the current log level
 * @return the kernel log level
 */
int kernel_get_log_level(void) {
    return kernel_log_level;
}

/**
 * Sets the new log level and returns the value set
 * @param level - the log level to set
 * @return the kernel log level
 */
int kernel_set_log_level(int level) {
    if (level < KERNEL_LOG_LEVEL_NONE) {
        kernel_log_level = KERNEL_LOG_LEVEL_NONE;
    } else if (level > KERNEL_LOG_LEVEL_ALL) {
        kernel_log_level = KERNEL_LOG_LEVEL_ALL;
    } else {
        kernel_log_level = level;
    }

    kernel_log_info("kernel log level set to %d", kernel_log_level);

    return kernel_log_level;
}
