### This code is used for practicing static process relocation.

We create two kernel tasks and then relocate one of them to a new address (segAddr). To complete the relocation, we update the task’s EIP value to point to this new address. This example illustrates that when performing static process relocation, the entire process must remain contiguous in memory. Otherwise, certain instructions (e.g., jumps) may reference incorrect locations, since compiled code often computes absolute addresses using the current EIP plus an offset.

### be careful relative addressing issues when we do static process relocation. 

