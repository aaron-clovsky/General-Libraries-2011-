/****************************************************************/
/*                                                              */
/*        Win32/64 & POSIX Compatible Threading Library         */
/*                                                              */
/*            Copyright (C) July 2010 Aaron Clovsky             */
/*                                                              */
/****************************************************************/

/***************************************
Required Header and Compiler Options Check (GCC/VC++/Other)
***************************************/

#include "thread.h"

#ifdef  _WIN32
	#if !defined(_MSC_VER) || (defined(_MSC_VER) && !defined(_MT))
		#define malloc(x)     HeapAlloc  (GetProcessHeap(), 0, x)
		#define realloc(x, y) HeapReAlloc(GetProcessHeap(), 0, x, y)
		#define free(x)	      HeapFree   (GetProcessHeap(), 0, x)
	#endif
#else
	#ifndef _POSIX_THREADS
		#error POSIX support does not meet minimum requirements
	#endif

	#ifndef _REENTRANT
		#error The -pthread compiler switch is required
	#endif
#endif

/***************************************
Structure/Type Declarations
***************************************/

#ifdef _WIN64
	typedef struct
	{
		void *(*thread)(void *);
		void   *argument;
		int     return_flag;
	} thread_create_info;
	
	struct thread_return_struct
	{	
		void  *value;
		DWORD  id;
		
		struct thread_return_struct *next;
	};
	
	typedef struct thread_return_struct thread_return;
#endif

/***************************************
Global Variables
***************************************/

#ifdef _WIN32
	static CRITICAL_SECTION thread_tls_mutex;
	static volatile int     thread_tls_flag = 0;
	#ifdef _WIN64
		static thread_return    *thread_return_values = NULL;
		static CRITICAL_SECTION  thread_return_mutex;
		static int               thread_return_flag = 0;
	#endif
#else
	static pthread_mutex_t thread_tls_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/***************************************
Module Functions
***************************************/
#ifdef  _WIN32
	
	#ifdef _WIN64
		DWORD WINAPI thread_wrapper(void *p);
	#endif
	
	int thread_create(void *(*thread)(void *), void *argument, thread_handle *handle)
	{
		#ifndef _WIN64
			DWORD (WINAPI *win32_thread)(void *);
		#else
			thread_create_info *info;
		#endif
		
		thread_handle this_handle;
		
		if (!thread)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		#ifndef _WIN64
			*(int *)(&win32_thread) = *(int *)(&thread);
			
			this_handle.handle = CreateThread(NULL, 0, win32_thread, argument, 0, &this_handle.id);
		#else
			info = (thread_create_info *)malloc(sizeof(thread_create_info));
			
			if (!info)
			{
				return THREAD_ERROR_MALLOC_ERROR;
			}
			
			info->thread = thread;
			info->argument = argument;
			info->return_flag = handle ? 1 : 0;
			
  			if (!thread_return_init)
			{
				thread_return_init = 1;
				
				InitializeCriticalSection(&thread_return_mutex);
			}
			
			this_handle.handle = CreateThread(NULL, 0, thread_wrapper, (void *)info, 0, &this_handle.id);
		#endif
		
		if (!this_handle.handle)
		{
			#ifdef _WIN64
				free(info);
			#endif
			
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		if (handle)
		{
			*handle = this_handle;
		}
		else
		{
			CloseHandle(this_handle.handle);
		}
		
		return THREAD_ERROR_NONE;
	}
	
	#ifdef _WIN64
		static DWORD WINAPI thread_wrapper(void *p)
		{
			void               *value;
			int                 error_code;
			thread_return      *record;
			thread_create_info *info;
			
			info = (thread_create_info *)p;
			
			value = info->thread(info->argument);
			
			free(info);
			
			if (info->return_flag)
			{
				EnterCriticalSection(&thread_return_mutex);
				
				record = (thread_return *)malloc(sizeof(thread_return));
				
				if (record)
				{
				
					record->id    = GetCurrentThreadId();
					record->value = value;
					record->next  = thread_returnValues;
					
					thread_returnValues = record;					
				}
				
				LeaveCriticalSection(&thread_return_mutex);
			}
			
			return 0;
		}
	#endif
	
	int thread_join(thread_handle handle, void **return_value)
	{
		#ifdef _WIN64
			thread_return *current, *last;
			void          *value;
		#endif
		
		if (WaitForSingleObject(handle.handle, INFINITE) != WAIT_OBJECT_0)
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		#ifndef _WIN64
			if (return_value)
			{
				if (!GetExitCodeThread(handle.handle, (DWORD *)return_value))
				{
					return THREAD_ERROR_SYSTEM_CALL_ERROR;
				}
			}
		#else
			EnterCriticalSection(&thread_return_mutex);
			
			last = thread_returnValues;
			
			if (!last)
			{
				LeaveCriticalSection(&thread_return_mutex);
				
				return THREAD_ERROR_INITIALIZATION_ERROR;
			}
			else if (last->id == handle.id)
			{
				thread_returnValues = thread_returnValues->next;
				
				value = last->value;
				
				free(last);
			}
			else
			{
				current = last->next;
				
				while (current)
				{
					if (handle.id == current->id)
					{
						last->next = current->next;
						
						value = current->value;
						
						free(current);
						
						break;
					}
					
					last = current;
					current = current->next;
				}
				
				if (!current)
				{
					LeaveCriticalSection(&thread_return_mutex);
					
					return THREAD_ERROR_INITIALIZATION_ERROR;
				}
			}
			
			LeaveCriticalSection(&thread_return_mutex);
			
			if (return_value)
			{
				*return_value = value;
			}
		#endif
		
		CloseHandle(handle.handle);
		
		return THREAD_ERROR_NONE;
	}
	
	static int thread_mutex_lock_routine(thread_mutex *mutex, int tryLock)
	{
		int    error_code;
		char   name[32];
		HANDLE handle;
		
		if (*mutex == INVALID_HANDLE_VALUE)
		{
			sprintf(name, "%d:%p", GetCurrentProcessId(), mutex);
			
			handle = CreateMutex(NULL, FALSE, name);
			
			error_code = GetLastError();
			
			if (error_code == ERROR_SUCCESS)
			{
				if (*mutex == INVALID_HANDLE_VALUE)
				{
					*mutex = CreateMutex(NULL, FALSE, NULL);
				}
				
				CloseHandle(handle);
			}
			else if (error_code == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(handle);
				
				while (*mutex == INVALID_HANDLE_VALUE){;}
			}
			else
			{
				return THREAD_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		if (tryLock)
		{
			error_code = WaitForSingleObject(*mutex, 0);
			
			if (error_code == WAIT_OBJECT_0)
			{
				return THREAD_ERROR_NONE;
			}
			else if (error_code == WAIT_TIMEOUT)
			{
				return THREAD_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return THREAD_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		else
		{
			if (WaitForSingleObject(*mutex, INFINITE) != WAIT_OBJECT_0)
			{
				return THREAD_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		return THREAD_ERROR_NONE;
	}
	
	int thread_mutex_lock(thread_mutex *mutex)
	{
		return thread_mutex_lock_routine(mutex, 0);
	}
	
	int thread_mutex_trylock(thread_mutex *mutex)
	{
		return thread_mutex_lock_routine(mutex, 1);
	}
	
	int thread_mutex_unlock(thread_mutex *mutex)
	{
		if (!ReleaseMutex(*mutex))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	static int thread_once_routine(thread_once_control *control, void (*init)(void), int sync)
	{
		int    error_code;
		char   name[32];	
		HANDLE handle;
		
		if (!*control)
		{
			sprintf(name, "%d:%p", GetCurrentProcessId(), control);
			
			handle = CreateMutex(NULL, TRUE, name);
			
			error_code = GetLastError();
			
			if (error_code == ERROR_SUCCESS)
			{
				if (!*control)
				{
					init();
					
					*control = 1;
				}
				
				ReleaseMutex(handle);
				
				CloseHandle(handle);
			}
			else if (error_code == ERROR_ALREADY_EXISTS)
			{
				if (sync)
				{
					error_code = WaitForSingleObject(handle, INFINITE);
					
					if (error_code != WAIT_OBJECT_0)
					{
						while (!*control){;}
					}
					
					ReleaseMutex(handle);
				}
				
				CloseHandle(handle);
			}
			else
			{
				return THREAD_ERROR_SYSTEM_CALL_ERROR;
			}
		}

		return THREAD_ERROR_NONE;
	}
	
	int thread_once(thread_once_control *control, void (*init)(void))
	{
		return thread_once_routine(control, init, 1);
	}
	
	int thread_once_async(thread_once_control *control, void (*init)(void))
	{
		return thread_once_routine(control, init, 0);
	}
	
	void thread_yield(void)
	{
		SleepEx(0, FALSE);
	}
	
	void thread_sleep(unsigned int milliseconds)
	{
		SleepEx(milliseconds, FALSE);
	}
	
	int thread_semaphore_init(thread_semaphore *semaphore, unsigned int initial_value)
	{
		if (!semaphore)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		*semaphore = CreateSemaphore(NULL, initial_value, (LONG)(LONG_MAX), NULL);
		
		if (!*semaphore)
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	int thread_semaphore_wait(thread_semaphore *semaphore)
	{
		if (!semaphore)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		if (WaitForSingleObject(*semaphore, INFINITE) != WAIT_OBJECT_0)
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	int thread_semaphore_trywait(thread_semaphore *semaphore)
	{
		DWORD error_code;
		
		if (!semaphore)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		error_code = WaitForSingleObject(semaphore, 0);
		
		if (error_code == WAIT_OBJECT_0)
		{
			return THREAD_ERROR_NONE;
		}
		else if(error_code == WAIT_TIMEOUT)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		else
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
	}
	
	int thread_semaphore_timedwait(thread_semaphore *semaphore, unsigned int milliseconds)
	{
		DWORD error_code;
		
		error_code = WaitForSingleObject(*semaphore, milliseconds);
		
		if (error_code == WAIT_OBJECT_0)
		{
			return THREAD_ERROR_NONE;
		}
		else if(error_code == WAIT_TIMEOUT)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		else
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
	}
	
	int thread_semaphore_post(thread_semaphore *semaphore)
	{
		if (!semaphore)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		if (!ReleaseSemaphore(*semaphore, 1, NULL))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	void thread_semaphore_destroy(thread_semaphore *semaphore)
	{
		if (!semaphore)
		{
			return;
		}
		
		CloseHandle(*semaphore);
	}
	
#else
	
	
	int thread_create(void *(*thread)(void *), void *argument, thread_handle *handle)
	{
		pthread_t thisHandle;
		
		if (!thread)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		if (pthread_create(&thisHandle, NULL, thread, argument))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		if (handle)
		{
			*handle = thisHandle;
		}
		else
		{
			pthread_detach(thisHandle);
		}
		
		return THREAD_ERROR_NONE;
	}
	
	int thread_join(thread_handle handle, void **return_value)
	{
		int error_code;
		
		error_code = pthread_join((pthread_t)handle, return_value);
		
		if (error_code)
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	int thread_mutex_lock(thread_mutex *mutex)
	{
		if (pthread_mutex_lock(mutex))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	int thread_mutex_trylock(thread_mutex *mutex)
	{
		int error_code;
		
		error_code = pthread_mutex_trylock(mutex);
		
		if (error_code)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	int thread_mutex_unlock(thread_mutex *mutex)
	{
		if (pthread_mutex_unlock(mutex))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	static int thread_once_routine(thread_once_control *control, void (*init)(void), int sync)
	{
		if (!control->control)
		{
			if (pthread_mutex_lock(&control->mutex))
			{
				return THREAD_ERROR_SYSTEM_CALL_ERROR;
			}
			
			if (!control->control)
			{
				if (sync)
				{
					init();
					
					control->control = 1;
				}
				else
				{
					control->control = 1;
			
					pthread_mutex_unlock(&control->mutex);
					
					init();
					
					return THREAD_ERROR_NONE;
				}
			}
			
			pthread_mutex_unlock(&control->mutex);
		}
		
		return THREAD_ERROR_NONE;
	}

	int thread_once(thread_once_control *control, void (*init)(void))
	{
		return thread_once_routine(control, init, 1);
	}
	
	int thread_once_async(thread_once_control *control, void (*init)(void))
	{
		return thread_once_routine(control, init, 0);
	}
	
	void thread_yield(void)
	{
		sched_yield();
	}
	
	void thread_sleep(unsigned int milliseconds)
	{
		#if _POSIX_C_SOURCE >= 199309L
			struct timespec interval, remainder;
			
			interval.tv_sec = milliseconds / 1000;
			interval.tv_nsec = (milliseconds % 1000) * 1000000;
			
			while (nanosleep(&interval, &remainder) == -1)
			{ 
				interval = remainder;
			}
		#else
			unsigned int seconds, microseconds;
			
			seconds = milliseconds / 1000;
			microseconds = (milliseconds % 1000) * 1000;
			
			if (seconds)
			{
				sleep(seconds);
			}
			
			usleep(microseconds);
		#endif
	}
	
	int thread_semaphore_init(thread_semaphore *semaphore, unsigned int initial_value)
	{
		if (!semaphore)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		if (sem_init(semaphore, 0, initial_value))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	int thread_semaphore_wait(thread_semaphore *semaphore)
	{
		if (!semaphore)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		if (sem_wait(semaphore))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	int thread_semaphore_trywait(thread_semaphore *semaphore)
	{
		if (!semaphore)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		errno = 0;
		
		if (sem_trywait(semaphore))
		{
			if (errno == EAGAIN)
			{
				return THREAD_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return THREAD_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		return THREAD_ERROR_NONE;
	}
	
	#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
		int thread_semaphore_timedwait(thread_semaphore *semaphore, unsigned int milliseconds)
		{
			struct timespec timeout;
			
			if (!semaphore)
			{
				return THREAD_ERROR_INITIALIZATION_ERROR;
			}
			
			timeout.tv_sec = milliseconds / 1000;
			timeout.tv_nsec = (milliseconds % 1000) * 1000000;
			
			errno = 0;
			
			if (sem_timedwait(semaphore, &timeout))
			{
				if (errno == ETIMEDOUT)
				{
					return THREAD_ERROR_INITIALIZATION_ERROR;
				}
				else
				{
					return THREAD_ERROR_SYSTEM_CALL_ERROR;
				}
			}
			
			return THREAD_ERROR_NONE;
		}
	#endif
	
	int thread_semaphore_post(thread_semaphore *semaphore)
	{
		if (!semaphore)
		{
			return THREAD_ERROR_INITIALIZATION_ERROR;
		}
		
		if (sem_post(semaphore))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return THREAD_ERROR_NONE;
	}
	
	void thread_semaphore_destroy(thread_semaphore *semaphore)
	{
		if (!semaphore)
		{
			return;
		}
		
		sem_destroy(semaphore);
	}
#endif
	
int thread_tls_init(thread_tls *tls)
{	
	if (!tls)
	{
		return THREAD_ERROR_INITIALIZATION_ERROR;
	}
	
	#ifdef	_WIN32
		*tls = TlsAlloc();
		
		if (*tls == TLS_OUT_OF_INDEXES)
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
	#else
		if (pthread_key_create(tls, NULL))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
	#endif
	
	return THREAD_ERROR_NONE;
}
	
int thread_tls_read(thread_tls tls, void **value)
{
	if (!value)
	{
		return THREAD_ERROR_INITIALIZATION_ERROR;
	}
	
	#ifdef _WIN32
		*value = TlsGetValue(tls);
		
		if (!*value && GetLastError() != ERROR_SUCCESS)
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
	#else
		*value = pthread_getspecific(tls);
	#endif
	
	return THREAD_ERROR_NONE;
}
	
int thread_tls_write(thread_tls tls, void *value)
{
	if (!value)
	{
		return THREAD_ERROR_INITIALIZATION_ERROR;
	}
	
	#ifdef _WIN32
		if (!TlsSetValue(tls, value))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
	#else
		if (pthread_setspecific(tls, value))
		{
			return THREAD_ERROR_SYSTEM_CALL_ERROR;
		}
	#endif
	
	return THREAD_ERROR_NONE;
}

void thread_tls_destroy(thread_tls tls)
{
	#ifdef _WIN32
		TlsFree(tls);
	#else
		pthread_key_delete(tls);
	#endif
	
	return;
}