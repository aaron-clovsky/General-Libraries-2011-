#include <stdio.h>
#include <stdlib.h>
#include "thread.h"

thread_once_control control = THREAD_ONCE_INITIALIZER;
thread_tls          tls;

void *thread(void *);

void main(void)
{
	int            i, num;
	void          *result;
	thread_handle *handle;
	
	do
	{
		printf("Number of threads to use: ");
		scanf("%d", &num);
	}
	while (num <= 0);
	
	handle = (thread_handle *)malloc(num * sizeof(thread_handle));
	
	for (i = 0; i < num; i++) thread_create(thread, malloc(3), &handle[i]);
	
	for (i = 0; i < num; i++)
	{
		thread_join(handle[i], &result);
		
		if (!result || strcmp((char *)result, "OK"))
		{
			printf("An error has Ocurred\r\n");
		}
		else
		{
			free(result);
		}
	}
	
	printf("Test Complete\n");
	
	thread_tls_destroy(tls);
	
	free(handle);
}

void init(void)
{
	thread_tls_create(&tls);
}

void *thread(void *p)
{
	void *key;
	
	thread_once(&control, init);

	if (thread_tls_write(tls, p))
	{
		printf("Error calling thread_tls_write() in thread %p\r\n", p);
	}
	
	if (thread_tls_read(tls, &key))
	{
		printf("Error calling thread_tls_read() in thread %p\r\n", p);
	}
	
	if (key != p)
	{
		printf("%p\tCould not retrieve key!\n", p);
	}
	
	strcpy((char *)p, "OK");
	
	return p;
}