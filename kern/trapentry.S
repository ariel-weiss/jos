/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
  .text;             \
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
  jmp _alltraps;															\
  .data;																	\
  .long name

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
  .text;             \
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
  jmp _alltraps;															\
  .data;																	\
  .long name


/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
.data
.global trap_handlers
trap_handlers:

TRAPHANDLER_NOEC(divide,T_DIVIDE)
TRAPHANDLER_NOEC(debug,T_DEBUG)
TRAPHANDLER_NOEC(nmi,T_NMI)
TRAPHANDLER_NOEC(brkpt,T_BRKPT)
TRAPHANDLER_NOEC(oflow,T_OFLOW)
TRAPHANDLER_NOEC(bound,T_BOUND)
TRAPHANDLER_NOEC(illop,T_ILLOP)
TRAPHANDLER_NOEC(device,T_DEVICE)
TRAPHANDLER(dblflt,T_DBLFLT)
TRAPHANDLER_NOEC(corproc,9) /* 9 */
TRAPHANDLER(tss,T_TSS)
TRAPHANDLER(segnp,T_SEGNP)
TRAPHANDLER(stack,T_STACK)
TRAPHANDLER(gpflt,T_GPFLT)
TRAPHANDLER(pgflt,T_PGFLT)
TRAPHANDLER_NOEC(res,15) /* 15 */
TRAPHANDLER_NOEC(fperr,T_FPERR)
TRAPHANDLER_NOEC(align,T_ALIGN)
TRAPHANDLER_NOEC(mchk,T_MCHK)
TRAPHANDLER_NOEC(simderr,T_SIMDERR)   /*19*/
TRAPHANDLER_NOEC(hh1, 20)
TRAPHANDLER_NOEC(hh2, 21)
TRAPHANDLER_NOEC(hh3, 22)
TRAPHANDLER_NOEC(hh4, 23)
TRAPHANDLER_NOEC(hh5, 24)
TRAPHANDLER_NOEC(hh6, 25)
TRAPHANDLER_NOEC(hh7, 26)
TRAPHANDLER_NOEC(hh8, 27)
TRAPHANDLER_NOEC(hh9, 28)
TRAPHANDLER_NOEC(hhh1, 29)
TRAPHANDLER_NOEC(hhh2, 30)
TRAPHANDLER_NOEC(hhh3, 31)
TRAPHANDLER_NOEC(hhh4, 32)
TRAPHANDLER_NOEC(hhh5, 33)
TRAPHANDLER_NOEC(hhh6, 34)
TRAPHANDLER_NOEC(hhh7, 35)
TRAPHANDLER_NOEC(hhh8, 36)
TRAPHANDLER_NOEC(hhh9, 37)
TRAPHANDLER_NOEC(hhhh1, 38)
TRAPHANDLER_NOEC(hhhh2, 39)
TRAPHANDLER_NOEC(hhhh3, 40)
TRAPHANDLER_NOEC(hhhh4, 41)
TRAPHANDLER_NOEC(hhhh5, 42)
TRAPHANDLER_NOEC(hhhh6, 43)
TRAPHANDLER_NOEC(hhhh7, 44)
TRAPHANDLER_NOEC(hhhh8, 45)
TRAPHANDLER_NOEC(hhhh9, 46)
TRAPHANDLER_NOEC(ariel1, 47)
TRAPHANDLER_NOEC(hsyscall, T_SYSCALL)


/*
 * Lab 3: Your code here for _alltraps
 */
 _alltraps:
 # Build trap frame.
 pushl %ds
 pushl %es
 pushal

 # Set up data and per−cpu segments.  TODO: maybe bug
 movw $GD_KD, %ax
 movw %ax, %ds
 movw %ax, %es

 # Call trap(tf), where tf=%esp
 pushl %esp
 call trap
