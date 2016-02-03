/****************************************************************/
/*                                                              */
/*        Win32/64 & POSIX Compatible Threading Library         */
/*                                                              */
/*            Copyright (C) July 2010 Aaron Clovsky             */
/*                                                              */
/****************************************************************/

#ifndef THREAD_MODULE
#define THREAD_MODULE

/***************************************
Required Headers and Compiler Options Check (GCC/VC++/Other)
***************************************/

#ifdef  _WIN32
	#include <windows.h>
	
	#include <limits.h>
	#include <stdio.h>
#else
	#include <unistd.h>
	
	#ifdef _POSIX_THREADS
		#include <pthread.h>
	#endif
	
	#include <sched.h>
	#include <errno.h>
	#include <semaphore.h>
#endif

#ifndef THREAD_LOGICAL_CORES_X86
	#ifdef BSD
		#include <sys/param.h>
		#include <sys/sysctl.h>
	#endif
#endif

#include <stdlib.h>
#include <string.h>

/***************************************
Error Constant Definitions
***************************************/

#define THREAD_ERROR_NONE             	  0
#define THREAD_ERROR_NULL_POINTER_ERROR   1
#define THREAD_ERROR_INITIALIZATION_ERROR 2
#define THREAD_ERROR_SYSTEM_CALL_ERROR    3
#define THREAD_ERROR_MALLOC_ERROR         4

/***************************************
Structure/Type Declarations
***************************************/

//Type for mutex functions
#ifdef _WIN32
	#define THREAD_MUTEX_INITIALIZER INVALID_HANDLE_VALUE;
	
	typedef volatile HANDLE thread_mutex;
#else
	#define THREAD_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER;
	
	typedef pthread_mutex_t thread_mutex;
#endif

//Type for semaphore functions
#ifdef  _WIN32
	typedef HANDLE thread_semaphore;
#else
	typedef sem_t thread_semaphore;
#endif

//Type for threadOnce() and threadOnceAsync()
#ifdef _WIN32
	#define THREAD_ONCE_INITIALIZER 0;
	
	typedef volatile int thread_once_control;
#else
	#define THREAD_ONCE_INITIALIZER {0, PTHREAD_MUTEX_INITIALIZER}
	
	typedef struct
	{
		volatile int    control;
		pthread_mutex_t mutex;
	} thread_once_control;	
#endif

//Handle to a thread
#ifdef  _WIN32
	typedef struct
	{
		DWORD  id;
		HANDLE handle;
	} thread_handle;
#else
	typedef pthread_t thread_handle;
#endif
	
//Thread local storage handle
#ifdef  _WIN32
	typedef DWORD thread_tls;
#else
	typedef pthread_key_t thread_tls;
#endif

/***************************************
Function Declarations
***************************************/

//Spawn a thread	
int thread_create(void *(*thread)(void *), void *argument, thread_handle *handle);
//Join a thread and retrieve its return value
int thread_join(thread_handle handle, void **return_value);

//Allocate a thread local void *
int thread_tls_create(thread_tls *tls);
//Retrieve the value of a thread local void *
int thread_tls_read(thread_tls tls, void **value);
//Allocate and/or set the value of a thread local void *
int thread_tls_write(thread_tls tls, void *value);
//Release memory allocated by for thread local storage
void thread_tls_destroy(thread_tls tls);

//Lock a mutex
int thread_mutex_lock(thread_mutex *mutex);
//Attempt to lock a mutex
int thread_mutex_trylock(thread_mutex *mutex);
//Unlock a mutex
int thread_mutex_unlock(thread_mutex *mutex);

//Initialize a semaphore 
int thread_semaphore_create(thread_semaphore *semaphore, unsigned int initial_value);
//Wait for a semaphore
int thread_semaphore_wait(thread_semaphore *semaphore);
//Attempt to wait for a semaphore
int thread_semaphore_trywait(thread_semaphore *semaphore);

#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
	//Attempt to wait for a semaphore for a given period of time
	int thread_semaphore_timedwait(thread_semaphore *semaphore, unsigned int milliseconds);
#endif

//Post to a semaphore
int thread_semaphore_post(thread_semaphore *semaphore);
//Destroy a semaphore
void thread_semaphore_destroy(thread_semaphore *semaphore);

//Execute init() once, functions calling threadOnce() return when init() completes
int thread_once(thread_once_control *control, void (*init)(void));
//Execute init() once, functions calling threadOnceAsync() return immediately
int thread_once_async(thread_once_control *control, void (*init)(void));

//Yield thread time-slice to next runnable thread (MAY return immediately)
void thread_yield(void);
//Delay execution of a thread by at least a certain number of milliseconds
void thread_sleep(unsigned int milliseconds);

//Determine the number of logical cores on the current system
int thread_logical_cores(void);

#endif