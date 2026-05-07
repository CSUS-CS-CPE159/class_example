# XV6 File System Toolchain (mkfs)

This directory contains the `mkfs` utility, a host-side tool used to format a virtual disk image (`fs.img`) for the xv6 operating system. It translates files from your development machine into a structured disk layout that the xv6 kernel understands.

## 🏗 Disk Architecture

The file system image is organized into blocks of **512 bytes** each. The layout follows a specific sequence to allow the kernel to locate files efficiently:



1.  **Block 0 (Boot):** Unused by the file system; reserved for bootloader code.
2.  **Block 1 (Superblock):** Contains metadata about the file system (total blocks, number of inodes).
3.  **Inode Blocks:** Store `struct dinode` entries. Each inode defines a file's type, size, and location.
4.  **Bitmap Block:** A bit-array tracking which data blocks are free or allocated.
5.  **Data Blocks:** The actual raw data for files and directory structures.

## 🛠 Features

This version of `mkfs` supports extended attributes in the `dinode` structure:
*   **Permissions:** Integrated `mode` field for file access control (e.g., `0755`).
*   **Ownership:** Specific `ownerid` and `groupid` fields for multi-user simulation.
*   **Large Files:** Supports up to 10 direct blocks and one indirect block for expanded storage.

## 🚀 Getting Started

### Compilation
To compile the `mkfs` tool on your host machine:
```bash
make
```
### how to generate fs.img
```
make fs
```
