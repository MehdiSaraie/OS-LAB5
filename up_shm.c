#include "types.h"
#include "stat.h"
#include "user.h"

#define STDOUT 1
#define TIMES 3


#define NPROC 64  // maximum number of processes
struct ipc_perm {
	int id;
	int mode;
};
struct shmid_ds {
	struct ipc_perm perm_info;
	int ref_count;
	int processes_attached[NPROC];
	int frame;
};

int main(int argc, char *argv[]) {

    if (argc != 1) {
        printf(STDOUT, "bad argument\n");
        exit();
    }

	struct shmid_ds buf;
	int program_id = getpid();
	
	if (rwinit() < 0)
        printf(STDOUT, "init lock failed\n");

    for (int i = 0; i < TIMES; i++) {
    	if (getpid() == program_id) {
		    int n = fork();
		    if (n == 0) {   // child
		        if (shm_getat(6) < 0) {
		            printf(STDOUT, "shm_getat failed\n");
		            exit();
		        }
				if (shm_ctl(6, 1, &buf) < 0) {
					printf(STDOUT, "shm_ctl failed\n");
		            exit();
				}
				printf(STDOUT,"shm_id: %d\nmode: %d\n", buf.perm_info.id, buf.perm_info.mode);
		        sleep(20);
		        if (shm_detach(6) < 0) {
		            printf(STDOUT, "shm_detach failed\n");
		            exit();
		        }
		        break;
		    }
		    else if (n > 0) {   // parent
		        sleep(10);
		    }
		    else {  // error
		        printf(STDOUT, "fork failed\n");
		        exit();
		    }
		}
	}
    while (wait() != -1)
        ;

    exit();
}
