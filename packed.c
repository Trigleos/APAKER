#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* get_file_path(int pid) {
	char pid_str[13];
	sprintf(pid_str,"%d",pid);
	char path[30] = "/proc/";
	strcat(path,pid_str);
	strcat(path,"/maps");
	char* memory = (char*)malloc(sizeof(char) * (strlen(path)+1));
	strcpy(memory,path);
	return memory;
}


long int get_base_address(int pid) {
	char base_address[24];
	char *path = get_file_path(pid);
	FILE *fp;
	fp = fopen(path,"r");
	fgets(base_address, 20 ,fp);
	fclose(fp);
	long int base = (long int)strtol(strtok(base_address,"-"),NULL,16);
	return base;
}


long int get_encrypted_code(pid_t child, long int rip_addr) {
	long int encrypted_code_addr = rip_addr + 7;
	long int encrypted_code = ptrace(PTRACE_PEEKTEXT,child, encrypted_code_addr, 0);
	return encrypted_code;

}


void tracer(pid_t child){
	struct user_regs_struct regs;
	int seed, status;
	long int rip_addr,base;
	waitpid(child,NULL, 0);
	base = get_base_address((int)child);
	printf("Baseaddress:%lx\n",base);
	ptrace(PTRACE_CONT, child, NULL, NULL);
	
	for(int i=0;i<3;i++) {
		status = 0;
		waitpid(child,&status, 0);
		
		if (WIFEXITED(status)){
			break;
		}
		
		ptrace(PTRACE_GETREGS, child, NULL,&regs);
		rip_addr = regs.rip;
		printf("%lx\n", rip_addr);
		
		long int code;
		code = ptrace(PTRACE_PEEKTEXT, child, rip_addr-1,0);
		printf("%lx\n" , code);
		
		long int encrypted_code = get_encrypted_code(child, rip_addr);
		
		printf("%lx\n" , encrypted_code);
		
		/*seed = retrieve_nanomites(child,addr)*/
		ptrace(PTRACE_CONT, child, NULL, NULL);
		
	}
	
}



int main()
{   
	pid_t child;
	child = fork();
	if(child == 0) {
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		execl("output_nano_pie", "output_nano_pie", NULL);
	}
	else 
	{
		tracer(child);
	}
	return 0;
}
