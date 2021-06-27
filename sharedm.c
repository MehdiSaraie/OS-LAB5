#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

#define SHM_SIZE 1024

struct ipc_perm {
	int id;
	int mode;
};

struct shmid_ds {
	struct ipc_perm perm_info;
	int ref_count;
	int processes_attached[NPROC];
	int attached_size;
	uint frame;
};

struct shm_table {
	struct shmid_ds segments[SHM_SIZE];
	int size;
};

struct shm_table table = {.size = 0};

int shm_getat(int id){
	int found = 0;
	acquiresleep(&shm_mutex);
	struct shmid_ds* seg;
	for (seg = table.segments; seg < &table.segments[NPROC]; seg++){
		if (seg->perm_info.id == id){
			found = 1;
			//cprintf("found\n");
			break;
		}
	}
	
	if(!found){
		char* mem = kalloc();
		if(mem == 0){
      		cprintf("kalloc failed\n");
      		return -1;
		}
		//cprintf("not found\n");
		memset(mem, 0, PGSIZE);
		seg = &table.segments[table.size++];
		seg->frame = V2P(mem);
		seg->perm_info.id = id;
		seg->perm_info.mode = 1;
		seg->attached_size = 0;
		seg->ref_count = 0;
	}
	
	struct proc *curproc = myproc();
	//cprintf("proc size: %d\n", curproc->sz);
	if (seg->perm_info.mode == 2){
		cprintf("segment not allowed (lebeled)\n");
		return -1;
	}
	else if (seg->perm_info.mode == 1){ //rw
		if(mappages(curproc->pgdir, (char*)PGROUNDUP(curproc->sz), PGSIZE, seg->frame, PTE_W|PTE_U) < 0){
			cprintf("mappages failed\n");
			return -1;
		}
		//cprintf("frame: %d\n", seg->frame);
	}
	else if (seg->perm_info.mode == 0){ //r
		if(mappages(curproc->pgdir, (char*)PGROUNDUP(curproc->sz), PGSIZE, seg->frame, PTE_W|PTE_U) < 0){
			cprintf("mappages failed\n");
			return -1;
		}
		//cprintf("frame: %d\n", seg->frame);
	}
	curproc->sz += PGSIZE;
	seg->ref_count++;
	seg->processes_attached[seg->attached_size++] = curproc->pid;
	//cprintf("proc size: %d\n", curproc->sz);
	releasesleep(&shm_mutex);
	return 0;
}


