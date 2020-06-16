/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>
#include <kern/time.h>
#include <kern/e1000.h>


// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(curenv,s,len,PTE_U|PTE_P);
	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.
	// LAB 4: Your code here.
	struct Env* new_env;
	int res = env_alloc(&new_env,curenv->env_id);
	if(res < 0) return res;
	envid_t	aba_id = curenv->env_id;
  memcpy(&new_env->env_tf,&curenv->env_tf,sizeof(struct Trapframe));
	new_env->env_tf.tf_regs.reg_eax = 0;
	new_env->env_status = ENV_NOT_RUNNABLE;
	return new_env->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	if((status != ENV_RUNNABLE)&&(status != ENV_NOT_RUNNABLE)) return -E_INVAL;
	struct Env* env_to_change;
	int res = envid2env(envid,&env_to_change,1);
	if(res < 0) return -E_BAD_ENV;
	env_to_change->env_status = status;
	return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 5: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!

	struct Env* our_env;
	int res = envid2env(envid,&our_env,1);
	if(res < 0) return -E_BAD_ENV;
	user_mem_assert(our_env, (void *) tf, sizeof(struct Trapframe), PTE_U);
	tf->tf_cs |= 0x3;  // TODO: maybe the BUG is here
	tf->tf_eflags |= FL_IF;
	our_env->env_tf = *tf;
	return 0;

}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	struct Env* env;
	int res = envid2env(envid,&env,1);
	if(res < 0) return -E_BAD_ENV;
	env->env_upcalls[T_PGFLT] = func;
	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	if(((perm & (~PTE_SYSCALL)) != 0) ||
	 	!(perm & PTE_P) || (!(perm & PTE_U))) return -E_INVAL;
	if((uint32_t)va >= UTOP || va != ROUNDDOWN(va, PGSIZE) ) return -E_INVAL;
	struct Env* env;
	int res = envid2env(envid,&env,1);
	if(res < 0) return -E_BAD_ENV;
	struct PageInfo* pagey = page_alloc(ALLOC_ZERO);
	if(!pagey) return -E_NO_MEM;
	res = page_insert(env->env_pgdir, pagey, va, perm);
	if(res < 0){
		page_free(pagey);
		return -E_NO_MEM;
	}
	return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	if(((perm & (~PTE_SYSCALL)) != 0) ||
	 	!(perm & PTE_P) || (!(perm & PTE_U))) return -E_INVAL;
	if((uint32_t)srcva >= UTOP || srcva != ROUNDDOWN(srcva, PGSIZE) ) return -E_INVAL;
	if((uint32_t)dstva >= UTOP || dstva != ROUNDDOWN(dstva, PGSIZE) ) return -E_INVAL;
	struct Env* src_env;
	struct Env* dst_env;
	int res = envid2env(srcenvid,&src_env,1);
	if(res < 0) return -E_BAD_ENV;

	res = envid2env(dstenvid,&dst_env,1);
	if(res < 0) return -E_BAD_ENV;

	//Page lookup
	pte_t* pte_store;
	struct PageInfo* pagey = page_lookup(src_env->env_pgdir, srcva,&pte_store);
	if(!pagey) return -E_INVAL;
	if(!(*pte_store & PTE_W) && (perm & PTE_W)) return -E_INVAL;
	//Page insert
	res = page_insert(dst_env->env_pgdir, pagey, dstva, perm);
	if(res < 0){
		return -E_NO_MEM;
	}
	return 0;
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	if((uint32_t)va >= UTOP || va != ROUNDDOWN(va, PGSIZE) ) return -E_INVAL;
	struct Env* env;
	int res = envid2env(envid,&env,1);
	if(res < 0) return -E_BAD_ENV;
	page_remove(env->env_pgdir,va);
	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	int32_t r;
	struct Env * dst_env;
	r = envid2env(envid,&dst_env,0);
	if(r<0) return -E_BAD_ENV;
	if(dst_env->env_ipc_recving != 1) return -E_IPC_NOT_RECV;

	pte_t *src_pte;
	unsigned new_perm = 0;
	//Handling Page Sending
	if((uint32_t)srcva < UTOP){

		if(((perm & (~PTE_SYSCALL)) != 0) ||
		 	!(perm & PTE_P) || (!(perm & PTE_U))) return -E_INVAL;
		if(srcva != ROUNDDOWN(srcva, PGSIZE) ) return -E_INVAL;
		struct PageInfo * pp = page_lookup(curenv->env_pgdir, srcva, &src_pte);
		if(!pp) return -E_INVAL;
		if ((perm & PTE_W) && !(*src_pte & PTE_W)) return -E_INVAL;

		if ((uint32_t)dst_env->env_ipc_dstva < UTOP){
			//Page insert
			r = page_insert(dst_env->env_pgdir, pp, dst_env->env_ipc_dstva, perm);
			if(r < 0)	return -E_NO_MEM;
			new_perm = perm;
		}//Else, continue
	}
	//Handling Value Sending

	//    env_ipc_recving is set to 0 to block future sends;
	dst_env->env_ipc_recving = 0;
	//    env_ipc_from is set to the sending envid;
	dst_env->env_ipc_from = curenv->env_id;
	//    env_ipc_value is set to the 'value' parameter;
	dst_env->env_ipc_value = value;
	//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
	dst_env->env_ipc_perm = new_perm;
	// The target envireonment is marked runnable again, returning 0
	dst_env->env_tf.tf_regs.reg_eax = 0;
	dst_env->env_status = ENV_RUNNABLE;
	return 0;

}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	if ((uint32_t)dstva < UTOP && dstva != ROUNDDOWN(dstva,PGSIZE)) return -E_INVAL;
	curenv->env_status = ENV_NOT_RUNNABLE;
	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	sys_yield();
	panic("Error in sys_ipc_recv: should never get here\n");

		return 0;
	}

static int
sys_env_set_upcall(envid_t envid, uint32_t trapno, void *func)
{
	// LAB 4: Your code here.
	struct Env * env;
	if(envid2env(envid, &env, 1)) return -E_BAD_ENV;
	if(trapno >= 0 && trapno <= 15)
		env->env_upcalls[trapno] = func;
	return 0;
}


// Return the current time.
static int
sys_time_msec(void)
{
	// LAB 6: Your code here.
        return time_msec();
}


/**
* Make sure the elf is valid before using this syscall
**/
static void inner_exec(struct Proghdr * ph,struct Proghdr *eph,void* code){
	for (; ph < eph; ph++) {
		if(ph->p_type == ELF_PROG_LOAD) {
			region_alloc(curenv, (void *) ph->p_va, ph->p_memsz);

			memset((void *) ph->p_va, 0, ph->p_memsz);
			memcpy((void *) ph->p_va, code + ph->p_offset, ph->p_filesz);
		}
	}
}

static envid_t
sys_exec(void* code, const char **argv) {
	struct  Elf * head = (struct Elf *) code;
	uint32_t arguments_num;
	size_t string_size = 0;
	char arg_buf[100];
	char * strings;
	uintptr_t *argvs;
	struct Proghdr *ph;
	struct Proghdr *eph;

	// load each program segment (ignores ph flags)
	ph = (struct Proghdr *) ((uint8_t *) head + head->e_phoff);
	eph = ph + head->e_phnum;
	inner_exec(ph,eph,code);

	for(arguments_num = 0; argv[arguments_num] != 0; arguments_num++) {
		if(string_size + strlen(argv[arguments_num]) >= 100) break;
		strcpy(arg_buf + string_size, argv[arguments_num]);
		string_size += strlen(argv[arguments_num]) + 1;
		arg_buf[string_size - 1] = '\0';
	}

	strings = (char *) USTACKTOP - string_size;
	argvs = (uintptr_t *)(ROUNDDOWN(strings, 4) - 4 * (arguments_num + 1));

	if((void *)(argvs - 2) < (void *) USTACKTOP - PGSIZE)
		return - E_NO_MEM;

	uint32_t i;
	char *p = arg_buf;
	for(i = 0; i < arguments_num; i++) {
		argvs[i] = (intptr_t) strings;
		strcpy(strings, p);
		strings += strlen(p) + 1;
		p += strlen(p) + 1;
	}
	argvs[arguments_num] = 0;
	argvs[-1] = (intptr_t) argvs;
	argvs[-2] = arguments_num;

	curenv->env_tf.tf_eip = head->e_entry;
	curenv->env_tf.tf_esp = (intptr_t) &argvs[-2];

	sched_yield();

}
static int
sys_send_packet(void *srcva, size_t len)
{
    if (user_mem_check(curenv, srcva, len, PTE_U) < 0)
        return -E_INVAL;

    return E1000_transmit(srcva, len);
}
static int
sys_recv_packet(void *dstva, uint16_t *len_store)
{
    if (user_mem_check(curenv, dstva, E1000_ETH_PACKET_LEN, PTE_U|PTE_W) < 0)
        return -E_INVAL;

		int r = E1000_receive(dstva, len_store);
		if(r == 0 ) return 0;
		curenv->env_status = ENV_NOT_RUNNABLE;
		curenv->e1000_waiting = true;
		curenv->env_tf.tf_regs.reg_eax = -E_RXD_EMPTY;
		sys_yield();
    return r;
}
// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.
	if(syscallno >= NSYSCALLS) return -E_INVAL;
	//panic("syscall not implemented");


	switch (syscallno) {
		case SYS_cputs:
		{ sys_cputs((char*)a1,a2); return 0; }
		case SYS_cgetc:
		{ return sys_cgetc(); }
		case SYS_getenvid:
		{ return sys_getenvid(); }
		case SYS_env_destroy:
		{ return sys_env_destroy((envid_t)a1); }
		case SYS_yield:
		{ sys_yield(); return 0;}
		case SYS_exofork:
		{ return sys_exofork(); }
		case SYS_env_set_status:
		{ return sys_env_set_status((envid_t)a1,(int)a2); }
		case SYS_page_alloc:
		{ return sys_page_alloc((envid_t)a1,(void*)a2,(int)a3); }
		case SYS_page_map:
		{ return sys_page_map((envid_t) a1, (void*)a2,(envid_t) a3, (void*)a4, (int)a5);}
		case SYS_page_unmap:
		{ return sys_page_unmap((envid_t) a1,(void*)a2); }
		case SYS_env_set_pgfault_upcall:
		{ return sys_env_set_pgfault_upcall((envid_t) a1,(void*)a2); }
		case SYS_env_set_upcall:
		{ return sys_env_set_upcall((envid_t) a1,(uint32_t) a2,(void*)a3); }
		case SYS_ipc_recv:
		{ return sys_ipc_recv((void*) a1);}
		case SYS_ipc_try_send:
		{ return sys_ipc_try_send((envid_t) a1, (uint32_t)a2,(void*) a3, (unsigned)a4);}
		case SYS_env_set_trapframe:
		{ return sys_env_set_trapframe((envid_t) a1,(struct Trapframe *)a2);  }
		case SYS_exec:
		{ return sys_exec((void *) a1, (const char **) a2); }
		case SYS_time_msec:
            return sys_time_msec();
		case SYS_send_packet:
						return sys_send_packet((void *) a1, (size_t) a2);
		case SYS_recv_packet:
						return sys_recv_packet((void *) a1, (uint16_t *) a2);

	default:
		return -E_INVAL;
	}
}
