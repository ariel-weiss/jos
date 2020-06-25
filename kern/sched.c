#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

void sched_halt(void);
void train_classifier(){
	classifier_ready = false;
	int i,j,sum;
	bool y;
	int iterations = 0;
	bool error = true;
	int maxIterations = 10000;
	double alpha = 0.01;
	int sign;

	while (error && iterations < maxIterations)
	{
		error = false;
		for (i = 0; i < classifier_data_index ; i++)
		{
		    sum = 0;
	    for(j=0;j<50;j++){
	        sum+=classifier_data[i][j] * weight_arr[j];
	    }


			if (sum < 0)
			{
				y = false;
				sign = 1;
			}
			else
			{
				y = true;
				sign = -1;
			}

			if (y != labals[i])
			{
				error = true;
				for(j=0;j<50;j++){
				    weight_arr[j] += sign * alpha  * classifier_data[i][j] / 2;
					}

			}
		}

		iterations++;
	}
	classifier_ready = true;
}
// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *idle;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// LAB 4: Your code here.



		int32_t id,i;

		if (!curenv){
			id = -1 ;
		}
		else{
			id = ENVX(curenv->env_id);
		}

		for(i=id+1;i<NENV;i++){
			if(envs[i].env_status == ENV_RUNNABLE){
				env_run(&envs[i]); //ContextSwitch

				return; //Should not return
			}
		}
		for(i=0;i<id;i++){
			if(envs[i].env_status == ENV_RUNNABLE){
				env_run(&envs[i]); //ContextSwitch
				return; //Should not return
			}
		}
		if(curenv && curenv->env_status == ENV_RUNNING){
			env_run(curenv); //ContextSwich

			return; //Should not return
		}



	// sched_halt never returns
	sched_halt();
}


// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_NOT_RUNNABLE || // Environment might be waiting for IPC or network
		     envs[i].env_status == ENV_DYING
		     ) && envs[i].env_type == ENV_TYPE_USER )
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));
	/* Train our classifier: */
	if (cpunum() == 0){
			train_classifier();
	}
	/* ===================== */

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock

	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		"sti\n"
		"1:\n"
		"hlt\n"
		"jmp 1b\n"
	: : "a" (thiscpu->cpu_ts.ts_esp0));
}
