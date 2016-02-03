#include <stdio.h>
#include <stdlib.h>
#include "thread.h"

void *thread(void *);
void  init(void);

char                choice;
int                 counter = 0;
thread_once_control control = THREAD_ONCE_INITIALIZER;

void main(void)
{
	thread_handle *handle;
	int           i, num;
	
	do
	{
		printf("Number of threads to use: ");
		scanf("%d", &num);
	}
	while (num <= 0);
	
	do
	{
		printf("Use Asynchronous Initialization (y/n): ");
		scanf("%c%c", &choice, &choice);
	}
	while (choice != 'y' && choice != 'n');
	
	handle = (thread_handle *)malloc(num * sizeof(thread_handle));
	
	for (i = 0; i < num; i++) thread_create(thread, NULL, &handle[i]);
	
	for (i = 0; i < num; i++) thread_join(handle[i], NULL);
	
	if (counter != 1)
	{
		printf("An error has Ocurred\r\n");
	}
	
	printf("Test Complete\n");
	
	free(handle);
}

void *thread(void *p)
{
	if (choice == 'y')
	{
		if (thread_once_async(&control, init))
		{
			printf("Error performing initialization\r\n");
		}
		
		if (counter != 0)
		{
			printf("This should also print exactly once\r\n");
		}
	}	
	else
	{
		if (thread_once(&control, init))
		{
			printf("Error performing initialization\r\n");
		}
		
		if (counter != 1)
		{
			printf("An error has Occured\r\n");
		}
	}
	
	return NULL;
}

void init(void)
{
	if (choice == 'y')
	{
		thread_sleep(1000);
	}
	
	printf("This should print exactly once\r\n");
	
	counter++;
}