#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include "read_nanomites.h"

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


unsigned long int get_base_address(int pid) {
	char base_address[24];
	char *path = get_file_path(pid);
	FILE *fp;
	fp = fopen(path,"r");
	fgets(base_address, 20 ,fp);
	fclose(fp);
	free(path);
	unsigned long int base = strtoul(strtok(base_address,"-"),NULL,16);
	return base;
}

unsigned long int get_end_of_nanomite_section(pid_t child, unsigned long int start_addr) {
	unsigned long int code;
	unsigned long int end_addr = 0;
	char *breakpoint;
	int offset;
	char hex_string[17];
	
	start_addr += 8;
	while(end_addr == 0)
	{
		code = ptrace(PTRACE_PEEKTEXT, child, start_addr);
		sprintf(hex_string, "%016lx",code);
		breakpoint = strstr(hex_string,"cc");
		if(breakpoint != NULL) 
		{
			offset = (unsigned long int)breakpoint - (unsigned long int)hex_string;
			end_addr = start_addr + (7-offset/2);
			if(ptrace(PTRACE_PEEKTEXT, child, end_addr) != 0xcafe1055bfcc)
			{
				end_addr = 0;
			}
		}
		start_addr += 8;
	}
	return end_addr;
}

void decrypt_code(pid_t child, unsigned long int start_addr, unsigned long int base, struct packed_file packed)
{
	unsigned long int encrypted_code, md5_xor, decrypted_code;
	unsigned char md5_hash[17];
	char seed_string[17];
	char md5_turned[17];
	unsigned int seed;
	unsigned long int end_addr = get_end_of_nanomite_section(child,start_addr);
	int code_steals = (end_addr-start_addr-8)/16;
	start_addr += 8;
	for(int i=0;i<code_steals;i++)
	{
		encrypted_code = ptrace(PTRACE_PEEKTEXT, child, start_addr);
		seed = get_seed(start_addr-base,packed);
		sprintf(seed_string,"%d",seed);
		MD5(seed_string,strlen(seed_string),md5_hash);
		for(int i2 = 15; i2 > 7; i2--)
			sprintf((md5_turned+(15-i2)*2),"%02x", md5_hash[i2]);
		md5_xor = strtoul(md5_turned,NULL,16);
		decrypted_code = encrypted_code ^ md5_xor;
		ptrace(PTRACE_POKETEXT, child, start_addr, decrypted_code);
		start_addr += 16;
	}
}


void tracer(pid_t child){
	struct user_regs_struct regs;
	int seed, status;
	unsigned long int rip_addr,base;
	unsigned long int start_addr;
	waitpid(child,NULL, 0);
	base = get_base_address((int)child);
	ptrace(PTRACE_CONT, child, NULL, NULL);
	
	struct packed_file packed = read_in_nanomites("nanomites_dump");
	
	
	while(1) {
		status = 0;
		waitpid(child,&status, 0);
		
		if (WIFEXITED(status)){
			break;
		}
		
		ptrace(PTRACE_GETREGS, child, NULL,&regs);
		rip_addr = regs.rip-1;
		unsigned long int code;
		code = ptrace(PTRACE_PEEKTEXT, child, rip_addr,0);
		
		if(code == 0xcafeb055bfcc)
		{
			start_addr = rip_addr;
			printf("Decrypt\n");
			decrypt_code(child, rip_addr,base,packed);
			regs.rip = start_addr+10;
			ptrace(PTRACE_SETREGS, child, NULL, &regs);
		}
		else if (code == 0xcafe1055bfcc)
		{
			printf("Encrypt\n");
			regs.rip = rip_addr+10;
			ptrace(PTRACE_SETREGS, child, NULL, &regs);
		}
		else
		{
			printf("Fail\n");
			exit(-1);
		}
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
