#include <stdio.h>
#include <stdlib.h>
#include "thread.h"

void *thread(void *);

int              counter = 0;
thread_mutex     mutex = THREAD_MUTEX_INITIALIZER;
thread_semaphore semaphore;

void main(void)
{
	int            i, num, max;
	thread_handle *handle;
	
	do
	{
		printf("Number of concurrent threads: ");
		scanf("%d", &max);
	}
	while (max <= 0);
	
	do
	{
		printf("Number of threads to use: ");
		scanf("%d", &num);
	}
	while (num <= 0);
	
	if (thread_semaphore_create(&semaphore, max))
	{
		printf("Error initializing semaphore\n");
		
		return;
	}
	
	handle = (thread_handle *)malloc(num * sizeof(thread_handle));
	
	for (i = 0; i < num; i++) thread_create(thread, NULL, &handle[i]);
	
	for (i = 0; i < num; i++) thread_join(handle[i], NULL);
	
	printf("Test Complete\n");
	
	thread_semaphore_destroy(&semaphore);
	
	free(handle);
}

void *thread(void *p)
{
	if (thread_semaphore_wait(&semaphore))
	{
		printf("Error waiting for semaphore\r\n");
	
	}
	
	if (thread_mutex_lock(&mutex))
	{
		printf("Error locking mutex\r\n");
	}
	
	counter++;
	
	if(thread_mutex_unlock(&mutex))
	{
		printf("Error unlocking mutex\r\n");
	}
	
	thread_sleep(1000);
	
	printf("There are %d concurrent threads\r\n", counter);
	
	if (thread_mutex_lock(&mutex))
	{
		printf("Error locking mutex\r\n");
	}
	
	counter--;
	
	if(thread_mutex_unlock(&mutex))
	{
		printf("Error unlocking mutex\r\n");
	}
	
	if(thread_semaphore_post(&semaphore))
	{
		printf("Error posting semaphore\r\n");
	}
	
	return NULL;
}