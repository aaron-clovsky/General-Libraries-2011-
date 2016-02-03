/****************************************************************/
/*                                                              */
/*                 MP3 ID3 Tag Reading Library                  */
/*                                                              */
/*            Copyright (C) July 2010 Aaron Clovsky             */
/*                                                              */
/****************************************************************/

#ifndef ID3_MODULE
#define ID3_MODULE

/***************************************
Required Headers
***************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************
Error Constant Definitions
***************************************/

#define ID3_ERROR_NONE			0
#define ID3_ERROR_NULL_POINTER_ERROR  1
#define ID3_ERROR_MALLOC_ERROR		2
#define ID3_ERROR_CANNOT_OPEN_FILE	3
#define ID3_ERROR_NO_TAG		4

/***************************************
Constant Definitions
***************************************/

#define ID3_ONLY_READ_V1		1
#define ID3_ONLY_READ_V2		2
#define ID3_ACQUIRE_IMAGE		4

#define ID3_ENCODING_ISO88591		0
#define ID3_ENCODING_UTF8		1

/***************************************
Structure/Type Declarations
***************************************/

typedef struct 
{
 	char *artist;
	char *album;
	char *title;
	char *track;
	char *year;
	void *image;
	int   image_size;
	int   image_type;
	int   encoding;
	int   version;
} id3;

/***************************************
Function Declarations
***************************************/

//Abstracts ID3v1, ID3v1.1 and ID3v2.3, ID3v2.4 tags, supports ISO8859-1 and UTF-8 encoding.
//Stores tag data in id3 structure pointed to by info
int id3_read(const char *file_name, id3 *info, int flags);

//Frees resources allocated by id3_read and stored in an id3 structure 
void id3_free(id3 info);

#endif