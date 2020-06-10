// test user-level fault handler -- just exit when we fault

#include <inc/lib.h>

void
div_zero_handler(struct UTrapframe *utf)
{
	uint32_t err = utf->utf_err;
	cprintf("DIV BY ZERO PLEASEEE, error: %x\n", err & 7);
	sys_env_destroy(sys_getenvid());
}

void
umain(int argc, char **argv)
{
	cprintf("Let me div by zero.\n");
	set_exception_handler(T_DIVIDE, div_zero_handler);
	int zero = 0;
	int illegal =  1/zero;
}
