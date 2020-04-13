// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Backtrace", mon_backtrace },
	{ "showmaps", "Display all of the physical page mappings", mon_showmaps },
	{ "setprems", "Set premissions of given mapping. [11 -> UW, 10 -> U, 01 -> W, 00 -> N/A] ", mon_setprems },
	{ "padump", "Dump memory content from given *PHYSICAL* address", mon_padump },
	{ "vadump", "Dump memory content from given *VIRTUAL* address", mon_vadump },
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;
	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	//ebp f0109e58  eip f0100a62  args 00000001 f0109e80 f0109e98 f0100ed2 00000031
	// Your code here.
	cprintf("Stack backtrace:\n");
	uint32_t* cur_ebp = (uint32_t*)read_ebp();
	uint32_t cur_eip, arg_1, arg_2, arg_3, arg_4, arg_5;
	struct Eipdebuginfo dgi;

	while(cur_ebp != 0x0){
		cur_eip = *((uint32_t*)((uint32_t)cur_ebp + (4*1)));
		arg_1 = *((uint32_t*)((uint32_t)cur_ebp + (4*2+0)));
		arg_2 = *((uint32_t*)((uint32_t)cur_ebp + (4*2+1)));
		arg_3 = *((uint32_t*)((uint32_t)cur_ebp + (4*2+2)));
		arg_4 = *((uint32_t*)((uint32_t)cur_ebp + (4*2+3)));
		arg_5 = *((uint32_t*)((uint32_t)cur_ebp + (4*2+4)));
		cprintf("ebp %08x eip %08x args %08x %08x %08x %08x %08x\n",cur_ebp,cur_eip,arg_1,arg_2,arg_3,arg_4,arg_5);
		cur_ebp = (uint32_t*) *cur_ebp;

		if(debuginfo_eip(cur_eip, &dgi) == 0){
			 cprintf("\t   %s:%d: %.*s+%d\n",dgi.eip_file, dgi.eip_line,
			 dgi.eip_fn_namelen, dgi.eip_fn_name, cur_eip - dgi.eip_fn_addr);
		}
	}
	return 0;
}
int
mon_showmaps(int argc, char **argv, struct Trapframe *tf)
{

	if(argc<2 || argc>3){
			cprintf("Wrong arguments!!!\n");
			return 0;
	}
	uint32_t low_vaddr = (uint32_t)strtol(argv[1], NULL, 16);
	uint32_t high_vaddr;
	if(argc<3){
		high_vaddr = low_vaddr + PGSIZE;
	}else{
		high_vaddr = (uint32_t)strtol(argv[2], NULL, 16);
	}
	pte_t * cuu_page;
	uint32_t addr;
	uint32_t prem;
	uint32_t i=0;
	char prem_str[4] = {'-','-','-'};
	for(i=low_vaddr; i<high_vaddr; i += PGSIZE){
		cuu_page = pgdir_walk(kern_pgdir, (void*)i, 0);
		if(!cuu_page) {
					cprintf("Page ###: \tVA:%8x \tMapping: NONE \tPremissions: --- \n",i);
					continue;
		}
		addr = PGNUM(*cuu_page) << PTXSHIFT;
		prem = PGOFF(*cuu_page);
		if (prem & PTE_P) prem_str[0] = 'P';
		if (prem & PTE_W) prem_str[1] = 'W';
		if (prem & PTE_U) prem_str[2] = 'U';
		cprintf("Page #%3u: \tVA:%8x \tMapping: %8x \tPremissions: %s \n",PGNUM(*cuu_page),i,addr,prem_str);
		prem_str[0] = '-';prem_str[1] = '-';prem_str[2] = '-';
	}

	return 0;
}
int
mon_setprems(int argc, char **argv, struct Trapframe *tf)
{
	if(argc != 3 ){
			cprintf("Wrong arguments!!!\n");
			return 0;
	}
	uint32_t given_prems = (uint32_t)strtol(argv[2], NULL, 10);
	if (given_prems != 10 && given_prems != 11 && given_prems != 1 && given_prems != 0){
		cprintf("Wrong premissions!!!\n");
		return 0;
	}

	pte_t * cuu_page;
	uint32_t given_va = (uint32_t)strtol(argv[1], NULL, 16);

	uint32_t prem;
	cuu_page = pgdir_walk(kern_pgdir, (void*)given_va, 0);
	if(!cuu_page || *cuu_page == 0x0) {
		cprintf("VA: %x is not exist",given_va);
		return 0;
	}
	switch(given_prems){
		case 11: {*cuu_page = *cuu_page | PTE_W | PTE_U; break; }
		case 10: {*cuu_page = (*cuu_page | PTE_U) & (~PTE_W); break; }
		case 1: {*cuu_page = (*cuu_page | PTE_W) & (~PTE_U); break;}
		case 0: {*cuu_page = (*cuu_page & (~PTE_W)) & (~PTE_U); break;}
		default: return 0;
	}

	mon_showmaps(2,argv,tf);
	return 0;
}
int
mon_vadump(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t flag = 0;
	uint32_t low_vaddr = (uint32_t)strtol(argv[1], NULL, 16);
	uint32_t high_vaddr;
	uint32_t size;
	if(argc<2 || argc>4){
			cprintf("Wrong arguments!!!\n");
			return 0;
	}
	if(argc<3){
		high_vaddr = low_vaddr + 1;
	}
	if(argc>=3){
		high_vaddr = (uint32_t)strtol(argv[2], NULL, 16);
	}
	if(argc == 4){
		if(*argv[3] == 'y' || *argv[3] == 'Y' || *argv[3] == 't' || *argv[3] == 'T'){
			flag = 1;
		}
	}
	pte_t * cuu_page;
	uint32_t addr,addr_end;
	uint32_t i=0,j=0,k=0;
	for(i=low_vaddr; i<high_vaddr; i += (PGSIZE-PGOFF(i))){
		cuu_page = pgdir_walk(kern_pgdir, (void*)i, 0);
		if(!cuu_page || *cuu_page == 0x0) {
					cprintf("VA[%8x] is not mapped!\n",i);
					continue;
		}
		addr = PGNUM(*cuu_page) << PTXSHIFT;
		addr += PGOFF(i);
		if (i + (PGSIZE-PGOFF(i)) >= high_vaddr){
			size = high_vaddr - i;
			addr_end = addr + size;
		}
		else{
			addr_end = addr + (PGSIZE-PGOFF(i));
		}
		for(j=addr; j<addr_end; j +=4){
			k=(PGNUM(i) << PTXSHIFT) + PGOFF(j);
			if (flag){
				cprintf("%x:\t%x\t\n",k,*(uint32_t*)(j+KERNBASE));
			}
			else{
				cprintf("%8x:\t%2x\t",k,*(uint32_t*)(j+KERNBASE) & 0xFF);  //extract first byte
				cprintf("%8x:\t%2x\t",k+1,*(uint32_t*)(j+KERNBASE)>>8 & 0xFF); //extract second byte
				cprintf("%8x:\t%2x\t",k+2,*(uint32_t*)(j+KERNBASE)>>16 & 0xFF); //extract third byte
				cprintf("%8x:\t%2x\n",k+3,*(uint32_t*)(j+KERNBASE)>>24 & 0xFF); //extract fourth byte
			}
		}
	}
	return 0;
}

int
mon_padump(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t flag=0;
	if(argc<2 || argc>4){
			cprintf("Wrong arguments!!!\n");
			return 0;
	}
	if(argc == 4){
		if(*argv[3] == 'y' || *argv[3] == 'Y' || *argv[3] == 't' || *argv[3] == 'T'){
			flag = 1;
		}
	}
	uint32_t low_paddr = (uint32_t)strtol(argv[1], NULL, 16);
	uint32_t high_paddr;
	if(argc<3){
		high_paddr = low_paddr + 1;
	}else{
		high_paddr = (uint32_t)strtol(argv[2], NULL, 16);
	}
	pte_t * cuu_page;
	uint32_t addr;
	int i;
	for(i=low_paddr; i<high_paddr; i +=4){
		if (flag){
			cprintf("%x:\t%x\t\n",i,*(uint32_t*)(i+KERNBASE));
		}
		else{
			cprintf("%x:\t%x\t",i,*(uint32_t*)(i+KERNBASE) & 0xFF);  //extract first byte
			cprintf("%x:\t%x\t",i+1,*(uint32_t*)(i+KERNBASE)>>8 & 0xFF); //extract second byte
			cprintf("%x:\t%x\t",i+2,*(uint32_t*)(i+KERNBASE)>>16 & 0xFF); //extract third byte
			cprintf("%x:\t%x\n",i+3,*(uint32_t*)(i+KERNBASE)>>24 & 0xFF); //extract fourth byte
		}

	}


	return 0;
}
/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
