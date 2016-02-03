#include <stdio.h>
#include <stdlib.h>
#include "thread.h"

void *thread(void *);

int          counter = 0;
thread_mutex mutex = THREAD_MUTEX_INITIALIZER;

void main(void)
{
	int            i, num;
	thread_handle *handle;
	
	do
	{
		printf("Number of threads to use: ");
		scanf("%d", &num);
	}
	while (num <= 0);
	
	handle = (thread_handle *)malloc(num * sizeof(thread_handle));
	
	for (i = 0; i < num; i++) thread_create(thread, NULL, &handle[i]);
	
	for (i = 0; i < num; i++) thread_join(handle[i], NULL);
	
	printf("Test Complete\n");
	
	free(handle);
}

void *thread(void *p)
{
	if (thread_mutex_lock(&mutex))
	{
		printf("Error locking mutex\r\n");
	}
	
	counter++;
	
	thread_sleep(1000);
	
	if (counter != 1)
	{
		printf("An error has occured\r\n");
	}
	
	counter--;
	
	if(thread_mutex_unlock(&mutex))
	{
		printf("Error unlocking mutex\r\n");
	}
	
	return NULL;
}