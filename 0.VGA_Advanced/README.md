### VGA Text Mode
VGA text mode is fundamentally an 8-bit system, which means it is limited by how many unique character "slots" the hardware can address at once. 

#### Standard Mode: 256 characters. By default, VGA uses an 8-bit character code for each screen position, allowing for 256 unique glyphs simultaneously. 

#### Internal Storage
VGA hardware has Plane 2 (font memory), which is dedicated to storing character bitmaps. 

Each character in the memory is allocated as 16 pixels high (standard for 80x25)

# Compile into <i>MyOS.dli</i>
```
make
```
Start the target window, and you can see the file spede.sock in current folder:
```
spede-target
```
Run it under GDB.
```
spede-run -d
```

#### Reference
https://wiki.osdev.org/VGA_Hardware#:~:text=to%20be%20written.-,Memory%20Layout%20in%20text%20modes,requires%20changes%20in%20addressing%20logic.
