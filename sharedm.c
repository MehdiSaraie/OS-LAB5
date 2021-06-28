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
	for (seg = table.segments; seg < &table.segments[table.size]; seg++){
		if (seg->perm_info.id == id){
			found = 1;
			break;
		}
	}
	
	if(!found){
		char* mem = kalloc();
		if(mem == 0){
      		cprintf("kalloc failed\n");
      		releasesleep(&shm_mutex);
      		return -1;
		}
		memset(mem, 0, PGSIZE);
		seg = &table.segments[table.size++];
		seg->frame = V2P(mem);
		seg->perm_info.id = id;
		seg->perm_info.mode = 1;
		seg->ref_count = 0;
		cprintf("segment %d created\n", seg->perm_info.id);
	}
	
	struct proc *curproc = myproc();
	//cprintf("proc size: %d\n", curproc->sz);
	for(int* p = seg->processes_attached; p < &seg->processes_attached[seg->ref_count]; p++){
		if (*p == curproc->pid){
			cprintf("process already has been attached to segment\n");
			releasesleep(&shm_mutex);
			return -1;
		}
	}
	
	if (seg->perm_info.mode == 2){
		cprintf("segment not allowed (lebeled)\n");
		releasesleep(&shm_mutex);
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
		if(mappages(curproc->pgdir, (char*)PGROUNDUP(curproc->sz), PGSIZE, seg->frame, PTE_U) < 0){
			cprintf("mappages failed\n");
			releasesleep(&shm_mutex);
			return -1;
		}
		//cprintf("frame: %d\n", seg->frame);
	}
	curproc->sz += PGSIZE;
	seg->processes_attached[seg->ref_count++] = curproc->pid;
	//cprintf("proc size: %d\n", curproc->sz);
	cprintf("procces %d attached to segment %d\n", curproc->pid, seg->perm_info.id);
	releasesleep(&shm_mutex);
	return 0;
}

int shm_detach(int id){
	int seg_found = 0;
	acquiresleep(&shm_mutex);
	struct shmid_ds* seg;
	for (seg = table.segments; seg < &table.segments[table.size]; seg++){
		if (seg->perm_info.id == id){
			seg_found = 1;
			break;
		}
	}
	
	if(!seg_found){
		cprintf("shared segment not found\n");
		releasesleep(&shm_mutex);
		return -1;
	}
	
	struct proc *curproc = myproc();
	int proc_found = 0;
	for(int* p = seg->processes_attached; p < &seg->processes_attached[seg->ref_count]; p++){
		if (*p == curproc->pid){
			proc_found = 1;
			for(int* np = p+1; np < &seg->processes_attached[seg->ref_count]; np++) //shift left all
				*(np-1) = *np;
			seg->ref_count--;
			cprintf("procces %d detached from segment %d\n", curproc->pid, seg->perm_info.id);
			
			if(seg->ref_count == 0 && seg->perm_info.mode == 2){
				cprintf("segment %d destroyed\n", seg->perm_info.id);
				for(struct shmid_ds* nseg = seg+1; nseg < &table.segments[table.size]; nseg++){
					*(nseg-1) = *nseg;
				}
				table.size--;
			}
			break;
		}
	}
	
	if(!proc_found){
		cprintf("process not attached to segment yet\n");
		releasesleep(&shm_mutex);
		return -1;
	}
	
	releasesleep(&shm_mutex);
	return 0;
}

int shm_ctl(int shmid, int cmd, struct shmid_ds* buf){
	acquiresleep(&shm_mutex);
	if (cmd == 0){ //IPC_SET
	
	}
	else if (cmd == 1){ //IPC_STAT
		int seg_found = 0;
		struct shmid_ds* seg;
		for (seg = table.segments; seg < &table.segments[table.size]; seg++){
			if (seg->perm_info.id == shmid){
				seg_found = 1;
				buf->perm_info.mode = seg->perm_info.mode;
				buf->perm_info.id = seg->perm_info.id;
				break;
			}
		}
		if(!seg_found){
			cprintf("segment not found\n");
			return -1;
		}
	}
	else if (cmd == 2){ //IPC_RMID
		int found = 0;
		struct shmid_ds* seg;
		for (seg = table.segments; seg < &table.segments[table.size]; seg++){
			if (seg->perm_info.id == shmid){
				found = 1;
				break;
			}
		}
	
		if(!found){
			cprintf("segment %d not found\n", seg->perm_info.id);
			return -1;
		}

		seg->perm_info.mode = 2;
		if(seg->ref_count == 0){
			cprintf("segment %d destroyed\n", seg->perm_info.id);
			for(struct shmid_ds* nseg = seg+1; nseg < &table.segments[table.size]; nseg++){
				*(nseg-1) = *nseg;
			}
			table.size--;
		}
		return 0;
	}
	
	releasesleep(&shm_mutex);
	return 0;
}

