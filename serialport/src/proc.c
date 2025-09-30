// proc.c, 159
// all processes are coded here
// processes do not use kernel space (data.h) or code (handlers, tools, etc.)
// all must be done thru system service calls

#include "spede.h"      // cons_xxx below needs
#include "syscall.h"
#include "data.h"       // current_pid needed below
#include "proc.h"       // prototypes of processes
#include "handlers.h"

// Init PID 1, always ready to run, never preempted
void Init(void) {
  int i;
  char key;
  char str[] = " Hello, World! Team TestOS: Name1, Name2, Name3\n\r";

  while(1){
    if(cons_kbhit()){           // if a key is pressed on Target PC
      key = cons_getchar();     // get the key

      switch(key){              // switch by the key obtained {
        case 'p':
          cons_printf(str); // call SysPrintHandler to print
          break;
        case 'b':
          breakpoint();         // go into gdb
          break;
        case 'q':
          exit(0);              // quit program
      }   
    }

    //loop for LOOP times { // to cause approx 1 second of delay
    for(i=0; i<LOOP; i++){      
      asm("inb $0x80");         // call asm("inb $0x80") which delay .6 microsecond
    }
  }
}

void TermProc(void){
	// size 101
	char str_read[BUFF_SIZE]; 
	// init port device and port_t data associated
	int my_port = PortAlloc();    
  	while(1){
    		PortWrite("Now enter (username): ", my_port);
    		PortRead(str_read, my_port);
    		cons_printf("Read from port #%d: %s\n", my_port, str_read);

		PortWrite("Now enter (password): ", my_port);
    		PortRead(str_read, my_port);
    		cons_printf("Read from port #%d: %s\n", my_port, str_read);
    		PortWrite("Hello, Team MyOS here! \r\n", my_port); // \r also!
	}
}
