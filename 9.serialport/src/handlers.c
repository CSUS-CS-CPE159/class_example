// handlers.c, 159

#include "spede.h"
#include "types.h"
#include "handlers.h"
#include "data.h"
#include "proc.h"
#include "queue.h"

// to create process, alloc PID, PCB, and stack space
// build TF into stack, set PCB, register PID to ready_q
void NewProcHandler(func_ptr_t p){  // arg: where process code starts
  int pid; 

  if((queue_is_empty(&free_q))){           // this may occur for testing 
    cons_printf("Kernel Panic: no more PID left!\n");
    breakpoint();                   // breakpoint() into GDB
    return;
  }
  queue_out(&free_q, &pid);               // get 'pid' from free_q
  printf("pid is %d\n", pid);
  memset((char *)&pcb[pid], 0, sizeof(pcb_t));
  memset((char *)&proc_stack[pid], 0, PROC_STACK_SIZE); // use tool to clear the PCB (indexed by 'pid')
  pcb[pid].state = READY; 
  queue_in(&ready_q, pid);

  pcb[pid].TF_p = (TF_t *)&proc_stack[pid][PROC_STACK_SIZE - sizeof(TF_t)]; // point TF_p to highest area in stack
  // then fill out the eip of the TF
  pcb[pid].TF_p->eip = (unsigned int)p; // new process code
  pcb[pid].TF_p->eflags = EF_DEFAULT_VALUE|EF_INTR; // EFL will enable intr!
  pcb[pid].TF_p->cs = get_cs();         // duplicate from current CPU
  pcb[pid].TF_p->ds = get_ds();         // duplicate from current CPU
  pcb[pid].TF_p->es = get_es();         // duplicate from current CPU
  pcb[pid].TF_p->fs = get_fs();         // duplicate from current CPU
  pcb[pid].TF_p->gs = get_gs();         // duplicate from current CPU
}

void GetPidHandler(void){
  pcb[current_pid].TF_p->eax = (unsigned int) current_pid;
}

// count cpu_time of running process and preempt it if reaching limit
void TimerHandler(void){
    int i;
    pcb[current_pid].cpu_time++;    // upcount cpu_time of the process (PID is current_pid)
    current_time++;

    for(i=0; i<Q_SIZE; i++){
        if((pcb[i].state == SLEEP) && (pcb[i].wake_time == current_time)){ 
	        queue_in(&ready_q, i);           // append pid to ready_q
      	    pcb[i].state = READY;       // update proc state 
	        ch_p[40] = 0xf00;
 	        ch_p[43] = 0xf00;
	        ch_p[i*80+40] = 0xf00 + i + '0';
 	        ch_p[i*80+43] = 0xf00 +'r';
    	}   
    }
  
    if(pcb[current_pid].cpu_time == TIME_LIMIT){ // if its cpu_time reaches the preset OS time limit
   	    pcb[current_pid].state = READY; // update/downgrade its state
    	queue_in(&ready_q, current_pid);   // move it to ready_q
    	ch_p[40] = 0xf00;
 	    ch_p[43] = 0xf00;
	    ch_p[current_pid*80+40] = 0xf00 + current_pid + '0';
	    ch_p[current_pid*80+43] = 0xf00 +'r';
    	current_pid = 0;              // no longer runs
    } 
    outportb(0x20, 0x60);           // Don't forget: notify PIC event-handling done
}

void SleepHandler(int sleep_amount){
  pcb[current_pid].wake_time = current_time + (100 * sleep_amount); // calc future wake time in pcb
  pcb[current_pid].state = SLEEP; // update proc state

  ch_p[40] = 0xf00;
  ch_p[43] = 0xf00;
  ch_p[current_pid*80+40] = 0xf00 + current_pid + '0';
  ch_p[current_pid*80+43] = 0xf00 +'S';
    	
  current_pid = 0;                // reset current_pid
}

void SemAllocHandler(int passes){
  int sid;

  for(sid = 0; sid < Q_SIZE; sid++){
    if(sem[sid].owner == 0) break; 
  }
  if(sid == Q_SIZE){
    cons_printf("Kernel panic: no more semaphores left!\n");
    return; 
  }

  memset((char *)&sem[sid], 0, sizeof(sem_t));
  
  sem[sid].passes = passes;
  sem[sid].owner = current_pid;
  pcb[current_pid].TF_p->eax = sid; 
}

void SemWaitHandler(int sid){

  if(sem[sid].passes > 0){
    sem[sid].passes--;
    return; 
  } else{ 
    queue_in(&sem[sid].wait_q, current_pid);
    pcb[current_pid].state = WAIT;
	    
    ch_p[40] = 0xf00;
    ch_p[43] = 0xf00;
    ch_p[current_pid*80+40] = 0xf00 + current_pid + '0';
    ch_p[current_pid*80+43] = 0xf00 +'W';
    current_pid = 0;
  }
}

void SemPostHandler(int sid){
  int free_pid = 0;

  if((sem[sid].wait_q.size == 0)){
    sem[sid].passes++;
    return;
  } else{
    queue_out(&sem[sid].wait_q, &free_pid);
    queue_in(&ready_q, free_pid);
    pcb[free_pid].state = READY;
 
    ch_p[40] = 0xf00;
    ch_p[43] = 0xf00;
    ch_p[free_pid*80+40] = 0xf00 + free_pid + '0';
    ch_p[free_pid*80+43] = 0xf00 +'r';
  }
}

void PortWriteOne(int port_num){
   int one;
    	
   if( queue_is_empty(&port[port_num].write_q) && 
	    queue_is_empty(&port[port_num].loopback_q) ){
        port[port_num].write_ok = 1;  // record missing write event
		return;
    }
	if( !queue_is_empty(&port[port_num].loopback_q) ){
        queue_out(&port[port_num].loopback_q, &one);
  	} else { 
    	queue_out(&port[port_num].write_q, &one);
		// Freed a slot in write_q
    	SemPostHandler(port[port_num].write_sid);
  	}
	printf("[TX p=%d ch='%c']\r\n", port_num, (char)one);
  	outportb(port[port_num].IO + DATA, (unsigned char)one);
}

// ---- One-byte RX path (lower half) ----
void PortReadOne(int port_num){
  	unsigned char raw = inportb(port[port_num].IO+DATA);
 	
    char one = (char)(raw & 0x7F);	
  	if(queue_is_full(&port[port_num].read_q) ){
    	cons_printf("Kernel Panic: you are typing on terminal is super fast!\n");
    	return;
  	}
  	queue_in(&port[port_num].read_q, one);
  	queue_in(&port[port_num].loopback_q, one);
	
  	if(one == '\r' || one == '\n'){
    	queue_in(&port[port_num].loopback_q, '\n');
  	}
  	SemPostHandler(port[port_num].read_sid);  
}

void PortHandler(){
	// PORT_NUM equals 3 (COM Ports 2, 3 4)
  	for(int port_num=0; port_num<PORT_NUM; port_num++){ 
        if (port[port_num].owner == 0) continue;
		unsigned char iir = inportb(port[port_num].IO+IIR);
		if(iir == IIR_RXRDY) PortReadOne(port_num);
		if(iir == IIR_TXRDY) PortWriteOne(port_num);
    	if(port[port_num].write_ok != 0)PortWriteOne(port_num);
  	}
	outportb(0x20, 0x63);
	outportb(0x20, 0x64);

}

void PortAllocHandler(int *eax){
	// COM2, COM3, COM4
  	static int IO[PORT_NUM] = {0x2f8, 0x3e8, 0x2e8};

	int p;
  	for(p = 0; p< PORT_NUM; p++){
        if(port[p].owner == 0) break;
  	}

  	if(p == PORT_NUM){
    	cons_printf("Kernel Panic: no port left!\n");
    	return;
  	}

	*eax = p;
 
	memset((char *)&port[p], 0, sizeof(port_t));
  	port[p].owner = current_pid;
  	port[p].IO = IO[p];
	// Primed for first TX
  	port[p].write_ok = 1;
	
	// Program UART to 9600 7-E-1, enable RX/TX interrupts
  	int baud = 9600;
  	int divisor = 115200 / baud;
    	
	outportb(port[p].IO + IER, 0x0);  // Disable all interrupts
  	outportb(port[p].IO + CFCR, CFCR_DLAB);		// Enable DLAB (set baud rate divisor)
  	outportb(port[p].IO + BAUDLO, (unsigned char)(divisor & 0xFF)); // set divisor
  	outportb(port[p].IO + BAUDHI, (unsigned char)((divisor >> 8) & 0xFF));
	outportb(port[p].IO + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS); // 7 bits, Even Parity, and 1 stop bit
	//outportb(port[port_num].IO + CFCR, CFCR_8BITS);
  	outportb(port[p].IO + MCR, MCR_DTR | MCR_RTS | MCR_IENABLE);  // IRQs enabled, RTS/DSR set
  	// small delay for hardware settle	
	for (int i = 0; i<= 0x63; ++i) asm volatile("inb $0x80");

  	outportb(port[p].IO+IER, IER_ERXRDY | IER_ETXRDY);
}

// ---- Buffer one char for TX (upper -> lower) ----
void PortWriteHandler(char one, int port_num){
  	if(queue_is_full(&port[port_num].write_q)) {
        cons_printf("Kernel Panic: terminal is not prompting (fast enough)?\n");
    	return;
  	}
  	queue_in(&port[port_num].write_q, one);
	if(port[port_num].write_ok != 0)
	{
	    PortWriteOne(port_num);
  	}
}

void PortReadHandler(char *one, int port_num){
  	if(queue_is_empty(&port[port_num].read_q)){
    	cons_printf("Kernel Panic: nothing in typing/read buffer?\n");
    	return;
  	}
	int ch;
  	queue_out(&port[port_num].read_q, &ch);
	*one = (char)ch;
}
