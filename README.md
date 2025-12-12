<!-- ========================================================= -->
<!--                       PROJECT BANNER                     -->
<!-- ========================================================= -->

<p align="center">
  <img src="https://via.placeholder.com/1200x250/0d1117/00d9ff?text=Mini+RTOS+on+STM32F446RE+%7C+Built+From+Scratch" />
</p>

<h1 align="center">Mini RTOS From Scratch on STM32F446RE</h1>
<p align="center"><i>A preemptive round-robin scheduler + semaphores + mailbox + FIFO + ARM Cortex-M4 context switching — all built from scratch.</i></p>

---

<!-- ========================================================= -->
<!--                           BADGES                          -->
<!-- ========================================================= -->

<p align="center">
  <img src="https://img.shields.io/badge/Language-C-blue.svg" />
  <img src="https://img.shields.io/badge/MCU-STM32F446RE-brightgreen.svg" />
  <img src="https://img.shields.io/badge/Architecture-ARM%20Cortex--M4-orange.svg" />
  <img src="https://img.shields.io/badge/RTOS-Custom%20Kernel-red.svg" />
  <img src="https://img.shields.io/badge/Assembly-ARM%20Thumb2-yellow.svg" />
  <img src="https://img.shields.io/badge/License-MIT-lightgrey.svg" />
</p>

---

# ⭐ Overview

This repository contains a **fully functional mini-RTOS implemented from scratch** for the **STM32F446RE (ARM Cortex-M4)**.  
Before starting this project, I had:

❌ Zero RTOS knowledge  
❌ Zero understanding of context switching  
❌ Zero idea about thread stacks, semaphores, and scheduling  
❌ Only basic STM32 bare-metal experience  

This project transformed all of that.  
I implemented:

- A **preemptive round-robin scheduler**
- Independent **thread stacks**
- **Blocking semaphores**
- **Mailbox communication**
- **FIFO with mutex + semaphores**
- **SysTick-driven context switching**
- **ARM Cortex-M4 assembly context switcher**

This project gave me a REAL understanding of how an RTOS works under the hood.

---

# 🚀 Journey / Learning Timeline

## 1️⃣ Starting With Zero RTOS Knowledge

When I started this project, I did not know:
- How schedulers work  
- How context switching works  
- What a TCB is  
- How semaphores block or wake tasks  
- How ARM Cortex-M handles interrupts  

So I first learned:
- Exception entry/exit on ARM  
- xPSR, LR, PC stacking  
- SysTick interrupt mechanism  
- NVIC priority grouping  

This foundational ARM knowledge became crucial for the assembly code.

---

# 2️⃣ Understanding ARM Cortex-M Architecture

ARM automatically pushes these registers on interrupt entry:
R0, R1, R2, R3, R12, LR, PC, xPSR

The RTOS must manually save:
R4–R11

This led me to implement:
```asm
PUSH {R4-R11}

and restore later.
This was the hardest conceptual part.

**# 3️⃣ Creating Thread Stacks**
Each thread gets its own stack (size: 400 bytes).
The initial thread frame is manually built:

stacks[i][STACK_SIZE-1] = 0x01000000;  // xPSR (Thumb bit)
stacks[i][STACK_SIZE-2] = (int32_t)taskX; // PC (thread entry)

-This taught me:
-Stack alignment
-Register order
-Thread entry mechanics
-Why xPSR must have the Thumb bit set

**# 4️⃣ Implementing the Scheduler**

Using a circular linked list of TCBs:
runpt = runpt->next;
while(runpt->blocked) {
    runpt = runpt->next;
}

This allowed:

-Round-robin scheduling
-Skipping blocked threads
This was my first real scheduler implementation.

**# 5️⃣ Designing Semaphores**

Implemented blocking semaphores:

(*s)--;
if(*s < 0) {
    runpt->blocked = s;
    OS_Suspend();
}
This forced me to understand:

-Blocking vs spinlock
-Critical sections
-Interrupt masking with PRIMASK
-Why semaphores unblock exactly one thread
I also implemented full semaphore signaling with unblocking logic.

**#6️⃣ Implementing FIFO and Mailbox**

FIFO required:
-Two semaphores (CurrentSize, RoomLeft)
-One mutex (FIFOmutex)
-Circular buffer logic with wrap-around

Mailbox required:
-Send semaphore
-Ack semaphore

These taught me:
-Resource sharing
-Mutual exclusion
-Producer-consumer patterns
-Preventing race conditions

**#7️⃣ ARM Assembly Context Switcher**

My favorite part of the RTOS.
1.SysTick handler performs:
2.Save old thread context
3.Save current SP to its TCB
4.Call scheduler
5.Load new thread SP
6.Restore registers
7.Exit interrupt → new thread runs
This is where the OS truly “comes alive.”

**#🧠 What I Learned**

**ARM Fundamentals**
-Exception entry auto-stacking
-Manual register saving
-R4–R11 preservation requirement
-xPSR Thumb bit importance
-NVIC priority grouping

**RTOS Core Concepts**
-Thread Control Block (TCB)
-Per-thread stack initialization
-Round-robin vs blocked thread skipping
-Preemptive vs cooperative scheduling

**Synchronization**
-Semaphores
-Mutexes
-FIFO queues
-Mailbox signaling logic

**Embedded Design Philosophy**
-Deterministic execution
-Correct critical section design
-Minimal overhead
-Thinking like an RTOS designer, not an RTOS user
  
