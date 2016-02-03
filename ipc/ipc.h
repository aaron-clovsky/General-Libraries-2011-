/****************************************************************/
/*                                                              */
/*WIN32/64 & POSIX Compatible Interprocess Communication Library*/
/*                                                              */
/*           Copyright (C) August 2010 Aaron Clovsky            */
/*                                                              */
/****************************************************************/

#ifndef IPC_MODULE
#define IPC_MODULE

/***************************************
Platform Support Check
***************************************/

#if (!(defined(_WIN32) || !defined(_POSIX_C_SOURCE) || !defined(_XOPEN_SOURCE)))
	#error This library is dependant on WIN32/64 or POSIX support
#endif

/***************************************
Required Headers
***************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
	#include <windows.h>
	#include <limits.h>
#else	
	#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
		#include <time.h>
	#endif
	
	#include <errno.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <sys/mman.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <semaphore.h>
#endif

/***************************************
Compile Time Options
***************************************/

#if (defined(_WIN32) || _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600)
	#ifndef IPC_POLLING_INTERVAL
		#define IPC_POLLING_INTERVAL 100
	#endif
	
	#define IPC_TIMEOUT
#endif

#ifndef IPC_MAX_SIMULTANEOUS_CLIENTS
	#define IPC_MAX_SIMULTANEOUS_CLIENTS 45
#endif

#ifndef IPC_NAME_MAX_LENGTH
	#define IPC_NAME_MAX_LENGTH 1000
#endif

/***************************************
Constant Definitions
***************************************/

#define IPC_HALF_DUPLEX_CLIENT 0
#define IPC_FULL_DUPLEX_CLIENT 1

/***************************************
Error Constant Definitions
***************************************/

#define IPC_ERROR_NONE                  0
#define IPC_ERROR_NULL_POINTER_ERROR    1
#define IPC_ERROR_INITIALIZATION_ERROR  2
#define IPC_ERROR_MALLOC_ERROR          3
#define IPC_ERROR_SYSTEM_CALL_ERROR     4

/***************************************
Structure/Type Declarations
***************************************/

#ifdef _WIN32
	
	typedef HANDLE ipc_instance;
	
	typedef HANDLE ipc_semaphore;
	
	typedef struct
	{
		unsigned int          server_size;
		volatile unsigned int client_size;
		volatile unsigned int response_size;
		DWORD                 server_pid;
		volatile HANDLE       duplex;
		volatile DWORD        duplex_pid;
		HANDLE                response_client_sem;
		char                  message[];
	} ipc_map;
	
	typedef struct
	{
		HANDLE   client;
		HANDLE   server;
		HANDLE   respond;
		HANDLE   file;
		ipc_map *map;
	} ipc_channel_server;
	
	typedef struct
	{
		HANDLE   client;
		HANDLE   server;
		HANDLE   respond;
		HANDLE   duplex;
		ipc_map *map;
	} ipc_channel_client;
	
	typedef struct
	{
		HANDLE semaphore;
		DWORD  pid;
	} ipc_channel_server_client;
	
	typedef struct
	{
		unsigned int size;
		char         data[];
	} ipc_shm_map;
	
	typedef struct
	{
		HANDLE       file;
		ipc_shm_map *map;
	} ipc_shm;
	
	
#else
	
	typedef struct
	{
		sem_t *semaphore;
		char  *path;
		char  *lockpath;
		char  *temppath;
		int    lockfile;
	} ipc_semaphore;
	
	typedef struct
	{
		char *path;
		int   file;
	} ipc_instance;
	
	typedef struct
	{
		sem_t                 server;
		unsigned int          server_size;
		volatile pid_t        server_pid;
		sem_t                 client;          //provides write access to the primary shm region
		volatile unsigned int client_size;
		volatile unsigned int client_offset;
		volatile unsigned int response_size;
		sem_t                 response_server; // synchronizes respond() - provides write access to secondary shm region
		sem_t                 response_client; // limits concurrent full duplex clients
		volatile pid_t        response_client_pid[IPC_MAX_SIMULTANEOUS_CLIENTS];
		sem_t		      response_client_sem[IPC_MAX_SIMULTANEOUS_CLIENTS];
		char                  message[];
	} ipc_map;
	
	typedef struct
	{
		char    *channel_path;
		char    *lock_path;
		int      lock_file;
		ipc_map *map;
	} ipc_channel_server;
	
	typedef unsigned int ipc_channel_server_client;
	
	typedef ipc_map * ipc_channel_client;
	
	typedef struct
	{
		unsigned int size;
		char         data[];
	} ipc_shm_map;
	
	typedef struct
	{
		char        *shm_path;
		int          lock_file;
		char        *lock_path; 
		ipc_shm_map *map;
	} ipc_shm;

#endif

/***************************************
Function Declarations
***************************************/

//Create a unique named IPC instance
int ipc_instance_create(const char *name, ipc_instance *instance);
//Check to see if an instance of a given name is running
int ipc_instance_check(const char *name);
//Destroy an IPC instance
void ipc_instance_destroy(ipc_instance *instance);

//Create an IPC communication channel
int ipc_channel_create(const char *name, unsigned int size, ipc_channel_server *channel);
//Open an existing IPC communication channel
int ipc_channel_open(const char *name, ipc_channel_client *channel, unsigned int *size);
//Send a message to an existing IPC communication channel
int ipc_channel_send(ipc_channel_client *channel, void *data, unsigned int size);
//Wait for and retrieve a message from an IPC channel
int ipc_channel_receive(ipc_channel_server *channel, void *buffer, unsigned int *size, ipc_channel_server_client *client);
//Determine the type of client connecting to an IPC server
int ipc_channel_client_type(ipc_channel_server_client client);
//Send a message to an existing IPC communication channel and wait for and retrieve a response
int ipc_channel_send_and_receive(ipc_channel_client *channel, void *out_data, unsigned int out_size, void *in_data, unsigned int *in_size);
//Respond on an IPC channel to a full duplex message sent by ipc_channel_send_and_receive()
int ipc_channel_respond(ipc_channel_server *channel, ipc_channel_server_client client, void *data, unsigned int size);
//Destroy an IPC communication channel
void ipc_channel_destroy(ipc_channel_server *channel);
//Close an IPC communication channel
void ipc_channel_close(ipc_channel_client *channel);

//Create a shared memory region
int ipc_shm_create(ipc_shm *shm, const char *name, unsigned int size, void **addr);
//Destroy a shared memory region
void ipc_shm_destroy(ipc_shm *shm);
//Open a shared memory region
int ipc_shm_open(const char *name, void **addr, unsigned int *size);
//Close a shared memory region
void ipc_shm_close(void *addr);

//Create an inter-process semaphore
int ipc_semaphore_create(const char *name, unsigned int initial_value, ipc_semaphore *semaphore);
//Open an existing inter-process semaphore
int ipc_semaphore_open(const char *name, ipc_semaphore *semaphore);
//Wait to decrement an inter-process semaphore
int ipc_semaphore_wait(ipc_semaphore *semaphore);
//Attempt to decrement an inter-process semaphore
int ipc_semaphore_trywait(ipc_semaphore *semaphore);

#ifdef IPC_TIMEOUT
	//Attempt to decrement an inter-process semaphore for the specified time interval
	int ipc_semaphore_timedwait(ipc_semaphore *semaphore, int milliseconds);
#endif

//Increment an inter-process semaphore
int ipc_semaphore_post(ipc_semaphore *semaphore);
//Close an inter-process semaphore
void ipc_semaphore_close(ipc_semaphore *semaphore);

#endif
