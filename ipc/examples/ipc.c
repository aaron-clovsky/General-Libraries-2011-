/****************************************************************/
/*                                                              */
/*WIN32/64 & POSIX Compatible Interprocess Communication Library*/
/*                                                              */
/*           Copyright (C) August 2010 Aaron Clovsky            */
/*                                                              */
/****************************************************************/

/***************************************
Required Header and Compiler Options Check (GCC/VC++/Other)
***************************************/

#include "ipc.h"

#ifdef  _WIN32
	#if !defined(_MSC_VER) || (defined(_MSC_VER) && !defined(_MT))
		#define malloc(x)     HeapAlloc  (GetProcessHeap(), 0, x)
		#define realloc(x, y) HeapReAlloc(GetProcessHeap(), 0, x, y)
		#define free(x)	      HeapFree   (GetProcessHeap(), 0, x)
	#endif
#else
	#if !defined(_POSIX_C_SOURCE) || (_POSIX_C_SOURCE < 199309L)
		#error POSIX support does not meet minimum requirements
	#endif

	//#ifndef _REENTRANT
	//	#error The -pthread compiler switch is required
	//#endif
#endif

/***************************************
Constant Definitions
***************************************/

#define IPC_INSTANCE_PUBLIC     0
#define IPC_CHANNEL_PUBLIC      1
#define IPC_CHANNEL_PRIVATE     2
#define IPC_CHANNEL_SEM_SERVER  3
#define IPC_CHANNEL_SEM_CLIENT  4
#define IPC_CHANNEL_SEM_RESPOND 5
#define IPC_SHM_PUBLIC          6
#define IPC_SHM_PRIVATE         7
#define IPC_SEMAPHORE_PUBLIC    8
#define IPC_SEMAPHORE_PRIVATE   9
#define IPC_SEMAPHORE_TEMPORARY 10

/***************************************
Module Functions
***************************************/

#ifdef _WIN32
	
	static int ipc_path(const char *name, int type, char **path)
	{
		const char *s;
		
		if (!name || !path)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		else if (!*name)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		if (strlen(name) > IPC_NAME_MAX_LENGTH)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		s = name;
		
		while (*s)
		{
			if ((*s < 'a' || *s > 'z') && (*s < 'A' || *s > 'Z') && (*s < '0' || *s > '9') && *s != '-' && *s != '_')
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			
			s++;
		}
		
		*path = (char *)malloc(strlen(name) + 13);
		
		if (!*path)
		{
			return IPC_ERROR_MALLOC_ERROR;
		}
		
		switch(type)
		{
			case IPC_INSTANCE_PUBLIC:
			{
				strcpy(*path, "Global\\ipc_i");
				
				break;
			}
			
			case IPC_CHANNEL_PUBLIC:
			{
				strcpy(*path, "ipc__c");
				
				break;
			}
			
			case IPC_CHANNEL_SEM_SERVER:
			{
				strcpy(*path, "ipc_1c");
				
				break;
			}
			
			case IPC_CHANNEL_SEM_CLIENT:
			{
				strcpy(*path, "ipc_2c");
				
				break;
			}
			
			case IPC_CHANNEL_SEM_RESPOND:
			{
				strcpy(*path, "ipc_3c");
				
				break;
			}
			
			case IPC_SEMAPHORE_PUBLIC:
			{
				strcpy(*path, "ipc__s");
				
				break;
			}
			
			case IPC_SHM_PUBLIC:
			{
				strcpy(*path, "ipc__m");
				
				break;
			}
			
			default:
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
		}
		
		strcat(*path, name);
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_instance_create(const char *name, ipc_instance *instance)
	{
		int   error;
		char *path;
		
		if (!name || !instance)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = ipc_path(name, IPC_INSTANCE_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		*instance = CreateMutex(NULL, FALSE, path);
		
		free(path);
		
		if (!*instance)
		{
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_instance_check(const char *name)
	{
		int     error;
		char   *path;
		HANDLE  handle;
		
		if (!name)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = ipc_path(name, IPC_INSTANCE_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		handle = OpenMutex(MUTEX_ALL_ACCESS , FALSE, path);
		
		free(path);
		
		if (!handle)
		{
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		CloseHandle(handle);
		
		return IPC_ERROR_NONE;
	}
	
	void ipc_instance_destroy(ipc_instance *instance)
	{
		if (!instance)
		{
			return;
		}
		
		CloseHandle(*instance);
	}
	
	
	int ipc_channel_create(const char *name, unsigned int size, ipc_channel_server *channel)
	{
		int     error;
		char   *path;
		
		if (!name || !channel)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		if (!size)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		error = ipc_path(name, IPC_CHANNEL_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		//Create a pagefile backed file handle
		channel->file = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, path);
		
		error = GetLastError();
		
		free(path);
		
		if (error != ERROR_SUCCESS)
		{
			if (error == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(channel->file);
				
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Map the file handle to memory
		channel->map = (ipc_map *) MapViewOfFile(channel->file, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ipc_map) + size * 2); 
		
		if (!channel->map)
		{
			CloseHandle(channel->file);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		channel->map->server_size = size;
		channel->map->server_pid = GetCurrentProcessId();
		
		channel->map->duplex = NULL;
		
		//Create server semaphore
		error = ipc_path(name, IPC_CHANNEL_SEM_SERVER, &path);
		
		if (error)
		{
			CloseHandle(channel->file);
			UnmapViewOfFile(channel->map);
			
			return error;
		}
		
		channel->server = CreateSemaphore(NULL, 0, 1, path);
		
		error = GetLastError();
		
		free(path);
		
		if (error != ERROR_SUCCESS)
		{
			CloseHandle(channel->file);
			UnmapViewOfFile(channel->map);
			
			if (error == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(channel->server);
				
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Create client semaphore
		error = ipc_path(name, IPC_CHANNEL_SEM_CLIENT, &path);
		
		if (error)
		{
			CloseHandle(channel->file);
			UnmapViewOfFile(channel->map);
			CloseHandle(channel->server);
			
			return error;
		}
		
		channel->client = CreateSemaphore(NULL, 1, 1, path);
		
		error = GetLastError();
		
		free(path);
		
		if (error != ERROR_SUCCESS)
		{
			CloseHandle(channel->file);
			UnmapViewOfFile(channel->map);
			CloseHandle(channel->server);
			
			if (error == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(channel->client);
				
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Create response semaphore
		error = ipc_path(name, IPC_CHANNEL_SEM_RESPOND, &path);
		
		if (error)
		{
			CloseHandle(channel->file);
			UnmapViewOfFile(channel->map);
			CloseHandle(channel->server);
			CloseHandle(channel->client);
			
			return error;
		}
		
		channel->respond = CreateSemaphore(NULL, 1, 1, path);
		
		error = GetLastError();
		
		free(path);
		
		if (error != ERROR_SUCCESS)
		{
			CloseHandle(channel->file);
			UnmapViewOfFile(channel->map);
			CloseHandle(channel->server);
			CloseHandle(channel->client);
			
			if (error == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(channel->respond);
				
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_channel_open(const char *name, ipc_channel_client *channel, unsigned int *size)
	{
		int     error;
		char   *path;
		HANDLE  file;
		
		if (!name || !size || !channel)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = ipc_path(name, IPC_CHANNEL_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		//Open the shared memory file handle
		file = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, path);
		
		error = GetLastError();
		
		free(path);
		
		if (error != ERROR_SUCCESS)
		{
			if (error == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(file);
				
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Open the map of the file handle
		channel->map = (ipc_map *) MapViewOfFile(file, FILE_MAP_ALL_ACCESS, 0, 0, 0); 
		
		CloseHandle(file);
		
		if (!channel->map)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Open server semaphore
		error = ipc_path(name, IPC_CHANNEL_SEM_SERVER, &path);
		
		if (error)
		{
			UnmapViewOfFile(channel->map);
			
			return error;
		}
		
		channel->server = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, path);
		
		error = GetLastError();
		
		free(path);
		
		if (error != ERROR_SUCCESS)
		{
			UnmapViewOfFile(channel->map);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Open client semaphore
		error = ipc_path(name, IPC_CHANNEL_SEM_CLIENT, &path);
		
		if (error)
		{
			CloseHandle(channel->server);
			UnmapViewOfFile(channel->map);
			
			return error;
		}
		
		channel->client = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, path);
		
		error = GetLastError();
		
		free(path);
		
		if (error != ERROR_SUCCESS)
		{
			CloseHandle(channel->server);
			UnmapViewOfFile(channel->map);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Open response semaphore
		error = ipc_path(name, IPC_CHANNEL_SEM_RESPOND, &path);
		
		if (error)
		{
			CloseHandle(channel->server);
			CloseHandle(channel->client);
			UnmapViewOfFile(channel->map);
			
			return error;
		}
		
		channel->respond = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, path);
		
		error = GetLastError();
		
		free(path);
		
		if (error != ERROR_SUCCESS)
		{
			CloseHandle(channel->server);
			CloseHandle(channel->client);
			UnmapViewOfFile(channel->map);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Initialize duplex semaphore
		channel->duplex = NULL;
		
		//Return server size
		*size = channel->map->server_size;
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_channel_send(ipc_channel_client *channel, void *data, unsigned int size)
	{
		DWORD  error;
		HANDLE handle;
		
		if (!channel || !data)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		if (size > channel->map->server_size)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		if (!channel->map->server_pid)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		//Open server process handle to query activity
		handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, channel->map->server_pid);
		
		if (!handle)
		{
			if (GetLastError() == ERROR_INVALID_PARAMETER)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Wait for client semaphore
		while (1)
		{
			error = WaitForSingleObject(channel->client, IPC_POLLING_INTERVAL);
			
			if (error == WAIT_TIMEOUT)
			{
				if (GetExitCodeProcess(handle, &error))
				{
					if (error != STILL_ACTIVE)
					{
						CloseHandle(handle);
						
						return IPC_ERROR_INITIALIZATION_ERROR;
					}
				}
				else
				{
					CloseHandle(handle);
					
					return IPC_ERROR_SYSTEM_CALL_ERROR;
				}
			}
			else if (error == WAIT_OBJECT_0)
			{
				break;
			}
			else
			{
				CloseHandle(handle);
				
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Close server process handle
		CloseHandle(handle);
		
		//copy data (check 0 size)
		channel->map->client_size = size;
		
		if (size)
		{
			memcpy(channel->map->message, data, size);
		}
		
		//Set half duplex message
		channel->map->duplex = NULL;
		
		//Release server semaphore
		if (!ReleaseSemaphore(channel->server, 1, NULL))
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_channel_receive(ipc_channel_server *channel, void *buffer, unsigned int *size, ipc_channel_server_client *client)
	{
		DWORD error;
		
		if (!channel || !buffer || !size || !client)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		//wait for server semaphore
		error = WaitForSingleObject(channel->server, INFINITE);
		
		if (error != WAIT_OBJECT_0)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Copy client_size to size
		*size = channel->map->client_size;
		
		//Copy data to buffer if non-zero size
		if (*size)
		{
			memcpy(buffer, channel->map->message, *size);
		}
		
		//Return duplex client semaphore handle
		client->semaphore = channel->map->duplex;
		
		//Set client_semaphore to null
		channel->map->duplex = NULL;
		
		//Release client semaphore
		if (!ReleaseSemaphore(channel->client, 1, NULL))
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_channel_client_type(ipc_channel_server_client client)
	{
		return client.semaphore ? IPC_FULL_DUPLEX_CLIENT : IPC_HALF_DUPLEX_CLIENT;
	}
	
	int ipc_channel_send_and_receive(ipc_channel_client *channel, void *out_data, unsigned int out_size, void *in_buffer, unsigned int *in_size)
	{
		DWORD  error;
		HANDLE handle, duplex;
		
		if (!channel || !in_buffer || !in_size)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		if ((!out_data && out_size) || out_size > channel->map->server_size)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		//Open server process handle
		handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, FALSE, channel->map->server_pid);
		
		if (!handle)
		{
			if (GetLastError() == ERROR_INVALID_PARAMETER)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Create unnamed semaphore - initially locked
		duplex = CreateSemaphore(NULL, 0, 1, NULL);
		
		error = GetLastError();
		
		if (error != ERROR_SUCCESS)
		{
			CloseHandle(handle);
			
			if (error == ERROR_ALREADY_EXISTS)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Wait for client semaphore
		while (1)
		{
			error = WaitForSingleObject(channel->client, IPC_POLLING_INTERVAL);
			
			if (error == WAIT_TIMEOUT)
			{
				if (GetExitCodeProcess(handle, &error))
				{
					if (error != STILL_ACTIVE)
					{
						CloseHandle(handle);
						CloseHandle(duplex);
						
						return IPC_ERROR_INITIALIZATION_ERROR;
					}
				}
				else
				{
					CloseHandle(handle);
					CloseHandle(duplex);
					
					return IPC_ERROR_SYSTEM_CALL_ERROR;
				}
			}
			else if (error == WAIT_OBJECT_0)
			{
				break;
			}
			else
			{
				CloseHandle(handle);
				CloseHandle(duplex);
				
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Copy data (check 0 size)
		channel->map->client_size = out_size;
		
		if (out_size)
		{
			memcpy(channel->map->message, out_data, out_size);
		}
		
		//Duplicate semaphore handle to server process
		if (!DuplicateHandle(GetCurrentProcess(), duplex, handle, &channel->map->duplex, NULL, FALSE, DUPLICATE_SAME_ACCESS))
		{
			CloseHandle(handle);
			CloseHandle(duplex);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		channel->map->duplex_pid = GetCurrentProcessId();
		
		//Release server semaphore
		if (!ReleaseSemaphore(channel->server, 1, NULL))
		{
			CloseHandle(handle);
			CloseHandle(duplex);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Wait on unnamed semaphore
		while (1)
		{
			error = WaitForSingleObject(duplex, IPC_POLLING_INTERVAL);
			
			if (error == WAIT_TIMEOUT)
			{
				if (GetExitCodeProcess(handle, &error))
				{
					if (error != STILL_ACTIVE)
					{
						CloseHandle(handle);
						CloseHandle(duplex);
						
						return IPC_ERROR_INITIALIZATION_ERROR;
					}
				}
				else
				{
					CloseHandle(handle);
					CloseHandle(duplex);
					
					return IPC_ERROR_SYSTEM_CALL_ERROR;
				}
			}
			else if (error == WAIT_OBJECT_0)
			{
				break;
			}
			else
			{
				CloseHandle(handle);
				CloseHandle(duplex);
				
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Close handle to server process
		CloseHandle(handle);
		
		//Close handle to the duplex semaphore
		CloseHandle(duplex);
		
		//Copy size to in_size
		*in_size = channel->map->response_size;
		
		//copy data to in_buffer if non-zero size
		if (*in_size)
		{
			memcpy(in_buffer, channel->map->message + channel->map->server_size, *in_size);
		}
		
		//Release respond semaphore
		if (!ReleaseSemaphore(channel->respond, 1, NULL))
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_channel_respond(ipc_channel_server *channel, ipc_channel_server_client client, void *data, unsigned int size)
	{
		DWORD  error;
		HANDLE handle;
		
		if (!channel || !data)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		if (size > channel->map->server_size)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		//Open client process handle to query activity
		handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, client.pid);
		
		if (!handle)
		{
			if (GetLastError() == ERROR_INVALID_PARAMETER)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		if (GetExitCodeProcess(handle, &error))
		{
			if (error != STILL_ACTIVE)
			{
				CloseHandle(handle);
				
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
		}
		
		CloseHandle(handle);
		
		//Wait for respond semaphore
		error = WaitForSingleObject(channel->respond, INFINITE);
		
		if (error != WAIT_OBJECT_0)
		{
			CloseHandle(client.semaphore);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Copy data to secondary channel
		channel->map->response_size = size;
		
		//Copy data if non-zero size
		if (size)
		{
			memcpy(channel->map->message + channel->map->server_size, data, size);
		}
		
		//Release duplex semaphore
		if (!ReleaseSemaphore(client.semaphore, 1, NULL))
		{
			CloseHandle(client.semaphore);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Close handle to duplex semaphore
		CloseHandle(client.semaphore);
		
		return IPC_ERROR_NONE;
	}
	
	void ipc_channel_destroy(ipc_channel_server *channel)
	{
		if (!channel)
		{
			return;
		}
		
		if (!channel->map)
		{
			return;
		}
		
		UnmapViewOfFile(channel->map);
		CloseHandle(channel->file);
		
		CloseHandle(channel->server);
		CloseHandle(channel->client);
		CloseHandle(channel->respond);
		
		channel->map = NULL;
	}
	
	void ipc_channel_close(ipc_channel_client *channel)
	{
		if (!channel)
		{
			return;
		}
		
		if (!channel->map)
		{
			return;
		}
		
		UnmapViewOfFile(channel->map);
		
		CloseHandle(channel->server);
		CloseHandle(channel->client);
		CloseHandle(channel->respond);
		
		if (channel->duplex)
		{
			CloseHandle(channel->duplex);
		}
		
		channel->map = NULL;
	}
	
	
	int ipc_shm_create(ipc_shm *shm, const char *name, unsigned int size, void **addr)
	{
		int     error;
		char   *path;
		
		if (!name || !shm || !addr)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		if (!size)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		error = ipc_path(name, IPC_SHM_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		//Create a pagefile backed file handle
		shm->file = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, path);
		
		error = GetLastError();
		
		free(path);
		
		if (error != ERROR_SUCCESS)
		{
			if (error == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(shm->file);
				
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Map the file handle to memory
		shm->map = (ipc_shm_map *) MapViewOfFile(shm->file, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ipc_shm_map) + size); 
		
		if (!shm->map)
		{
			CloseHandle(shm->file);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		shm->map->size = size;
		
		*addr = (void *)shm->map->data;
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_shm_open(const char *name, void **addr, unsigned int *size)
	{
		int          error;
		char        *path;
		HANDLE       file;
		ipc_shm_map *map;
		
		if (!name || !size || !addr)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = ipc_path(name, IPC_SHM_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		//Open the shared memory file handle
		file = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, path);
		
		error = GetLastError();
		
		free(path);
		
		if (error != ERROR_SUCCESS)
		{
			if (error == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(file);
				
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Open the map of the file handle
		map = (ipc_shm_map *) MapViewOfFile(file, FILE_MAP_ALL_ACCESS, 0, 0, 0); 
		
		CloseHandle(file);
		
		if (!map)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		*size = map->size;
		*addr = (void *)map->data;
		
		return IPC_ERROR_NONE;
	}
	
	void ipc_shm_destroy(ipc_shm *shm)
	{
		if (!shm)
		{
			return;
		}
		
		if (!shm->map)
		{
			return;
		}
		
		UnmapViewOfFile(shm->map);
		CloseHandle(shm->file);
		
		shm->map = NULL;
	}
	
	void ipc_shm_close(void *addr)
	{
		if (!addr)
		{
			return;
		}
		
		UnmapViewOfFile((void *)((char *)addr - sizeof(ipc_shm_map)));
	}
	
	
	int ipc_semaphore_create(const char *name, unsigned int initial_value, ipc_semaphore *semaphore)
	{
		int   error;
		char *path;
		
		if (!name || !semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = ipc_path(name, IPC_SEMAPHORE_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		*semaphore = CreateSemaphore(NULL, initial_value, (LONG)(LONG_MAX), path);
		
		free(path);
		
		error = GetLastError();
		
		if (error != ERROR_SUCCESS)
		{
			if (error == ERROR_ALREADY_EXISTS)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_semaphore_open(const char *name, ipc_semaphore *semaphore)
	{
		int   error;
		char *path;
		
		if (!name || !semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = ipc_path(name, IPC_SEMAPHORE_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		*semaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, path);
		
		free(path);
		
		if (!*semaphore)
		{
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_semaphore_wait(ipc_semaphore *semaphore)
	{
		if (!semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		if (WaitForSingleObject(*semaphore, INFINITE) != WAIT_OBJECT_0)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_semaphore_trywait(ipc_semaphore *semaphore)
	{
		DWORD error;
		
		if (!semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = WaitForSingleObject(semaphore, 0);
		
		if (error == WAIT_OBJECT_0)
		{
			return IPC_ERROR_NONE;
		}
		else if (error == WAIT_TIMEOUT)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		else
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
	}
	
	int ipc_semaphore_timedwait(ipc_semaphore *semaphore, int milliseconds)
	{
		DWORD error;
		
		if (!semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = WaitForSingleObject(*semaphore, milliseconds);
		
		if (error == WAIT_OBJECT_0)
		{
			return IPC_ERROR_NONE;
		}
		else if(error == WAIT_TIMEOUT)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		else
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
	}
	
	int ipc_semaphore_post(ipc_semaphore *semaphore)
	{
		if (!semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		if (!ReleaseSemaphore(*semaphore, 1, NULL))
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return IPC_ERROR_NONE;
	}
	
	void ipc_semaphore_close(ipc_semaphore *semaphore)
	{
		if (!semaphore)
		{
			return;
		}
		
		CloseHandle(*semaphore);
	}
	
	
#else
	
	
	static int ipc_path(const char *name, int type, char **path)
	{
		const char *s;
		
		if (!name || !path)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		else if (!*name)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		s = name;
		
		while (*s)
		{
			if ((*s < 'a' || *s > 'z') && (*s < 'A' || *s > 'Z') && (*s < '0' || *s > '9') && *s != '-' && *s != '_')
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			
			s++;
		}
		
		*path = (char *)malloc(strlen(name) + 8);
		
		if (!*path)
		{
			return IPC_ERROR_MALLOC_ERROR;
		}
		
		switch(type)
		{
			case IPC_INSTANCE_PUBLIC:
			{
				strcpy(*path, "/ipc__i");
				
				break;
			}
			
			case IPC_CHANNEL_PUBLIC:
			{
				strcpy(*path, "/ipc__c");
				
				break;
			}
			
			case IPC_CHANNEL_PRIVATE:
			{
				sprintf(*path, "/ipc_c_");
				
				break;
			}
			
			case IPC_SHM_PUBLIC:
			{
				strcpy(*path, "/ipc__m");
				
				break;
			}
			
			case IPC_SHM_PRIVATE:
			{
				sprintf(*path, "/ipc_m_");
				
				break;
			}
			
			case IPC_SEMAPHORE_PUBLIC:
			{
				strcpy(*path, "/ipc__s");
				
				break;
			}
			
			case IPC_SEMAPHORE_PRIVATE:
			{
				strcpy(*path, "/ipc_s_");
				
				break;
			}
			
			case IPC_SEMAPHORE_TEMPORARY:
			{
				strcpy(*path, "/ipcs__");
				
				break;
			}
			
			default:
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
				
				break;
			}
		}
		
		strcat(*path, name);
		
		return IPC_ERROR_NONE;
	}
	
	//If Win32 version uses CreateMutex() then will share
	//a namespace with semaphores -> compensate accordingly
	int ipc_instance_create(const char *name, ipc_instance *instance)
	{
		int           error;
		struct flock  lock;
		struct stat   info;
		
		if (!name || !instance)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = ipc_path(name, IPC_INSTANCE_PUBLIC, &instance->path);
		
		if (error)
		{
			return error;
		}
		
		instance->file = shm_open(instance->path, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		if (instance->file == -1)
		{
			free(instance->path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		lock.l_type   = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start  = 0;
		lock.l_len    = 0;
		lock.l_pid    = getpid();
		
		errno = 0;
		
		if (fcntl(instance->file, F_SETLK, &lock) == -1)
		{
			close(instance->file);
			free(instance->path);
			
			if (errno == EACCES || errno == EAGAIN)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Check for nearly impossible race condition
		fstat(instance->file, &info);
		
		if (info.st_nlink == 0)
		{
			close(instance->file);
			free(instance->path);
			
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_instance_check(const char *name)
	{
		int           error, file;
		struct flock  lock;
		char         *path;
		
		if (!name)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = ipc_path(name, IPC_INSTANCE_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		file = shm_open(path, O_RDONLY, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		free(path);
		
		if (file == -1)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		lock.l_type   = F_WRLCK;
		lock.l_start  = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len    = 0;   
		
		if (fcntl(file, F_GETLK, &lock) == -1)
		{
			close(file);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		if (lock.l_type != F_WRLCK)
		{
			close(file);
			
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		close(file);
		
		return IPC_ERROR_NONE;
	}
	
	void ipc_instance_destroy(ipc_instance *instance)
	{
		if (!instance)
		{
			return;
		}
		
		if (!instance->path)
		{
			return;
		}
		
		shm_unlink(instance->path);
		
		free(instance->path);
		
		close(instance->file);
		
		instance->path = NULL;
	}
	
	
	int ipc_channel_create(const char *name, unsigned int size, ipc_channel_server *channel)
	{
		int           error;
		int           file;
		struct flock  lock;
		struct stat   info;
		
		if (!name || !channel)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		if (!size)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		error = ipc_path(name, IPC_CHANNEL_PUBLIC, &channel->channel_path);
		
		if (error)
		{
			return error;
		}
		
		error = ipc_path(name, IPC_CHANNEL_PRIVATE , &channel->lock_path);
		
		if (error)
		{
			free(channel->channel_path);
			
			return error;
		}
		
		channel->lock_file = shm_open(channel->lock_path, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		if (file == -1)
		{
			free(channel->channel_path);
			free(channel->lock_path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		lock.l_type   = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start  = 0;
		lock.l_len    = 0;
		lock.l_pid    = getpid();
		
		errno = 0;
		
		if (fcntl(channel->lock_file, F_SETLK, &lock) == -1)
		{
			close(channel->lock_file);
			free(channel->channel_path);
			free(channel->lock_path);
			
			if (errno == EACCES || errno == EAGAIN)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;	
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;	
			}
		}
		
		//Check for nearly impossible race condition
		fstat(channel->lock_file, &info);
		
		if (info.st_nlink == 0)
		{
			close(channel->lock_file);
			free(channel->channel_path);
			free(channel->lock_path);
			
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		file = shm_open(channel->channel_path, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		if (file == -1)
		{
			close(channel->lock_file);
			
			shm_unlink(channel->lock_path);
			
			free(channel->channel_path);
			free(channel->lock_path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		ftruncate(file, sizeof(ipc_map) + size * 2);
		
		channel->map = (ipc_map *)mmap(0x0, sizeof(ipc_map) + size * 2, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0x0);
		
		close(file);
		
		if (channel->map == MAP_FAILED)
		{
			close(channel->lock_file);
			
			shm_unlink(channel->channel_path);
			shm_unlink(channel->lock_path);
			
			free(channel->channel_path);
			free(channel->lock_path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		if (sem_init(&channel->map->server, 1, 0))
		{
			munmap((void *)channel->map, sizeof(ipc_map) + size * 2);
			
			close(channel->lock_file);
			
			shm_unlink(channel->channel_path);
			shm_unlink(channel->lock_path);
			
			free(channel->channel_path);
			free(channel->lock_path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		if (sem_init(&channel->map->client, 1, 1))
		{
			sem_destroy(&channel->map->server);
			
			munmap((void *)channel->map, sizeof(ipc_map) + size * 2);
			
			close(channel->lock_file);
			
			shm_unlink(channel->channel_path);
			shm_unlink(channel->lock_path);
			
			free(channel->channel_path);
			free(channel->lock_path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		if (sem_init(&channel->map->response_server, 1, 1))
		{
			sem_destroy(&channel->map->server);
			sem_destroy(&channel->map->client);
			
			munmap((void *)channel->map, sizeof(ipc_map) + size * 2);
			
			close(channel->lock_file);
			
			shm_unlink(channel->channel_path);
			shm_unlink(channel->lock_path);
			
			free(channel->channel_path);
			free(channel->lock_path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		if (sem_init(&channel->map->response_client, 1, IPC_MAX_SIMULTANEOUS_CLIENTS))
		{
			sem_destroy(&channel->map->server);
			sem_destroy(&channel->map->client);
			sem_destroy(&channel->map->response_server);
			
			munmap((void *)channel->map, sizeof(ipc_map) + size * 2);
			
			close(channel->lock_file);
			
			shm_unlink(channel->channel_path);
			shm_unlink(channel->lock_path);
			
			free(channel->channel_path);
			free(channel->lock_path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		channel->map->server_size = size;
		
		channel->map->server_pid = getpid();
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_channel_open(const char *name, ipc_channel_client *channel, unsigned int *size)
	{
		int           error;
		int           file;
		char         *path;
		struct flock  lock;
		struct stat   info;
		
		if (!name || !size || !channel)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = ipc_path(name, IPC_CHANNEL_PRIVATE, &path);
		
		if (error)
		{
			return error;
		}
		
		file = shm_open(path, O_RDONLY, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		free(path);
		
		if (file == -1)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		lock.l_type   = F_WRLCK;
		lock.l_start  = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len    = 0;   
		
		if (fcntl(file, F_GETLK, &lock) == -1)
		{
			close(file);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		close(file);
		
		if (lock.l_type != F_WRLCK)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		error = ipc_path(name, IPC_CHANNEL_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		errno = 0;
		
		file = shm_open(path, O_RDWR, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		free(path);
		
		if (file == -1)
		{
			if (errno == ENOENT)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;	
			}
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		fstat(file, &info);
		
		//If the file size is zero something is wrong
		if (!info.st_size)
		{
			close(file);
			
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		*channel = (ipc_map *)mmap(0x0, info.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0x0);
		
		close(file);
		
		if (channel == MAP_FAILED)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		*size = (info.st_size - sizeof(ipc_map)) / 2;
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_channel_send(ipc_channel_client *channel, void *data, unsigned int size)
	{
		int      error;
		ipc_map *map;
		
		#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
			struct timespec timeout;
			
			timeout.tv_sec = 0;
			timeout.tv_nsec = IPC_POLLING_INTERVAL * 1000000;
		#endif
		
		if (!channel)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		map = *channel;
		
		if (!map || (!data && size))
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		if (size > map->server_size)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		if (kill(map->server_pid, 0))
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
			while (1)
			{
				errno = 0;
				
				error = sem_timedwait(&map->client, &timeout);
				
				if (!error)
				{
					break;
				}
				else if (errno != ETIMEDOUT)
				{
					return IPC_ERROR_SYSTEM_CALL_ERROR;
				}
				
				if (kill(map->server_pid, 0))
				{
					return IPC_ERROR_INITIALIZATION_ERROR;
				}
			}
		#else
			error = sem_wait(&map->client);
			
			if (error)
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		#endif
		
		map->client_size = size;
		
		if (size)
		{
			memcpy(map->message, data, size);
		}
		
		map->client_offset = 0;
		
		sem_post(&map->server);
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_channel_receive(ipc_channel_server *channel, void *buffer, unsigned int *size, ipc_channel_server_client *client)
	{
		int error;
		
		if (!channel || !buffer || !size || !client)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = sem_wait(&channel->map->server);
		
		if (error)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		*client = channel->map->client_offset;
		
		*size = channel->map->client_size;
		
		if (channel->map->client_size)
		{
			memcpy(buffer, channel->map->message, channel->map->client_size);
		}
		
		sem_post(&channel->map->client);
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_channel_client_type(ipc_channel_server_client client)
	{
		return client ? IPC_FULL_DUPLEX_CLIENT : IPC_HALF_DUPLEX_CLIENT;
	}
	
	int ipc_channel_send_and_receive(ipc_channel_client *channel, void *out_data, unsigned int out_size, void *in_buffer, unsigned int *in_size)
	{
		int      error;
		int      i;
		ipc_map *map;
		
		#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
			struct timespec timeout;
			
			timeout.tv_sec = 0;
			timeout.tv_nsec = IPC_POLLING_INTERVAL * 1000000;
		#endif
		
		if (!channel || !in_buffer || !in_size)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		map = *channel;
		
		if (!map || (!out_data && out_size))
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		if (out_size > map->server_size)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		if (kill(map->server_pid, 0))
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		//wait for slot to open in response_client
		#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
			while (1)
			{
				errno = 0;
				
				error = sem_timedwait(&map->response_client, &timeout);
				
				if (!error)
				{
					break;
				}
				else if (errno != ETIMEDOUT)
				{
					return IPC_ERROR_SYSTEM_CALL_ERROR;
				}
				
				if (kill(map->server_pid, 0))
				{
					return IPC_ERROR_INITIALIZATION_ERROR;
				}
			}
		#else
			error = sem_wait(&map->response_client);
			
			if (error)
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		#endif
		
		//Search for open slot
		for (i = 0; i < IPC_MAX_SIMULTANEOUS_CLIENTS; i++)
		{
			if (!map->response_client_pid[i])
			{
				map->response_client_pid[i] = getpid();
				
				map->client_offset = i + 1;
				
				break;
			}
		}
		
		//This condition should not be possible: Implies tampering with underlying mechanisms
		if (i == IPC_MAX_SIMULTANEOUS_CLIENTS)
		{
			sem_post(&map->client);
			sem_post(&map->response_client);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//create locked response_sem at offset
		if (sem_init(&map->response_client_sem[i], 1, 0))
		{
			map->response_client_pid[i] = 0;
			
			sem_post(&map->client);
			sem_post(&map->response_client);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Wait for client semaphore
		#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
			while (1)
			{
				errno = 0;
				
				error = sem_timedwait(&map->client, &timeout);
				
				if (!error)
				{
					break;
				}
				else if (errno != ETIMEDOUT)
				{
					sem_destroy(&map->response_client_sem[i]);
					map->response_client_pid[i] = 0;
					sem_post(&map->response_client);
					
					return IPC_ERROR_SYSTEM_CALL_ERROR;
				}
				
				if (kill(map->server_pid, 0))
				{
					sem_destroy(&map->response_client_sem[i]);
					map->response_client_pid[i] = 0;
					sem_post(&map->response_client);
					
					return IPC_ERROR_INITIALIZATION_ERROR;
				}
			}
		#else
			error = sem_wait(&map->client);
			
			if (error)
			{
				sem_destroy(&map->response_client_sem[i]);
				map->response_client_pid[i] = 0;
				sem_post(&map->response_client);
				
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		#endif
		
		map->client_size = out_size;
		
		if (out_size)
		{
			memcpy(map->message, out_data, out_size);
		}
		
		sem_post(&map->server);
		
		//timed wait loop on response_sem
		#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
			while (1)
			{
				errno = 0;
				
				error = sem_timedwait(&map->response_client_sem[i], &timeout);
				
				if (!error)
				{
					break;
				}
				else if (errno != ETIMEDOUT)
				{
					sem_destroy(&map->response_client_sem[i]);
					map->response_client_pid[i] = 0;
					sem_post(&map->response_client);
					
					return IPC_ERROR_SYSTEM_CALL_ERROR;
				}
				
				if (kill(map->server_pid, 0))
				{
					sem_destroy(&map->response_client_sem[i]);
					map->response_client_pid[i] = 0;
					sem_post(&map->response_client);
					
					return IPC_ERROR_INITIALIZATION_ERROR;
				}
			}
		#else
			error = sem_wait(&map->response_client_sem[i]);
			
			if (error)
			{
				sem_destroy(&map->response_client_sem[i]);
				map->response_client_pid[i] = 0;
				sem_post(&map->response_client);
				
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		#endif
		
		//retrieve data from &map->message[out.size]
		//client will receive data and post response semaphore and delete response_client_sem
		if (map->response_size)
		{
			memcpy(in_buffer, &map->message[map->server_size], map->response_size);
		}
		
		sem_destroy(&map->response_client_sem[i]);
		map->response_client_pid[i] = 0;
		
		//This may be needed for compatibility with some implementations
		memset(&map->response_client_sem[i], 0, sizeof(sem_t));
		
		sem_post(&map->response_server);
		
		sem_post(&map->response_client);
		
		return IPC_ERROR_NONE;
	}
	
	//respond() synchronizes with other respond() but does not obey ->client/->server
	//respond() could occur concurrently with another processes send() -> BUT THAT's OK!!
	//it's ok because respond works on a different area of the shm region forming a concurrently
	//accesable data channel
	//respond() can still deadlock a server thread, if send_and_receive is killed before posting
	//the response_server semaphore
	//other vulnerabilities?? Possible Fix???
	//Also possible to deadlock if client connects owns client and crashes before can release server
	int ipc_channel_respond(ipc_channel_server *channel, ipc_channel_server_client client, void *data, unsigned int size)
	{
		int      error;
		int      file;
		ipc_map *map;
		char    *path;
		
		if (!channel || !data)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		if (!channel->map->response_client_pid[client - 1])
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		if (size > channel->map->server_size)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		//server will wait on response_server semaphore when cycling through respond
		error = sem_wait(&channel->map->response_server);
		
		if (error)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		if (kill(channel->map->response_client_pid[client - 1], 0))
		{
			channel->map->response_client_pid[client - 1] = 0;
			
			sem_destroy(&channel->map->response_client_sem[client - 1]);
			
			memset(&channel->map->response_client_sem[client - 1], 0, sizeof(sem_t));
			
			sem_post(&channel->map->response_client);
			
			sem_post(&channel->map->response_server);
			
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		//server will copy data to second half of shm area
		if (size)
		{
			memcpy(&channel->map->message[channel->map->server_size], data, size);
		}
		
		channel->map->response_size = size;
		
		//server will post to async_sem
		sem_post(&channel->map->response_client_sem[client - 1]);
		
		return IPC_ERROR_NONE;	
	}
	
	void ipc_channel_destroy(ipc_channel_server *channel)
	{
		if (!channel)
		{
			return;
		}
		
		if (!channel->map || !channel->lock_path || !channel->channel_path)
		{
			return;
		}
		
		sem_destroy(&channel->map->client);
		sem_destroy(&channel->map->server);
		sem_destroy(&channel->map->response_client);
		sem_destroy(&channel->map->response_server);
		
		munmap((void *)channel->map, sizeof(ipc_map) + channel->map->server_size * 2);
		
		shm_unlink(channel->lock_path);
		shm_unlink(channel->channel_path);
		
		close(channel->lock_file);
		
		free(channel->channel_path);
		free(channel->lock_path);
		
		channel->map = NULL;
		channel->lock_path = NULL;
		channel->channel_path = NULL;
	}
	
	//ipc_channel_close() cannot close a semaphore which was created by this process - use destroy
	void ipc_channel_close(ipc_channel_client *channel)
	{
		if (!channel)
		{
			return;
		}
		
		if (!*channel)
		{
			return;
		}
		
		munmap((void *)(*channel), sizeof(ipc_map) + (*channel)->server_size * 2);
		
		*channel = NULL;
	}
	
	
	int ipc_shm_create(ipc_shm *shm, const char *name, unsigned int size, void **addr)
	{
		int           error;
		int           file;
		struct flock  lock;
		struct stat   info;
		
		if (!name || !shm || !addr)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		if (!size)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		error = ipc_path(name, IPC_SHM_PUBLIC, &shm->shm_path);
		
		if (error)
		{
			return error;
		}
		
		error = ipc_path(name, IPC_SHM_PRIVATE , &shm->lock_path);
		
		if (error)
		{
			free(shm->shm_path);
			
			return error;
		}
		
		shm->lock_file = shm_open(shm->lock_path, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		if (file == -1)
		{
			free(shm->shm_path);
			free(shm->lock_path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		lock.l_type   = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start  = 0;
		lock.l_len    = 0;
		lock.l_pid    = getpid();
		
		errno = 0;
		
		if (fcntl(shm->lock_file, F_SETLK, &lock) == -1)
		{
			close(shm->lock_file);
			free(shm->shm_path);
			free(shm->lock_path);
			
			if (errno == EACCES || errno == EAGAIN)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;	
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;	
			}
		}
		
		//Check for nearly impossible race condition
		fstat(shm->lock_file, &info);
		
		if (info.st_nlink == 0)
		{
			close(shm->lock_file);
			free(shm->shm_path);
			free(shm->lock_path);
			
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		file = shm_open(shm->shm_path, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		if (file == -1)
		{
			close(shm->lock_file);
			
			shm_unlink(shm->lock_path);
			
			free(shm->shm_path);
			free(shm->lock_path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		ftruncate(file, sizeof(ipc_shm_map) + size);
		
		shm->map = (ipc_shm_map *)mmap(0x0, sizeof(ipc_shm_map) + size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0x0);
		
		close(file);
		
		if (shm->map == MAP_FAILED)
		{
			close(shm->lock_file);
			
			shm_unlink(shm->shm_path);
			shm_unlink(shm->lock_path);
			
			free(shm->shm_path);
			free(shm->lock_path);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		shm->map->size = size;
		
		*addr = (void *)shm->map->data;
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_shm_open(const char *name, void **addr, unsigned int *size)
	{
		int           error;
		int           file;
		char         *path;
		struct flock  lock;
		struct stat   info;
		ipc_shm_map  *map;
		
		if (!name || !size || !addr)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = ipc_path(name, IPC_SHM_PRIVATE, &path);
		
		if (error)
		{
			return error;
		}
		
		file = shm_open(path, O_RDONLY, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		free(path);
		
		if (file == -1)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		lock.l_type   = F_WRLCK;
		lock.l_start  = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len    = 0;   
		
		if (fcntl(file, F_GETLK, &lock) == -1)
		{
			close(file);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		close(file);
		
		if (lock.l_type != F_WRLCK)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		error = ipc_path(name, IPC_SHM_PUBLIC, &path);
		
		if (error)
		{
			return error;
		}
		
		errno = 0;
		
		file = shm_open(path, O_RDWR, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		free(path);
		
		if (file == -1)
		{
			if (errno == ENOENT)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;	
			}
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		fstat(file, &info);
		
		//If the file size is zero something is wrong
		if (!info.st_size)
		{
			close(file);
			
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		map = (ipc_shm_map *)mmap(0x0, info.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0x0);
		
		close(file);
		
		if (map == MAP_FAILED)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		*size = map->size;
		*addr = map->data;
		
		return IPC_ERROR_NONE;
	}
	
	void ipc_shm_destroy(ipc_shm *shm)
	{
		if (!shm)
		{
			return;
		}
		
		if (!shm->map || !shm->lock_path || !shm->shm_path)
		{
			return;
		}
		
		munmap((void *)shm->map, sizeof(ipc_shm_map) + shm->map->size);
		
		shm_unlink(shm->lock_path);
		shm_unlink(shm->shm_path);
		
		close(shm->lock_file);
		
		free(shm->shm_path);
		free(shm->lock_path);
		
		shm->map = NULL;
		shm->lock_path = NULL;
		shm->shm_path = NULL;
	}
	
	void ipc_shm_close(void *addr)
	{
		ipc_shm_map *map;
		
		if (!addr)
		{
			return;
		}
		
		map = (ipc_shm_map *)((char *)addr - sizeof(ipc_shm_map));
		
		munmap((void *)(map), sizeof(ipc_shm_map) + map->size);
	}
	
	
	static int ipc_semaphore_register(ipc_semaphore *semaphore)
	{
		int          error;
		struct flock lock;
		
		lock.l_type   = F_RDLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start  = 0;
		lock.l_len    = 0;
		lock.l_pid    = getpid();
		
		if (fcntl(semaphore->lockfile, F_SETLK, &lock) == -1)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return IPC_ERROR_NONE;
	}
	
	static int ipc_semaphore_check(ipc_semaphore *semaphore)
	{
		int          error;
		struct flock lock;
		
		lock.l_type   = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start  = 0;
		lock.l_len    = 0;
		
		if (fcntl(semaphore->lockfile, F_GETLK, &lock) == -1)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		if (lock.l_type != F_RDLCK)
		{
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_semaphore_create(const char *name, unsigned int initial_value, ipc_semaphore *semaphore)
	{
		int           error, file;
		struct flock  lock;
		struct stat   info;
		
		if (!name || !semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		//Allocate/Verify path strings
		error = ipc_path(name, IPC_SEMAPHORE_PUBLIC, &semaphore->path);
		
		if (error)
		{
			return error;
		}
		
		error = ipc_path(name, IPC_SEMAPHORE_PRIVATE, &semaphore->lockpath);
		
		if (error)
		{
			free(semaphore->path);
			
			return error;
		}
		
		error = ipc_path(name, IPC_SEMAPHORE_TEMPORARY, &semaphore->temppath);
		
		if (error)
		{
			free(semaphore->path);
			free(semaphore->lockpath);
			
			return error;
		}
		
		//Create or Open temporary synchronization file and write lock (block if a lock is already in place)
		file = shm_open(semaphore->temppath, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		if (file == -1)
		{
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		lock.l_type   = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start  = 0;
		lock.l_len    = 0;
		lock.l_pid    = getpid();
		
		if (fcntl(file, F_SETLKW, &lock) == -1)
		{
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			close(file);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Check for nearly impossible race condition
		fstat(file, &info);
		
		if (info.st_nlink == 0)
		{
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			close(file);
			
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		//Create or Open process counter file
		semaphore->lockfile = shm_open(semaphore->lockpath, O_RDONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		if (semaphore->lockfile == -1)
		{
			shm_unlink(semaphore->temppath);
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			close(file);
			
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Check process counter - If a lock exists or the lock read fails then semaphore creation fails
		error = ipc_semaphore_check(semaphore);
		
		if (error != IPC_ERROR_INITIALIZATION_ERROR)
		{
			shm_unlink(semaphore->temppath);
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			close(file);
			close(semaphore->lockfile);
			
			if (error)
			{
				return error;
			}
			else
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
		}
		
		//--Proceed with semaphore creation--
		
		//Create first process reference
		error = ipc_semaphore_register(semaphore);
		
		if (error)
		{
			shm_unlink(semaphore->temppath);
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			close(file);
			close(semaphore->lockfile);
		}
		
		//Unlink any existing semaphore (not sure why this is required)
		sem_unlink(semaphore->path);
		
		//Create POSIX semaphore
		semaphore->semaphore = sem_open(semaphore->path, O_CREAT, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP, initial_value);
		
		if (semaphore->semaphore == SEM_FAILED)
		{
			shm_unlink(semaphore->temppath);
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			close(file);
			close(semaphore->lockfile);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		//Close and unlink temporary file
		shm_unlink(semaphore->temppath);
		
		close(file);
		
		return IPC_ERROR_NONE;
	}
	
	int ipc_semaphore_open(const char *name, ipc_semaphore *semaphore)
	{
		int           error, file;
		struct flock  lock;
		struct stat   info;
		
		if (!semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		//Allocate name strings
		error = ipc_path(name, IPC_SEMAPHORE_PUBLIC, &semaphore->path);
		
		if (error)
		{
			return error;
		}
		
		error = ipc_path(name, IPC_SEMAPHORE_PRIVATE, &semaphore->lockpath);
		
		if (error)
		{
			free(semaphore->path);
			
			return error;
		}
		
		error = ipc_path(name, IPC_SEMAPHORE_TEMPORARY, &semaphore->temppath);
		
		if (error)
		{
			free(semaphore->path);
			free(semaphore->lockpath);
			
			return error;
		}
		
		//Attempt to open temporary synchronization file and write lock
		//If file exists than ipc_semaphore_create() is in progress
		//(block if a lock is already in place)
		
		errno = 0;
		
		file = shm_open(semaphore->temppath, O_RDWR, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		if (file == -1)
		{
			if (errno != ENOENT)
			{
				free(semaphore->path);
				free(semaphore->lockpath);
				free(semaphore->temppath);
				
				return IPC_ERROR_SYSTEM_CALL_ERROR;	
			}
		}
		else
		{
			lock.l_type   = F_WRLCK;
			lock.l_whence = SEEK_SET;
			lock.l_start  = 0;
			lock.l_len    = 0;
			lock.l_pid    = getpid();
			
			if (fcntl(file, F_SETLKW, &lock) == -1)
			{
				free(semaphore->path);
				free(semaphore->lockpath);
				free(semaphore->temppath);
				close(file);
				
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
			
			//Check for nearly impossible race condition
			fstat(file, &info);
			
			if (info.st_nlink == 0)
			{
				free(semaphore->path);
				free(semaphore->lockpath);
				free(semaphore->temppath);
				close(file);
				
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
		}
		
		//Open the process counter file
		errno = 0;
		
		semaphore->lockfile = shm_open(semaphore->lockpath, O_RDONLY, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		if (semaphore->lockfile == -1)
		{
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			close(file);
			
			if (errno == ENOENT)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		//Check process counter - If a lock does not exist then open fails
		error = ipc_semaphore_check(semaphore);
		
		if (error)
		{
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			close(file);
			close(semaphore->lockfile);
			
			return IPC_ERROR_INITIALIZATION_ERROR;
		}
		
		//--Open the semaphore--
		
		//Create the process reference
		error = ipc_semaphore_register(semaphore);
		
		if (error)
		{			
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			close(file);
			close(semaphore->lockfile);
			
			return error;
		}
		
		//Open the POSIX semaphore
		semaphore->semaphore = sem_open(semaphore->path, 0);
		
		if (semaphore->semaphore == SEM_FAILED)
		{
			free(semaphore->path);
			free(semaphore->lockpath);
			free(semaphore->temppath);
			close(file);
			close(semaphore->lockfile);
			
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		close(file);
		
		return IPC_ERROR_NONE;
	}
	
	//All wait functions must periodically check the validity of the semaphore???
	
	//must check semaphore validity before wait and during (timedwait)
	int ipc_semaphore_wait(ipc_semaphore *semaphore)
	{
		int error;
		
		if (!semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = sem_wait(semaphore->semaphore);
		
		if (error)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return IPC_ERROR_NONE;
	}
	
	//must check semaphore validity before wait and during??? (timedwait)
	int ipc_semaphore_trywait(ipc_semaphore *semaphore)
	{
		int error;
		
		if (!semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		errno = 0;
		
		error = sem_trywait(semaphore->semaphore);
		
		if (error)
		{
			if (errno == EAGAIN)
			{
				return IPC_ERROR_INITIALIZATION_ERROR;
			}
			else
			{
				return IPC_ERROR_SYSTEM_CALL_ERROR;
			}
		}
		
		return IPC_ERROR_NONE;
	}
	
	//must check semaphore validity before wait and during??? (timedwait) [this will require fancy-ness]
	#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
		int ipc_semaphore_timedwait(ipc_semaphore *semaphore, int milliseconds)
		{
			int             error;
			struct timespec timeout;
			
			if (!semaphore)
			{
				return IPC_ERROR_NULL_POINTER_ERROR;
			}
			
			timeout.tv_sec = milliseconds / 1000;
			timeout.tv_nsec = (milliseconds % 1000) * 1000000;
			
			errno = 0;
			
			error = sem_timedwait(semaphore->semaphore, &timeout);
			
			if (error)
			{
				if (errno == ETIMEDOUT)
				{
					return IPC_ERROR_INITIALIZATION_ERROR;
				}
				else
				{
					return IPC_ERROR_SYSTEM_CALL_ERROR;
				}
			}
		}
	#endif
	
	//must check semaphore validity before post
	int ipc_semaphore_post(ipc_semaphore *semaphore)
	{
		int error;
		
		if (!semaphore)
		{
			return IPC_ERROR_NULL_POINTER_ERROR;
		}
		
		error = sem_post(semaphore->semaphore);
		
		if (error)
		{
			return IPC_ERROR_SYSTEM_CALL_ERROR;
		}
		
		return IPC_ERROR_NONE;
	}
	
	//Frees the data structure and this processes association to the semaphore,
	//The semaphore ceases to "exist" when all processes that opened the semaphore
	//have called close or terminted
	void ipc_semaphore_close(ipc_semaphore *semaphore)
	{
		int          error, file;
		struct flock lock;
		struct stat  info;
		
		if (!semaphore)
		{
			return;
		}
		
		free(semaphore->path);
		free(semaphore->lockpath);
		
		error = sem_close(semaphore->semaphore);
		
		if (error)
		{
			free(semaphore->temppath);
			
			close(semaphore->lockfile);
			
			return;
		}
		
		//Create or Open temporary synchronization file and write lock (block if a lock is already in place)
		file = shm_open(semaphore->temppath, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP);
		
		free(semaphore->temppath);
		
		if (file == -1)
		{
			close(semaphore->lockfile);
			
			return;
		}
		
		lock.l_type   = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start  = 0;
		lock.l_len    = 0;
		lock.l_pid    = getpid();
		
		if (fcntl(file, F_SETLKW, &lock) == -1)
		{
			close(file);
			close(semaphore->lockfile);
			
			return;
		}
		
		fstat(file, &info);
		
		if (info.st_nlink == 0)
		{
			close(file);
			close(semaphore->lockfile);
			
			return;
		}
		
		error = ipc_semaphore_check(semaphore);
		
		if (error == IPC_ERROR_INITIALIZATION_ERROR)
		{
			sem_unlink(semaphore->path);
			shm_unlink(semaphore->lockpath);
			shm_unlink(semaphore->temppath);
		}
		
		close(file);
		close(semaphore->lockfile);
		
		return;
	}
	
	
#endif
