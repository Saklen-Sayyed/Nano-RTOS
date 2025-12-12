.syntax unified
.cpu cortex-m4
.thumb

.global SysTick_Handler
.type SysTick_Handler, %function

.extern Scheduler

SysTick_Handler:	 /* --------( Processor Saves R0-R3,R12,LR,PC,PSR  )-------- */

	CPSID I			 /* --------( Prevent Interrupt During Switch      )-------- */

	PUSH {R4-R11}	 /* --------( Save Remaining Regs R4-R11           )-------- */
	LDR R0 , =runpt	 /* --------( R0=Pointer To runpt, Old Thread      )-------- */
	LDR R1 ,[R0]	 /* --------( R1 = runpt                           )-------- */
	STR SP ,[R1]	 /* --------( Save SP into TCB                     )-------- */
  //LDR R1 ,[R1,#4]	 /* --------( R1 = runpt->next                     )-------- */
  //STR R1 ,[R0]	 /* --------( RunPt = R1                           )-------- */
	PUSH {R0,LR}	 /* --------( Save R0(Address of runpt & LR)       )-------- */
	BL Scheduler     /* --------( Function Call in OS.c                )-------- */
	POP {R0,LR}      /* --------( Restore R0(Address of runpt & LR)    )-------- */
	LDR R1, [R0]     /* --------( R1 = &runpt->sp                      )-------- */
	LDR SP ,[R1]	 /* --------( New Thread SP; SP = runpt->sp;       )-------- */
	POP {R4-R11}	 /* --------( Restore Regs R4-R11                  )-------- */

	CPSIE I			 /* --------( Tasks Run With Interrupts Enabled    )-------- */
	BX LR			 /* --------( Restore R0-R3,R12,LR,PC,PSR          )-------- */


.global Start_OS
.type Start_OS, %function

Start_OS:

    LDR R0, =runpt /* --------( Currently Running Thread             )-------- */
	LDR R1,[R0]	   /* --------( R1 = Value Of runpt                  )-------- */
	LDR SP,[R1]    /* --------( New Thread SP; SP = runpt->sp;       )-------- */
	POP {R4-R11}   /* --------( Restore Regs R4-R11                  )-------- */
	POP {R0-R3}    /* --------( Restore Regs R0-R3                   )-------- */
	POP {R12}      /* --------( Restore Regs R12                     )-------- */
	ADD SP,SP,#4   /* --------( Discard LR From Initial Stack        )-------- */
	POP {LR}       /* --------( Start Location                       )-------- */
	ADD SP,SP,#4   /* --------( Discard PSR                          )-------- */

	CPSIE I        /* --------( Enable Interrupts At Processor Level )-------- */
	BX LR          /* --------( Start First Thread                   )-------- */
