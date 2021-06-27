#include "types.h"
#include "stat.h"
#include "user.h"

#define STDOUT 1
#define TIMES 3

int main(int argc, char *argv[]) {

    if (argc != 1) {
        printf(STDOUT, "bad argument\n");
        exit();
    }
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
