#include <stdio.h>
#include "ipc.h"
#include "thread.h"

int main(void)
{
	int          error;
	ipc_instance instance;
	
	error = ipc_instance_create("my_program", &instance);
	
	if (error)
	{
		if (error == IPC_ERROR_INITIALIZATION_ERROR)
		{
			printf("Another instance of this process is currently running!\n");
		}
		else
		{
			printf("An error occurred!\n");
		}
		
		exit(0);
	}
	else
	{
		printf("one instance running");
	}
	
	thread_sleep(5000);
	
	ipc_instance_destroy(&instance);
	
	return 0;
}