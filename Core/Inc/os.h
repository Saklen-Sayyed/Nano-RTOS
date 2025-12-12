/*
 * os.h
 *
 *  Created on: Nov 28, 2025
 *      Author: Saklen
 */

#ifndef OS_H
#define OS_H
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

#endif
