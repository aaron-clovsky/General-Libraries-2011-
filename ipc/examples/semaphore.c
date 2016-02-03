#include <stdio.h>
#include "ipc.h"

#ifdef __BORLANDC__
  #pragma argsused
#endif

void main(int argc, char **argv)
{
	int error;
	ipc_semaphore semaphore;
	
	if (argc > 1)
	{
		error = ipc_semaphore_open("my_semaphore", &semaphore);
		
		if (error)
		{
			printf("Error Opening Semaphore!\n");
			
			return;
		}

		printf("Opened Semaphore\n");
		
		if (ipc_semaphore_post(&semaphore))
		{
			printf("Error Unlocking Semaphore!\n");
			
			return;	
		}
		
		printf("Unlocked Semaphore\n");
		
		ipc_semaphore_close(&semaphore);
		
		printf("Closed Semaphore\n");
	}
	else
	{
		if (ipc_semaphore_create("my_semaphore", 1, &semaphore))
		{
			printf("Error Creating Semaphore!\n");
			
			return;	
		}
		
		printf("Created Semaphore\n");
		
		if (ipc_semaphore_wait(&semaphore))
		{
			printf("Error Locking Semaphore!\n");
			
			return;	
		}
		
		printf("Locked Semaphore\n");
		
		if (ipc_semaphore_wait(&semaphore))
		{
			printf("Error Locking Semaphore!\n");
			
			return;	
		}
		
		printf("Locked Semaphore Again\n");
		
		if (ipc_semaphore_post(&semaphore))
		{
			printf("Error Unlocking Semaphore!\n");
			
			return;	
		}

		printf("Unlocked Semaphore\n");
		
		ipc_semaphore_close(&semaphore);
		
		printf("Closed Semaphore\n");
	}
}

