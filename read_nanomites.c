#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct nanomite {
	int offset;
	int seed;
};

struct packed_file {
	int number_of_nanomites;
	struct nanomite *nanomites;
};



struct packed_file read_in_nanomites(char* filename)
{
	FILE *fp;
	int number_of_nanomites;
	struct packed_file pf;
	
	fp = fopen(filename,"r");
	char line[256];
	fgets(line, 255,fp);
	pf.number_of_nanomites = atoi(line);
	pf.nanomites = malloc(pf.number_of_nanomites*sizeof(struct nanomite));
	for(int i = 0;i<pf.number_of_nanomites;i++)
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

int get_seed(int offset, struct packed_file packed)
{
	for(int i = 0;i<packed.number_of_nanomites;i++)
	{
		if(packed.nanomites[i].offset == offset){
			return packed.nanomites[i].seed;
		}
	}
	return -1;
}


int main()
{
	struct packed_file packed = read_in_nanomites("nanomites_dump");
	
	int seed = get_seed(4552,packed);
	printf("%d\n",seed);
}
