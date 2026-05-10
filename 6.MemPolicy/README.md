
# Memory Management Policies (Allocation & GDT/Paging)

## Overview
This module demonstrates advanced memory management concepts including:
- Global Descriptor Table (GDT) for memory segmentation
- Page table setup and management
- Kernel memory allocation (kalloc)
- Memory protection via segmentation and paging
- Virtual to physical address translation

## Learning Objectives
- Understand GDT (Global Descriptor Table) and memory segmentation
- Learn how to set up paging and page tables
- Understand kernel memory allocation
- See how virtual memory protects processes
- Practice memory management policy implementation
- Understand the interaction between segmentation and paging

## Build and Run
```bash
make
spede-target   # Separate terminal
spede-run -d
```

## Key Concepts

### Memory Segmentation (GDT)

The Global Descriptor Table (GDT) defines memory segments:

**Each segment descriptor contains:**
- Base address (where segment starts)
- Limit (size of segment)  
- Type (code/data, read/write permissions)
- Privilege level (ring 0=kernel, ring 3=user)

**Common segments:**
- **Kernel Code**: Ring 0, executable
- **Kernel Data**: Ring 0, read/write
- **User Code**: Ring 3, executable
- **User Data**: Ring 3, read/write

### Paging

Paging adds another layer of memory protection:

**Two-level page table:**
- Page Directory (4MB address space)
- Page Tables (4KB address space)

**Page table entry:** Contains present bit, read/write, user/supervisor bits

## Files in This Module

### gdt.h / gdt.c
- Defines GDT structure
- GDT initialization
- Loading GDT with LGDT instruction
- Setting up kernel and user segments

### kalloc.h / kalloc.c
- Kernel memory allocator
- Simple allocation from kernel heap
- Tracking allocated memory blocks
- Free list management

### page.h / page.c
- Page table setup and management
- Virtual memory initialization
- Identity mapping (virtual = physical)
- Enabling paging

### process.h / process.c
- Process control blocks with memory info
- Process memory allocation
- Paging setup per process

## Memory Protection Mechanisms

### Segmentation-Based
1. GDT defines maximum segment size and privilege
2. Invalid memory access within segment OK
3. Cross-segment access may be invalid

### Paging-Based
1. Page table entry has Present bit
2. Invalid page access = page fault
3. Fine-grained per-page protection
4. Read/Write/Execute bits per page

### Supervisor vs. User Mode
- **Supervisor (Ring 0)**: Kernel code, all access allowed
- **User (Ring 3)**: User programs, restricted access
- Transitions via system calls and exceptions

## Kernel Memory Allocation

The module includes a kernel memory allocator for tracking memory use.

## Progression
- **Before**: `5.PageFault` - Exception handling for memory
- **Next**: `7.Syscall` - System call implementation
- **Next**: `8.UserProcess` - User-mode processes

## Why This Matters

Memory management is where hardware and OS software meet, enabling:
- Process isolation and protection
- Virtual memory abstraction
- Fair resource sharing
- Security enforcement

