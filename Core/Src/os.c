/*
 * os.c
 *
 *  Created on: Nov 27, 2025
 *      Author: Saklen Sayyed
 */


#include "stm32f4xx.h"

#define NUM_THREADS  (3)
#define STACK_SIZE  (100)   /* -----( Total 400 Bytes   )----- */
#define FIFOSIZE    (100)

/* --------------------------( Systick Macro's )-------------------------- */

#define CLKSOURCE_MASK       (1<<2)
#define TICKINT_MASK         (1<<1)
#define ENABLE_SYSTICK_MASK	 (1<<0)

/* -----------------------( Thread Control Block )------------------------ */

struct tcb{
	int32_t *sp;       /* --------( Virtual Stack Pointer )-------- */
	struct tcb *next;  /* --------( Link To Next Block    )-------- */
	int32_t *blocked;  /* --------( Blocking Field        )-------- */
};

typedef struct tcb tcbType;

tcbType tcbs[NUM_THREADS];
tcbType *runpt;

int32_t stacks[NUM_THREADS][STACK_SIZE];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef void(*ptr_to_func)(void);

/* ----------------( Mail Global Variable & Semaphores )----------------- */

int32_t  Send;      /* --------( Semaphore: Data Available )-------- */
int32_t   Ack;      /* --------( Semaphore: Mailbox Free   )-------- */
int32_t  Mail;      /* --------( The Actual Data Container )-------- */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ------------------------------( FIFO )--------------------------------- */

uint32_t volatile *PutPt;        /* --------( put next )-------- */
uint32_t volatile *GetPt;        /* --------( get next )-------- */
uint32_t static Fifo[FIFOSIZE];
int32_t CurrentSize;             /* --------( Semaphore : 0 means FIFO empty       )-------- */
int32_t RoomLeft;                /* --------( Semaphore : 0 means FIFO full        )-------- */
int32_t FIFOmutex;               /* --------( Semaphore : exclusive access to FIFO )-------- */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ----------------------( Function Declarations )------------------------ */

void OS_Init(void);
void mailbox_init(void);
void OS_InitSemaphore(int32_t *s, int32_t value);
int OS_Add_Threads(ptr_to_func task0, ptr_to_func task1, ptr_to_func task2);
void SetInitialStack(int i);
int32_t Start_Critical(void);
void End_Critical(int32_t primask);
void OS_Launch(uint32_t time_slice);
void Start_OS(void);
void Scheduler(void);
void OS_Wait(int32_t *s);
void OS_Signal(int32_t *s);
void OS_Suspend(void);
void SendMail(int32_t data);
int32_t RecvMail(void);
void OS_Fifo_Init(void);
void OS_Fifo_Put(int32_t data);
int32_t OS_Fifo_Get(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/**************************[ OS Init ]***************************/

// Objective : OS Initialization
// Input     : None
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void OS_Init(void){

    __disable_irq();  /* --------( Block all interrupts during RTOS setup )-------- */
     mailbox_init();  /* --------( Initialize Mailbox Semaphore           )-------- */
     OS_Fifo_Init();  /* --------( Initialize FIFO                        )-------- */
}

/***********************[ MailBox Init ]************************/

// Objective : Mailbox Semaphore Initialization
// Input     : None
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void mailbox_init(void){

	OS_InitSemaphore(&Send, 0); /* --------( No Data Initially )-------- */
	OS_InitSemaphore( &Ack, 1); /* --------( Mailbox Is Free   )-------- */

}

void OS_InitSemaphore(int32_t *s, int32_t value){

  int32_t status = Start_Critical();
  *s = value;
  End_Critical(status);
}

/***********************[ OS Add Threads ]***********************/

// Objective : Create Circular Linked List &
//             Initialize stack of specific thread
// Input     : three pointers to a void/void foreground tasks
// Output    : 1 if successful, 0 if this thread cannot be added

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int OS_Add_Threads(ptr_to_func task0, ptr_to_func task1, ptr_to_func task2){

  int32_t status;
  status = Start_Critical(); /* --------( Turn Off Interrupts )-------- */

  tcbs[0].next = &tcbs[1];  /* --------( TCB0 Points to TCB1 )-------- */
  tcbs[1].next = &tcbs[2];  /* --------( TCB1 Points to TCB2 )-------- */
  tcbs[2].next = &tcbs[0];  /* --------( TCB2 Points to TCB0 )-------- */

  SetInitialStack(0); stacks[0][STACK_SIZE-2] = (int32_t)(task0); // PC
  SetInitialStack(1); stacks[1][STACK_SIZE-2] = (int32_t)(task1); // PC
  SetInitialStack(2); stacks[2][STACK_SIZE-2] = (int32_t)(task2); // PC

  runpt = &tcbs[0];         /* --------( thread 0 will run first )-------- */
  End_Critical(status);		/* --------( Turn On Interrupts      )-------- */

  return 1; // successful
}

/*********************[ Set Initial Stack ]*********************/

// Objective : Initialize Virtual Stack Pointer to thread stack
// Input     : Index(Location) Of Thread
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void SetInitialStack(int i){

  tcbs[i].sp = &stacks[i][STACK_SIZE-16]; // thread stack pointer

  /* --------( Setting Thumb Bit To Configure Thumb Instruction Set )-------- */

  stacks[i][STACK_SIZE-1]  = 0x01000000; /* --------( xPSR   )-------- */

  /* --------( Dummy Values For Diagnostics/Debugging Purpose )-------- */

  stacks[i][STACK_SIZE-3]  = 0x14141414; /* --------( R14 - Link Register     )-------- */
  stacks[i][STACK_SIZE-4]  = 0x12121212; /* --------( R12 - Scratch Register  )-------- */
  stacks[i][STACK_SIZE-5]  = 0x03030303; /* --------( R3  - Argument Register )-------- */
  stacks[i][STACK_SIZE-6]  = 0x02020202; /* --------( R2  - Argument Register )-------- */
  stacks[i][STACK_SIZE-7]  = 0x01010101; /* --------( R1  - Argument Register )-------- */
  stacks[i][STACK_SIZE-8]  = 0x00000000; /* --------( R0  - Argument Register )-------- */

  stacks[i][STACK_SIZE-9]  = 0x11111111; /* --------( R11 - Frame Pointer     )-------- */
  stacks[i][STACK_SIZE-10] = 0x10101010; /* --------( R10 - General purpose   )-------- */
  stacks[i][STACK_SIZE-11] = 0x09090909; /* --------( R9  - Platform Specific )-------- */
  stacks[i][STACK_SIZE-12] = 0x08080808; /* --------( R8  - General purpose   )-------- */
  stacks[i][STACK_SIZE-13] = 0x07070707; /* --------( R7  - General purpose   )-------- */
  stacks[i][STACK_SIZE-14] = 0x06060606; /* --------( R6  - General purpose   )-------- */
  stacks[i][STACK_SIZE-15] = 0x05050505; /* --------( R5  - General purpose   )-------- */
  stacks[i][STACK_SIZE-16] = 0x04040404; /* --------( R4  - General purpose   )-------- */

}

/**************************[  Start Critical  ]**************************/

// Objective : Save Current Interrupt State & Disable Interrupts Globally
// Input     : None
// Output    : Old State Of Interrupts

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int32_t Start_Critical(void){

    int32_t primask = __get_PRIMASK();  /* --------( Save current interrupt state )-------- */
    __disable_irq();                    /* --------( Disable interrupts globally  )-------- */
    return primask;                     /* --------( Return old state             )-------- */
}

/*********************[  End Critical  ]*********************/

// Objective : Enable Interrupts Globally As Per Old State
// Input     : 32 bit integer ( Old State Of Interrupts )
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void End_Critical(int32_t primask){

    __set_PRIMASK(primask);            /* ---------( Restore previous state )-------- */
}

/*********************[  OS Launch  ]*********************/

// Objective : Configure Systick Enable Interrupt And Start Thread
// Input     : 32 bit integer ( Time Slice = SystemFreq/ThreadFreq )
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void OS_Launch(uint32_t time_slice){ //OS_Launch(SystemCoreClock / THREADFREQ);

	NVIC_SetPriorityGrouping(0);	    /* ---------( All Preemption Bits          )-------- */
	NVIC_SetPriority(SysTick_IRQn, 14); /* ---------( Low Priority                 )-------- */

    SysTick->CTRL = 0;                  /* ---------( Disable SysTick During Setup )-------- */
    SysTick->LOAD = time_slice - 1;     /* ---------( Set reload value             )-------- */
    SysTick->VAL  = 0;                  /* ---------( Clear current value          )-------- */

    SysTick->CTRL =
    		 	CLKSOURCE_MASK      |   /* ---------( Processor clock              )-------- */
				TICKINT_MASK        |   /* ---------( Enable SysTick interrupt     )-------- */
				ENABLE_SYSTICK_MASK ;   /* ---------( Start SysTick                )-------- */

    Start_OS();
}

/*************************[ Scheduler ]**************************/

// Objective : Schedule Threads Skip To Next If Blocked
// Input     : None
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void Scheduler(void){
  runpt = runpt->next;      /* ---( Run Next Thread Not Blocked )--- */
  while(runpt->blocked){    /* ---( Skip If Blocked             )--- */
    runpt = runpt->next;
  }
}

/* -----------------------( SpinLock Semaphore )------------------------ */

/*************************[ OS Wait ]**************************/

// Objective : Spin Till Condition Satisfied
// Input     : Address Of int32_t Semaphore
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*void OS_Wait(int32_t *s){

	int32_t status = Start_Critical();

	while((*s) == 0){
		End_Critical(status);
		OS_Suspend();
		status = Start_Critical();
	}
	(*s) = (*s) - 1;
	End_Critical(status);
}*/

void OS_Wait(int32_t *s){

	int32_t status = Start_Critical();

	(*s) = (*s) - 1;
	if((*s) < 0){
    runpt->blocked = s;     /* ---( Reason It Is Blocked )--- */
    End_Critical(status);
    OS_Suspend();           /* ---( Thread Switcher      )--- */
  }
	End_Critical(status);
}

/*************************[ OS Signal ]**************************/

// Objective : Satisfy Condition for OS_Wait
// Input     : Address Of int32_t Semaphore
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*void OS_Signal(int32_t *s){

  int32_t status = Start_Critical();
  (*s) = (*s) + 1;
  End_Critical(status);
}*/

void OS_Signal(int32_t *s){

  tcbType *pt;
  int32_t status = Start_Critical();
  (*s) = (*s) + 1;
  if((*s) <= 0){
    pt = runpt->next;         /* ---( Search For A Thread Blocked On This Semaphore )--- */
    while(pt->blocked != s){
      pt = pt->next;
    }
    pt->blocked = NULL;       /* ---( Wake This One )--- */
  }
  End_Critical(status);
}

void OS_Suspend(void){

	SCB->ICSR = (1 << 26) ; /* ---( Trigger SysTick, But Not Reset Timer )--- */
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* -----------------------( Mail Send & Receive )------------------------ */

/*************************[ Send Mail ]**************************/

// Objective : Receive Data And Substitute in Mail Wait
//             As Long As Mail Is Full
// Input     : int32_t Data
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void SendMail(int32_t data){
    Mail = data;
    OS_Signal(&Send);
    OS_Wait(&Ack);
}

/************************[ Receive Mail ]************************/

// Objective : Send Data If Mail Is Full
//             As Long As Mail Is Full
// Input     : None
// Output    : int32_t Data

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int32_t RecvMail(void){

    int32_t data;
    OS_Wait(&Send);
    data = Mail;
    OS_Signal(&Ack);
    return data;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/************************[ OS_Fifo_Init ]************************/

// Objective : Initiates Fifo
// Input     : None
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void OS_Fifo_Init(void){

	PutPt = GetPt = &Fifo[0]; // Empty
	OS_InitSemaphore(&CurrentSize, 0);
	OS_InitSemaphore(&RoomLeft, FIFOSIZE);
    OS_InitSemaphore(&FIFOmutex, 1);

}

/************************[ OS_Fifo_Put ]************************/

// Objective : Write To Fifo
// Input     : int32_t data
// Output    : None

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void OS_Fifo_Put(int32_t data){

	OS_Wait(&RoomLeft);
	OS_Wait(&FIFOmutex);
	*(PutPt) = data;               /* ---( Put data )--- */
	PutPt++;
	if(PutPt == &Fifo[FIFOSIZE]){
		PutPt = &Fifo[0];          /* ---(   Wrap   )--- */
	}
	OS_Signal(&FIFOmutex);
	OS_Signal(&CurrentSize);

}

/************************[ OS_Fifo_Put ]************************/

// Objective : Read From Fifo
// Input     : None
// Output    : int32_t data

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int32_t OS_Fifo_Get(void){

	uint32_t data;
    OS_Wait(&CurrentSize);
    OS_Wait(&FIFOmutex);
    data = *(GetPt);     /* ---( Get Data                   )--- */
    GetPt++;             /* ---( Points To Next Data To Get )--- */

    if(GetPt == &Fifo[FIFOSIZE]){
      GetPt = &Fifo[0];  /* ---( Wrap )--- */
    }

    OS_Signal(&FIFOmutex);
    OS_Signal(&RoomLeft);
    return data;
}
