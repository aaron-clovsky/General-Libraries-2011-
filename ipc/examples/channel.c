#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipc.h"
#include "thread.h"

#define CHANNEL_SIZE 1024

void *thread(void *argument);

ipc_channel_server server;

int main(int argc, char **argv)
{
	unsigned int        size, thread1, thread2;
	char               *buffer;
	thread_handle       handle[2];
	ipc_channel_client  channel;
	
	thread1 = 1;
	thread2 = 2;

	if (argc > 1)
	{
		if (ipc_channel_open("my_channel", &channel, &size))
		{
			printf("Error opening channel\n");
			
			return 0;
		}
		
		if ((strlen(argv[1]) + 1) > size)
		{
			printf("Message is too large to send over this channel\nServer reported a maximum message size of %u bytes\n", size);
			
			return 0;
		}
		
		buffer = (char *)malloc(size);
		
		if (argc == 2)
		{
			if (ipc_channel_send(&channel, (void *)argv[1], strlen(argv[1]) + 1))
			{
				printf("Error sending message\n");
				
				return 0;
			}
		}
		else if (argc == 3 && !strcmp(argv[2], "-f"))
		{
			if (ipc_channel_send_and_receive(&channel, (void *)argv[1], strlen(argv[1]) + 1, buffer, &size))
			{
				printf("Send and Receive Error\n");
				
				return 0;
			}
			
			printf("Received: \"%s\"\n", buffer);
		}
		else
		{
			printf("%s", "Usage: channel [message] [-f]\n");
		}
		
		free(buffer);
		
		ipc_channel_close(&channel);
	}
	else
	{
		if (ipc_channel_create("my_channel", CHANNEL_SIZE, &server))
		{
			printf("Error Creating IPC channel\n");
			
			return 0;
		}
		
		thread_create(thread, &thread1, &handle[0]);
		thread_create(thread, &thread2, &handle[1]);
		
		thread_join(handle[0], NULL);
		thread_join(handle[1], NULL);
		
		ipc_channel_destroy(&server);
	}
	
	return 0;
}

int ipc_channel_receive(ipc_channel_server *channel, void *buffer, unsigned int *size, ipc_channel_server_client *client);

void *thread(void *argument)
{
	unsigned int              size;
	char                      buffer[CHANNEL_SIZE];
	ipc_channel_server_client client;
	
	ipc_channel_receive(&server, buffer, &size, &client);
	
	printf("Thread #%d received: \"%s\"\n", *(int *)argument, buffer);
	
	thread_sleep(2000);
	
	if (ipc_channel_client_type(client) == IPC_FULL_DUPLEX_CLIENT)
	{
		if (ipc_channel_respond(&server, client, buffer, size))
		{
			printf("Error Responding on IPC channel\n");
			
			return 0;	
		}
	}
	
	return NULL;
}
