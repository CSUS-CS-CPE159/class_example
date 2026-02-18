<h3>Timer Event</h3>

<p>
The objective of this example is to practice how to
program to use the timer interrupt events.
</p>

Write your own code following class lectures and this pseudo code:
```
test
```
Compile into <i>MyOS.dli</i>
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

### Example description 
<ol type=a>
<li>The timer-driven routine should output your name, character by character.
<li>The output sequence is at every 3/4 seconds one character is shown.
<li>The output location is at the center of the screen on the target PC.
<li>After the full name is shown, it then erases it and restart.
</ol>

### Spede also provide some useful I/O functions
```
cons_putchar() // print one character on the target display
cons_printf()  // print one sentence on the target display
cons_kbhit()   // poll keyboard input, returns 1 if pressed
```
