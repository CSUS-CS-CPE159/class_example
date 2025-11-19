// Device Driver: "Interrupt Driven"

#include "spede.h"    // given SPEDE stuff
#include "types.h"    // data types
#include "handlers.h" // handler code
#include "proc.h"     // processes such as Init()
#include "events.h"   // events for kernel to serve
#include "queue.h"    // queue function

// kernel's own data:
int current_pid, current_time, vehicle_sid;       	// current selected PID; if 0, none selected
q_t ready_q, free_q;    				// processes ready to run and not used
pcb_t pcb[PROC_NUM];    				// process control blocks
char proc_stack[PROC_NUM][PROC_STACK_SIZE];       	// process runtime stacks
struct i386_gate *IDT_p;
unsigned short *ch_p = (unsigned short*)0xB8000;  	// init ch_p pointer to vga
sem_t sem[Q_SIZE];
port_t port[PORT_NUM];

void IDTEntrySet(int event_num, func_ptr_t event_addr){
	fill_gate(&IDT_p[event_num], (int)event_addr, get_cs(), ACC_INTR_GATE, 0);
} 

void Scheduler(){                 // choose a PID as current_pid to load/run
    if(current_pid != 0) return;    // if continue below, find one for current_pid

    if (queue_is_empty(&ready_q)){         // if ready_q.size is 0 {
		cons_printf("Kernel Panic: no process to run!\n"); // big problem!
    	breakpoint();                 // alternative
    }
    queue_out(&ready_q, &current_pid);    // get next ready-to-run process as current_pid
    pcb[current_pid].state = RUN;   // update proc state
    pcb[current_pid].cpu_time = 0;  // reset proc cpu_time count 
    ch_p[40] = 0xf00;
    ch_p[43] = 0xf00;
    ch_p[current_pid*80+40] = 0xf00 + current_pid + '0';
    ch_p[current_pid*80+43] = 0xf00 + 'R'; 
} 

// OS bootstrap from main() which is process 0, so we do not use this PID
int main() {
    int i;
     
    memset((char *)&ready_q, 0, sizeof(q_t));
    memset((char *)&free_q, 0, sizeof(q_t));
    memset((char *)&sem, 0, (sizeof(sem_t))*Q_SIZE);
    memset((char *)&port, 0, (sizeof(port_t))*PORT_NUM);
  
    current_time = 0;       // init current time 
    vehicle_sid = -1;       // vehicle proc running

    // queue free_q with pid 1~19
    for(i=1; i<PROC_NUM; i++){
    	queue_in(&free_q, i);
    }
  
    for(i=0; i<PORT_NUM; i++){
    	port[i].owner = 0;
    }

    IDT_p = get_idt_base();   // init IDT_p (locate IDT location)
    cons_printf("IDT located @ DRAM addr %x (%d).\n", IDT_p, IDT_p); // show location on Target PC
    IDTEntrySet(TIMER_EVENT, TimerEvent);
    IDTEntrySet(GETPID_EVENT, GetPidEvent);
    IDTEntrySet(SLEEP_EVENT, SleepEvent);
    IDTEntrySet(SEMALLOC_EVENT, SemAllocEvent);
    IDTEntrySet(SEMWAIT_EVENT, SemWaitEvent);
    IDTEntrySet(SEMPOST_EVENT, SemPostEvent);
    
    IDTEntrySet(PORT_EVENT, PortEvent);
    IDTEntrySet(PORT_EVENT+1, PortEvent);
    IDTEntrySet(PORTALLOC_EVENT, PortAllocEvent);
    IDTEntrySet(PORTWRITE_EVENT, PortWriteEvent);
    IDTEntrySet(PORTREAD_EVENT, PortReadEvent);
    
    outportb(0x21, ~0x19);    // set PIC mask for IRQ0, IRQ3 and IRQ4
    NewProcHandler(Init);     // call NewProcHandler(Init) to create Init proc
    NewProcHandler(TermProc); // call NewProcHandler(Init) to create Init proc
    Scheduler();              // call scheduler to select current_pid (if needed)
    Loader(pcb[current_pid].TF_p); // call Loader with the TF address of current_pid
    return 0;                 // compiler needs for syntax altho this statement is never exec
} 

void Kernel(TF_t *TF_p) {       // kernel code exec (at least 100 times/second)
    pcb[current_pid].TF_p = TF_p; // save TF_P into the PCB of current_pid
  
    // switch according to the event_num in the TF TF_p points to {
    switch (TF_p->event_num){
		case TIMER_EVENT: 
	    	TimerHandler();
	    	break;
        case SLEEP_EVENT:
            SleepHandler(TF_p->eax);
            break;
		case GETPID_EVENT:
            GetPidHandler(); 
            break;
        case SEMALLOC_EVENT:
            SemAllocHandler(TF_p->eax);
            break;
        case SEMWAIT_EVENT:
            SemWaitHandler(TF_p->eax);
            break;
    	case SEMPOST_EVENT:
            SemPostHandler(TF_p->eax);
      	    break;
    	case PORT_EVENT:
      	    PortHandler();
      	    break;
    	case PORTALLOC_EVENT:
      	    PortAllocHandler((int *)&TF_p->eax);
      	    break;
    	case PORTWRITE_EVENT:
      	    PortWriteHandler((char)TF_p->eax, TF_p->ebx);
      	    break;
    	case PORTREAD_EVENT:
      	    PortReadHandler((char *)TF_p->eax, TF_p->ebx);
      		break;
    	default:
      	    cons_printf("Kernel Panic: unknown event_num %d!\n"); 
      	    breakpoint();
    }
    Scheduler();                    // call scheduler to select current_pid (if needed)
    Loader(pcb[current_pid].TF_p);  // call Loader with the TF address of current_pid
} 
