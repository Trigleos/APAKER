#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include "read_nanomites.h"
#include "nanomites_encrypted.h"

char* get_file_path(int pid) {		//construct filepath to /proc/[pid]/maps
	char pid_str[13];
	sprintf(pid_str,"%d",pid);		//transform pid int to string
	char path[30] = "/proc/";
	strcat(path,pid_str);			//concatenate /proc/, [pid] and /maps
	strcat(path,"/maps");
	char* memory = (char*)malloc(sizeof(char) * (strlen(path)+1));	//allocate memory for filepath on heap
	strcpy(memory,path);			// copy string onto heap
	return memory;
}


unsigned long int get_base_address(int pid) {		//returns base address of process
	char base_address[24];
	
	char *path = get_file_path(pid);		//get /proc/[pid]/maps string
	FILE *fp;

	fp = fopen(path,"r");				//open /proc/[pid]/maps
	fgets(base_address, 20 ,fp);			//read in first line
	fclose(fp);					//example of first line: 55d89e865000-55d89e866000 r--p 00000000 08:05 131446
	free(path);					//free allocated memory on heap for filepath
	
	unsigned long int base = strtoul(strtok(base_address,"-"),NULL,16);		//take everything before '-' and convert it to unsigned long. This is the base address
	return base;
}

unsigned long int get_end_of_nanomite_section(pid_t child, unsigned long int start_addr) {		//search for nanomite section end marker and return that address
	unsigned long int code;
	unsigned long int end_addr = 0;
	char *breakpoint;
	int offset;
	char hex_string[17];
	
	start_addr += 10;	//skip past start marker
	while(end_addr == 0)	//while the end marker has not been found
	{
		code = ptrace(PTRACE_PEEKTEXT, child, start_addr);	//get code from child
		sprintf(hex_string, "%016lx",code);			//convert it to hex string
		breakpoint = strstr(hex_string,"cc");			//search for breakpoint in hexstring
		if(breakpoint != NULL) 				//if there is one
		{
			offset = (unsigned long int)breakpoint - (unsigned long int)hex_string;	//get offset of breakpoint in that string
			end_addr = start_addr + (7-offset/2);						//calculate address of breakpoint (takes into account endianness)
			if(ptrace(PTRACE_PEEKTEXT, child, end_addr) != 0xcafe1055bfcc)		//check if the code at this position is actually the end marker, could be a random 0xcc in encrypted code
			{
				end_addr = 0;		//if it's not the end marker, reset end address
			}
		}
		start_addr += 8;
	}
	return end_addr;
}

void decrypt_code(pid_t child, unsigned long int start_addr, unsigned long int base, struct packed_file packed)	//decrypts nanomites in section
{
	unsigned long int encrypted_code, md5_xor, decrypted_code;
	unsigned char md5_hash[17];
	char seed_string[17];
	char md5_turned[17];
	unsigned int seed;
	
	unsigned long int end_addr = get_end_of_nanomite_section(child,start_addr);		//get address of end of our current nanomites section
	
	int code_steals = (end_addr-start_addr-10)/16;					//one code steal = 8 bytes encrypted and 8 bytes unencrypted
	
	start_addr += 10;									//encrypted code starts after marker
	
	for(int i=0;i<code_steals;i++)							//for number of nanomites in this section
	{
		encrypted_code = ptrace(PTRACE_PEEKTEXT, child, start_addr);			//get encrypted code from child	
		seed = get_seed(start_addr-base,packed);					//get md5 seed from nanomites_dump file based on offset
		sprintf(seed_string,"%d",seed);						//convert seed to string
		MD5(seed_string,strlen(seed_string),md5_hash);				//calculate MD5 of seed string and put it into md5_hash
		
		for(int i2 = 15; i2 > 7; i2--)						//convert last 8 bytes of MD5 hash into string, also turn them around because of endianness
			sprintf((md5_turned+(15-i2)*2),"%02x", md5_hash[i2]);
			
		md5_xor = strtoul(md5_turned,NULL,16);					//convert hex string to unsigned long
		decrypted_code = encrypted_code ^ md5_xor;					//xor encrypted code with md5 value to get original code
		ptrace(PTRACE_POKETEXT, child, start_addr, decrypted_code);			//put code back into the child's memory so it can execute the section
		start_addr += 16;								//skip to the next encrypted nanomite
	}
}

void encrypt_code(pid_t child, unsigned long int start_addr,unsigned long int end_addr, unsigned long int base, struct packed_file packed)	//same routine as decrypt_code, only to reencrypt code after child has gone through section
{
	unsigned long int encrypted_code, md5_xor, decrypted_code;
	unsigned char md5_hash[17];
	char seed_string[17];
	char md5_turned[17];
	unsigned int seed;
	
	int code_steals = (end_addr-start_addr-10)/16;
	
	start_addr += 10;
	
	for(int i=0;i<code_steals;i++)
	{
		decrypted_code = ptrace(PTRACE_PEEKTEXT, child, start_addr);
		seed = get_seed(start_addr-base,packed);
		sprintf(seed_string,"%d",seed);
		MD5(seed_string,strlen(seed_string),md5_hash);
		
		for(int i2 = 15; i2 > 7; i2--)
			sprintf((md5_turned+(15-i2)*2),"%02x", md5_hash[i2]);
			
		md5_xor = strtoul(md5_turned,NULL,16);
		encrypted_code = decrypted_code ^ md5_xor;
		ptrace(PTRACE_POKETEXT, child, start_addr, encrypted_code);
		start_addr += 16;
	}
}


void tracer(pid_t child){
	struct user_regs_struct regs;
	int seed, status;
	unsigned long int rip_addr,base;
	unsigned long int start_addr;
	
	waitpid(child,NULL, 0);			//ignore first trap to parent
	ptrace(PTRACE_CONT, child, NULL, NULL);	
	
	base = get_base_address((int)child);		//get base address of child process
	
	struct packed_file packed = read_in_nanomites();	//read in nanomites details from nanomites_dump file created by the preparation python script
	
	
	for (int i=0;i<3;i++) {
		status = 0;
		waitpid(child,&status, 0);		//Wait for child to trap to parent
		
		if (WIFEXITED(status)){		//If child has exited, stop loop
			break;
		}
		
		ptrace(PTRACE_GETREGS, child, NULL,&regs);	//get regs from child
		rip_addr = regs.rip-1;				//set rip_addr to address of 0xcc
		unsigned long int code;
		code = ptrace(PTRACE_PEEKTEXT, child, rip_addr,0);	//get code from child at breakpoint address

		if(code == 0xcafeb055bfcc)				//if code is nanomites section start marker
		{
			start_addr = rip_addr;				//save start_addr of nanomites section for later
			decrypt_code(child, rip_addr,base,packed);	//decrypt nanomites in current nanomites section
			regs.rip = rip_addr+10;			//set rip addr to the first real instruction after nanomite section start marker
			ptrace(PTRACE_SETREGS, child, NULL, &regs);	//change rip of child 
		}
		else if (code == 0xcafe1055bfcc)			//if code is nanomites section end marker
		{
			encrypt_code(child,start_addr,rip_addr,base,packed);	//encrypt current nanomite section again
			regs.rip = rip_addr+10;			//set rip addr to the first real instruction after nanomite section end marker
			ptrace(PTRACE_SETREGS, child, NULL, &regs);	//change rip of child 
		}
		ptrace(PTRACE_CONT, child, NULL, NULL);		//let child continue execution
		
	}
	
}

void write_nanomites_file()		//write unsigned char array that represents ELF to file so it can be executed
{
	FILE *fp;
	fp = fopen("child_elf","wb");
	fwrite(resc_nanomites_encrypted,1,resc_nanomites_encrypted_len,fp);
	fclose(fp);
	chmod("child_elf",0777);	//make file executable
}

int main()
{   
	write_nanomites_file();
	pid_t child;
	child = fork();
	if(child == 0) {
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);		//ask parent to trace
		execl("child_elf", "child_elf", NULL);	//execute nanomite encrypted ELF
	}
	else 
	{
		tracer(child);		//parent traces child
		remove("child_elf");	//remove child file after execution has finished
	}
	return 0;
}
