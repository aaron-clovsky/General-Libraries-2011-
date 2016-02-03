#include "ipc.h"
#include <stdio.h>

void main(int argc, char **argv)
{
	int           error;
	ipc_shm       shm;
	void         *mem;
	unsigned int  size;
	
	if (argc > 2)
	{
		printf("Incorrect number of arguments.\n");
	}
	else if (argc == 1)
	{
		error = ipc_shm_open("my_shm", &mem, &size);
		
		if (error)
		{
			printf("Error #%d opening the shared memory area.\n", error);
			
			exit(0);
		}
		
		printf("Read message \"%s\" from shared memory area.\n", (char *)mem);
		
		ipc_shm_close(mem);
	}
	else
	{
		error = ipc_shm_create(&shm, "my_shm", 1024, &mem);
		
		if (error)
		{
			printf("Error #%d creating the shared memory area.\n", error);
			
			exit(0);
		}
		
		printf("Posting message \"%s\" to shared memory area.\n", argv[1]);
		
		strcpy((char *)mem, argv[1]);
		
		printf("Press the return key to end the program.\n");
		
		getchar();
		
		ipc_shm_destroy(&shm);
	}
}
