/**
 * Calling Convention Example
 * 
 * Demonstrates x86-64 calling convention by examining function call stack frames.
 * 
 * This example shows how parameters are passed to functions and how the
 * call stack is organized. By stepping through with GDB, you can observe:
 * - Function parameters on the stack
 * - Return addresses
 * - Stack pointer (esp) changes during function calls
 * - Local variable storage
 * 
 * Key Learning Points:
 * - Understanding the stack layout during function calls
 * - How calling conventions affect register and parameter passing
 * - The role of the stack pointer (esp) and base pointer (ebp)
 * 
 * GDB Usage:
 *   (gdb) b main          - Break at main
 *   (gdb) si              - Step one instruction
 *   (gdb) print $esp      - Check stack pointer
 *   (gdb) layout asm      - Show assembly code
 */
#include "stdio.h"

/**
 * Simple addition function
 * Demonstrates parameter passing via stack
 * 
 * @param a First operand
 * @param b Second operand
 * @param c Third operand
 * @return Sum of a, b, and c
 */
int add(int a, int b, int c){
    return a + b + c;
}

int main(){
    int a = 1;
    int b = 2;
    int c = 3;
    int sum = add(a, b, c);  // Stack frame created here
    printf("sum is %d\n", sum);
    return 1;
}
