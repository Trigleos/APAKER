#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanomite_dump.h"

//used to parse nanomites_dump file

struct nanomite {	//single nanomite struct
	int offset;
	int seed;
};

struct packed_file {	//struct that contains every nanomite in the encrypted ELF
	int number_of_nanomites;
	struct nanomite *nanomites;
};



struct packed_file read_in_nanomites(char* filename)		//creates packed_file struct from nanomites_dump file
{
	FILE *fp;
	int number_of_nanomites;
	struct packed_file pf;
	
	//fp = fopen(filename,"r");	//read in first line that contains the number of nanomites
	fp = fmemopen(nanomites_dump,nanomites_dump_len,"r");
	char line[256];
	fgets(line, 255,fp);
	pf.number_of_nanomites = atoi(line);
	
	pf.nanomites = malloc(pf.number_of_nanomites*sizeof(struct nanomite));  //allocate memory for every nanomite struct
	
	for(int i = 0;i<pf.number_of_nanomites;i++)		//read in different nanomite lines and parse offset and seed
	{
		fgets(line, 255,fp);
		struct nanomite nm;
		nm.offset = atoi(strtok(line,":"));
		nm.seed = atoi(strtok(NULL,":"));
		pf.nanomites[i] = nm;
	}
	fclose(fp);
	return pf;
}

int get_seed(int offset, struct packed_file packed)	//return seed based on offset
{
	for(int i = 0;i<packed.number_of_nanomites;i++)
	{
		if(packed.nanomites[i].offset == offset){
			return packed.nanomites[i].seed;
		}
	}
	return -1;
}

/*
int main()
{
	struct packed_file packed = read_in_nanomites("nanomites_dump");
	
	int seed = get_seed(4556,packed);
	printf("%d\n",seed);
}
*/

