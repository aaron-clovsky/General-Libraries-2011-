/****************************************************************/
/*                                                              */
/*          XML Creation/Parsing/Modification Library           */
/*                                                              */
/*            Copyright (C) July 2010 Aaron Clovsky             */
/*                                                              */
/****************************************************************/

#ifndef XML_MODULE
#define XML_MODULE

/***************************************
Required Headers
***************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/***************************************
Error Constant Definitions
***************************************/

#define XML_ERROR_NONE                	0
#define XML_ERROR_NULL_POINTER_ERROR	1
#define XML_ERROR_INITIALIZATION_ERROR	2
#define XML_ERROR_MALLOC_ERROR        	3
#define XML_ERROR_TAG_NOT_FOUND       	4
#define XML_ERROR_SYNTAX_ERROR        	5
#define XML_ERROR_BUFFER_OVERFLOW     	6
#define XML_ERROR_FILE_IO_ERROR       	7

/***************************************
Structure/Type Declarations
***************************************/

typedef struct 
{
	unsigned int  size;
	char         *data;
} xml;

/***************************************
Function Declarations
***************************************/

//Create XML document in memory
int xml_create(xml *document);

//Free memory related to the document
void xml_destroy(xml *document);

//Create an XML document in memory from a NULL terminated string
int xml_import(xml *document, const char *data);

//Read an XML document into memory from a file
int xml_read(xml *document, const char *file_name);

//Write an XML document from memory into a file
int xml_write(xml document, const char *file_name);

//Add an open XML tag to the document
int xml_open_tag(xml *document, const char *tag, ...);

//Add a close XML tag to the document
int xml_close_tag(xml *document, const char *tag);

//Add a complete XML tag to the document
int xml_tag(xml *document, const char *tag, const char *content, ...);

//Add a complete XML tag without normal content restrictions to the document
int xml_data_tag(xml *document, const char *tag, const char *content, ...);

//Read the value of a tag or attribute in an XML document
int xml_parse(xml document, char *buffer, unsigned int size, const char *query, ...);

//Replace the value of a tag or attribute in an XML document
int xml_modify(xml *document, const char *value, const char *query, ...);

#endif