/****************************************************************/
/*                                                              */
/*          XML Creation/Parsing/Modification Library           */
/*                                                              */
/*            Copyright (C) July 2010 Aaron Clovsky             */
/*                                                              */
/****************************************************************/

/***************************************
Compile Time Options
***************************************/

#ifndef XML_ALLOCATION_UNIT
	#define XML_ALLOCATION_UNIT 1024
#else
	#if XML_ALLOCATION_UNIT <= 0
		#error The value specified for XML_ALLOCATION_UNIT is invalid
	#endif
#endif

/***************************************
Required Header and Compiler Options Check (GCC/VC++/Other)
***************************************/

#include "xml.h"

#ifdef  _WIN32
	#include <windows.h>
	
	#if !defined(_MSC_VER) || (defined(_MSC_VER) && !defined(_MT))
		#define malloc(x)     HeapAlloc  (GetProcessHeap(), 0, x)
		#define realloc(x, y) HeapReAlloc(GetProcessHeap(), 0, x, y)
		#define free(x)	      HeapFree   (GetProcessHeap(), 0, x)
	#endif
#endif

/***************************************
Module Functions
***************************************/

static int xml_insert(xml *document, const char *tag, const char *content, int type, const char *attr_name[], const char *attr_value[], unsigned int len)
{
	unsigned int  i, attr_length, size;
	const char   *s;
	
	attr_length = 0;
	
	for (i = 0; i < len; i++)
	{
		s = attr_name[i];
		
		while (*s && *s != '&') s++;
		
		attr_length += s - attr_name[i] + strlen(attr_value[i]) + 4;
	}
	
	size = strlen(document->data) + (len ? attr_name[0] - tag - 1 : strlen(tag)) * (content ? 2 : 1) + attr_length + 4 + (type  > 1 ? (content ? strlen(content) + (type == 2 ? 3 : 15): 1) : type ? 1 : 0);
	
	if (size > document->size)
	{
		document->size = (size / XML_ALLOCATION_UNIT + 1) * XML_ALLOCATION_UNIT;
		document->data = (char *)realloc(document->data, document->size);
		
		if (!document->data)
		{
			return XML_ERROR_MALLOC_ERROR;
		}
	}
	
	if (type == 1)
	{
		strcat(document->data, "</");
	}
	else
	{
		strcat(document->data, "<");
	}
	
	if (len)
	{
		document->data[strlen(document->data) + (attr_name[0] - tag - 1)] = '\0';
		memcpy(document->data + strlen(document->data), tag, attr_name[0] - tag - 1);
	}
	else
	{
		strcat(document->data, tag);
	}
	
	for (i = 0; i < len; i++)
	{
		strcat(document->data, " ");
		
		if (i != len - 1)
		{
			document->data[strlen(document->data) + (attr_name[i + 1] - attr_name[i] - 1)] = '\0';
			memcpy(document->data + strlen(document->data), attr_name[i], attr_name[i + 1] - attr_name[i] - 1);
		}
		else
		{
			strcat(document->data, attr_name[i]);
		}
		
		strcat(document->data, "=\"");
		
		if (attr_value[i])
		{
			strcat(document->data, attr_value[i]);
		}
		
		strcat(document->data, "\"");
	}
	
	if (content)
	{
		strcat(document->data, type == 2 ? ">" : "><![CDATA[");
		strcat(document->data, content);
		strcat(document->data, type == 2 ? "</" : "]]></");
		
		if (len)
		{
			document->data[strlen(document->data) + (attr_name[0] - tag - 1)] = '\0';
			memcpy(document->data + strlen(document->data), tag, attr_name[0] - tag - 1);
		}
		else
		{
			strcat(document->data, tag);
		}
		
		strcat(document->data, ">\n");
	}
	else
	{
		if (type > 1)
		{
			strcat(document->data, "/>\n");
		}
		else
		{
			strcat(document->data, ">\n");
		}
	}
	
	return XML_ERROR_NONE;
}

static char *xml_strstr(char *start, char *end, char *search, int len)
{	
	while (start < end)
	{
		if (start < end - 9 && !memcmp(start, "<![CDATA[", 9))
		{
			start +=  9;
			
			while (start < end - len - 3 && memcmp(start++, "]]>", 3));
			
			if (start < end - len - 3)
			{
				start +=  3;
			}
		}
		
		if (start > end - len)
		{	
			break;
		}
		
		if (!memcmp(start, search, len))
		{
			return start;
		}
		
		start++;
	}
	
	return NULL;
}

static char *xml_skip_tag(char *open, char *end)
{
	int   i, taglen;
	char *s, *ss, *close, *nextopen, *nextclose, *nextnextopen, *nextnextclose;
	
	s = open;
	
	if (open[1] == '!')
	{
		while (s < end && *s != '>') s++;
		
		s++;
		
		return s;
	}
	
	while (s < end && *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n' && *s != '/' && *s != '>') s++;
	
	if (s >= end)
	{
		return NULL;
	}
	
	switch (*s)
	{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
		{
			taglen = s - open;
			
			while (s < end && *s != '>') s++;
			
			if (s == end)
			{
				return NULL;
			}			
			
			if (*(s - 1) != '/')
			{
				s++;
				
				break;
			}
			else
			{
				s--;
			}
		}
		
		case '/':
		{
			s++;
			
			if (*s != '>')
			{
				return NULL;
			}
			
			s++;
			
			return s;
		}
		
		case '>':
		{
			taglen = s - open;
			
			s++;
			
			break;
		}
	}
	
	close = (char *) malloc(taglen + 2);
	close[0] = '<';
	close[1] = '/';	
	memcpy(close + 2, open + 1, taglen - 1);
	close[taglen + 1] = '\0';
	
	i = 0;
	
	while (1)
	{
		nextopen = xml_strstr(s, end, open, taglen);
		
		if (nextopen)
		{
			ss = nextopen;
			
			while (ss < end && *ss != '>') ss++;
			
			ss--;
			
			if (*ss == '/')
			{
				nextopen = xml_strstr(ss, end, open, taglen);
			}
		}
		
		nextclose = xml_strstr(s, end, close, taglen + 1);
		
		if (!nextclose)
		{
			free(close);
			
			return NULL;
		}
		
		if (!nextopen || nextopen > nextclose)
		{
			if (i)
			{
				if (nextopen)
				{
					nextnextclose = xml_strstr(nextclose + taglen + 1, end, close, taglen + 1);
					
					if (nextnextclose && nextnextclose < nextopen)
					{
						i--;
						s = nextclose + taglen + 1;
					}
					else
					{
						s = nextopen + taglen;
					}
				}
				else
				{
					for (; i; i--)
					{
						s = nextclose + taglen + 1;
						
						nextclose = xml_strstr(s, end, close, taglen + 1);
						
						if (!nextclose)
						{
							free(close);
							
							return NULL;
						}
					}
				}
			}
			else
			{
				free(close);
				
				s = nextclose + taglen + 1;
				
				while (s < end && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')) s++;
				
				if (*s != '>')
				{
					return NULL;
				}
				
				s++;
				
				return s;
			}
		}
		else
		{
			i++;
			s = nextopen + taglen;
		}
	}
}

static int xml_rw(char *buffer, unsigned int size, xml *document, const char *query, int indices[])
{
	int      i, index, counter;
	char    *copy, *element, *start, *end, *tag, *openbracket, *closebracket, *attribute;
	va_list  list;
	
	if (!buffer || !query)
	{
		return XML_ERROR_NULL_POINTER_ERROR;
	}
	
	if (!document->data || !document->size)
	{
		return XML_ERROR_INITIALIZATION_ERROR;
	}
	
	counter = 0;
	
	start = document->data;
	
	end = document->data + strlen(document->data);
	
	while (start < end && *start != '<') start++;
	
	if (start == end || *start != '<')
	{
		return XML_ERROR_SYNTAX_ERROR;
	}
	
	if (start[1] == '?')
	{
		start += 2;
		
		while (start < end && *start != '?') start++;
		
		if (start == end || start[1] != '>')
		{
			return XML_ERROR_SYNTAX_ERROR;
		}
		
		start += 2;
		
		while (start < end && (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')) start++;
		
		if (start == end || *start != '<')
		{
			return XML_ERROR_SYNTAX_ERROR;
		}
	}
	
	copy = (char *)malloc(strlen(query) + 1);
	strcpy(copy, query);
	
	attribute = strchr(copy, '&');
	
	if (attribute)
	{
		*attribute++ = '\0';
		
		if (strchr(attribute, '&'))
		{
			free(copy);
			
			return XML_ERROR_INITIALIZATION_ERROR;
		}
	}
	
	element = strtok(copy, ".");
	
	while (element)
	{
		if (strchr(element, ' ') || strchr(element, '\t') || strchr(element, '\r') || strchr(element, '\n'))
		{
			free(copy);
			
			return XML_ERROR_SYNTAX_ERROR;
		}
		
		index = 0;
		openbracket = strchr(element, '[');
		
		if (openbracket)
		{
			if (openbracket == element || openbracket[1] != ']' || openbracket[2])
			{
				free(copy);
				
				return XML_ERROR_INITIALIZATION_ERROR;
			}
			
			index = indices[counter++];
			
			*openbracket = '\0';
		}
		else
		{
			closebracket = strchr(element, ']');
			
			if (closebracket)
			{
				free(copy);
				
				return XML_ERROR_INITIALIZATION_ERROR;
			}
		}
		
		if (size > strlen(element) + 2)
		{
			tag = buffer;
		}
		else
		{
			tag = (char *)malloc(strlen(element) + 2);
		}
		
		*tag = '<'; 
		strcpy(tag + 1, element);
		
		while (start < end && (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')) start++;
		
		if (start == end || *start != '<')
		{
			free(copy);
			
			if (size <= strlen(element) + 2)
			{
				free(tag);
			}
			
			return XML_ERROR_SYNTAX_ERROR;
		}
		
		while (1)
		{
			if (!memcmp(start, tag, strlen(element) + 1))
			{
				if (index)
				{
					index--;
				}
				else
				{
					break;
				}
			}
			
			start = xml_skip_tag(start, end);
			
			if (!start)
			{
				free(copy);
				
				if (size <= strlen(element) + 2)
				{
					free(tag);
				}
				
				return XML_ERROR_TAG_NOT_FOUND;
			}
			
			while (start < end && (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')) start++;
			
			if (start == end || *start != '<')
			{
				free(copy);
				
				if (size <= strlen(element) + 2)
				{
					free(tag);
				}
				
				return XML_ERROR_SYNTAX_ERROR;
			}
		}
		
		end = xml_skip_tag(start, end);
		
		if (!end)
		{
			free(copy);
			
			if (size <= strlen(element) + 2)
			{
				free(tag);
			}
			
			return XML_ERROR_TAG_NOT_FOUND;
		}
		
		if (*(end - 2) == '/')
		{
			start = end - 1;
		}
		else
		{
			while (*--end != '<');
		}
		
		while (start < end && *start != '>') start++;
		
		start++;
		
		if (size <= strlen(element) + 2)
		{
			free(tag);
		}
		
		element = strtok(NULL, ".");
	}
	
	if (attribute)
	{
		if (start == end)
		{
			end -= 2;
			
			start = end;
		}
		else
		{
			end = start;
			
			while (*++end != '>');
			
			end -= 2;
		}
		
		while (*--start != '<');
		
		while (start < end && *start != ' ' && *start != '\t' && *start != '\r' && *start != '\n') start++;
		
		if (start == end)
		{
			free(copy);
			
			return XML_ERROR_SYNTAX_ERROR;
		}
		
		while (start < end && *start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') start++;
		
		if (start == end)
		{
			free(copy);
			
			return XML_ERROR_SYNTAX_ERROR;
		}
		
		i = strlen(attribute);
		
		while (memcmp(start, attribute, i))
		{
			while (start < end && *start != '=') start++;		
			
			if (start == end)
			{
				free(copy);
				
				return XML_ERROR_SYNTAX_ERROR;
			}
			
			start++;
			
			while (start < end && *start != '\"') start++;		
			
			if (start == end)
			{
				free(copy);
				
				return XML_ERROR_SYNTAX_ERROR;
			}
			
			start++;
			
			while (start < end && *start != '\"') start++;		
			
			if (start == end)
			{
				free(copy);
				
				return XML_ERROR_SYNTAX_ERROR;
			}
			
			start++;
			
			while (start < end && *start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') start++;
			
			if (start == end)
			{
				free(copy);
				
				return XML_ERROR_SYNTAX_ERROR;
			}
		}
		
		start += i;
		
		while (start < end && *start != '=') start++;		
		
		if (start == end)
		{
			free(copy);
			
			return XML_ERROR_SYNTAX_ERROR;
		}
		
		start++;
		
		while (start < end && *start != '\"') start++;		
		
		if (start == end)
		{
			free(copy);
			
			return XML_ERROR_SYNTAX_ERROR;
		}
		
		start++;
		
		end = strchr(start, '\"');
		
		if (!end)
		{
			free(copy);
			
			return XML_ERROR_SYNTAX_ERROR;
		}
	}
	
	free(copy);
	
	if (size)
	{
		copy = buffer;
		
		while(start < end && buffer < copy + size - 1)
		{
			if (!memcmp(start, "<![CDATA[", 9))
			{
				start += 9;
			}
			
			if (!memcmp(start, "]]>", 3))
			{
				start += 3;
			}
			
			*buffer++ = *start++;
		}
		
		*buffer = '\0';
		
		if (start != end)
		{
			return XML_ERROR_BUFFER_OVERFLOW;
		}
	}
	else
	{	
		i = (((start - document->data) + (document->data + strlen(document->data) - end) + strlen(buffer) + 1) / XML_ALLOCATION_UNIT + 1) * XML_ALLOCATION_UNIT;
		
		copy = (char *)malloc(i);
		
		if (!copy)
		{
			return XML_ERROR_MALLOC_ERROR;
		}
		
		memcpy(copy, document->data, (start - document->data));
		copy[start - document->data] = '\0';
		strcat(copy, buffer);
		strcat(copy, end);
		
		free(document->data);
		document->data = copy;
		
		document->size = i;
	}
	
	return XML_ERROR_NONE;
}

int xml_create(xml *document)
{
	char *xml_tag = "<?xml version=\"1.0\"?>\n";
	
	if (!document)
	{
		return XML_ERROR_NULL_POINTER_ERROR;
	}
	
	document->size = strlen(xml_tag) > XML_ALLOCATION_UNIT ? (strlen(xml_tag) / XML_ALLOCATION_UNIT + 1) * XML_ALLOCATION_UNIT : XML_ALLOCATION_UNIT;
	document->data = (char *)malloc(document->size);
	
	if (!document->data)
	{
		return XML_ERROR_MALLOC_ERROR;
	}
	
	strcpy(document->data, xml_tag);
	
	return XML_ERROR_NONE;
}

void xml_destroy(xml *document)
{
	if (!document)
	{
		return;
	}
	
	if (!document->data || !document->size)
	{
		return;
	}
	
	free(document->data);

	document->size = 0;
}

int xml_open_tag(xml *document, const char *tag, ...)
{
	int         i, len;
	const char *s, *attr_name[256], *attr_value[256];
	va_list     list;
	
	if (!document || !tag)
	{
		return XML_ERROR_NULL_POINTER_ERROR;
	}
	
	if (!document->size || !document->data)
	{
		return XML_ERROR_INITIALIZATION_ERROR;
	}
	
	len = 0;
	
	s = tag;
	
	while (*s)
	{
		if (*s++ == '&')
		{
			if (len > 255)
			{
				return XML_ERROR_INITIALIZATION_ERROR;
			}
			
			attr_name[len++] = s;
		}
	}
	
	va_start(list, tag);
	
	for (i = 0; i < len; i++)
	{
		attr_value[i] = va_arg(list, char *);
		
		if (!attr_value[i])
		{
			return XML_ERROR_NULL_POINTER_ERROR;
		}
	}
	
	va_end(list);
	
	return xml_insert(document, tag, NULL, 0, attr_name, attr_value, len);
}

int xml_close_tag(xml *document, const char *tag)
{
	if (!document || !tag)
	{
		return XML_ERROR_NULL_POINTER_ERROR;
	}
	
	if (!document->size || !document->data)
	{
		return XML_ERROR_INITIALIZATION_ERROR;
	}
	
	return xml_insert(document, tag, NULL, 1, NULL, NULL, 0);
}

int xml_tag(xml *document, const char *tag, const char *content, ...)
{
	int         i, len;
	const char *s, *attr_name[256], *attr_value[256];
	va_list     list;
	
	if (!document || !tag)
	{
		return XML_ERROR_NULL_POINTER_ERROR;
	}
	
	if (!document->size || !document->data)
	{
		return XML_ERROR_INITIALIZATION_ERROR;
	}
	
	len = 0;
	
	s = tag;
	
	while (*s)
	{
		if (*s++ == '&')
		{
			if (len > 255)
			{
				return XML_ERROR_INITIALIZATION_ERROR;
			}
			
			attr_name[len++] = s;
		}
	}
	
	va_start(list, content);
	
	for (i = 0; i < len; i++)
	{
		attr_value[i] = va_arg(list, char *);
		
		if (!attr_value[i])
		{
			return XML_ERROR_NULL_POINTER_ERROR;
		}
	}
	
	va_end(list);
	
	return xml_insert(document, tag, content, 2, attr_name, attr_value, len);
}

int xml_data_tag(xml *document, const char *tag, const char *content, ...)
{
	int         i, c, len;
	const char *s, *attr_name[256], *attr_value[256];
	va_list     list;
	
	if (!document || !tag)
	{
		return XML_ERROR_NULL_POINTER_ERROR;
	}
	
	if (!document->size || !document->data)
	{
		return XML_ERROR_INITIALIZATION_ERROR;
	}
	
	len = 0;
	
	s = tag;
	
	while (*s)
	{
		if (*s++ == '&')
		{
			if (len > 255)
			{
				return XML_ERROR_INITIALIZATION_ERROR;
			}
			
			attr_name[len++] = s;
		}
	}
	
	va_start(list, content);
	
	for (i = 0; i < len; i++)
	{
		attr_value[i] = va_arg(list, char *);
		
		if (!attr_value[i])
		{
			return XML_ERROR_NULL_POINTER_ERROR;
		}
	}
	
	va_end(list);
	
	return xml_insert(document, tag, content, 3, attr_name, attr_value, len);
}

int xml_import(xml *document, const char *data)
{
	if (!document || !data)
	{
		return XML_ERROR_NULL_POINTER_ERROR;
	}
	
	document->size = strlen(data) + 1;
	document->data = (char *)malloc(document->size);
	
	if (!document->data)
	{
		return XML_ERROR_MALLOC_ERROR;
	}
	
	strcpy(document->data, data);
	
	return XML_ERROR_NONE;
}

int xml_read(xml *document, const char *file_name)
{
	FILE *handle;
	long  size;
	
	if (!document || !file_name)
	{
		return XML_ERROR_NULL_POINTER_ERROR;
	}
	
	handle = fopen(file_name, "rb");
	
	if (!handle)
	{
		return XML_ERROR_FILE_IO_ERROR;
	}
	
	if (fseek(handle, 0, SEEK_END))
	{
		return XML_ERROR_FILE_IO_ERROR;
	}
	
	size = ftell(handle);
	
	if (size == -1L)
	{
		return XML_ERROR_FILE_IO_ERROR;
	}
	
	rewind(handle);
	
	document->size = size;
	document->data = (char *)malloc(size + 1);
	
	if (!document->data)
	{
		return XML_ERROR_MALLOC_ERROR;
	}
	
	if (fread(document->data, 1, size, handle) != (unsigned long)size)
	{
		free(document->data);
		
		return XML_ERROR_FILE_IO_ERROR;
	}
	
	fclose(handle);
	
	document->data[document->size++] = '\0';
	
	return XML_ERROR_NONE;
}

int xml_write(xml document, const char *file_name)
{
	FILE *handle;
	
	if (!document.size || !document.data)
	{
		return XML_ERROR_NULL_POINTER_ERROR;
	}
	
	if (!file_name)
	{
		return XML_ERROR_INITIALIZATION_ERROR;
	}
	
	handle = fopen(file_name, "wb");
	
	if (!handle)
	{
		return XML_ERROR_FILE_IO_ERROR;
	}
	
	if (fwrite(document.data, 1, strlen(document.data), handle) != strlen(document.data))
	{
		return XML_ERROR_FILE_IO_ERROR;
	}
	
	fclose(handle);
	
	return XML_ERROR_NONE;
}

int xml_parse(xml document, char *buffer, unsigned int size, const char *query, ...)
{
	int         len, indices[256];
	const char *s;
	va_list     list;
	
	len = 0;
	
	va_start(list, query);
	
	s = query;
	
	while (*s)
	{
		if (s[0] == '[' && s[1] == ']')
		{
			if (len > 255)
			{
				return XML_ERROR_INITIALIZATION_ERROR;
			}
			
			indices[len++] = va_arg(list, int);
		}
		
		s++;
	}
	
	va_end(list);
	
	return xml_rw(buffer, (int)size, &document, query, indices);
}

int xml_modify(xml *document, const char *value, const char *query, ...)
{
	int         len, indices[256];
	const char *s;
	va_list  list;
	
	if (!document)
	{
		return XML_ERROR_NULL_POINTER_ERROR;
	}
	
	len = 0;
	
	va_start(list, query);
	
	s = query;
	
	while (*s)
	{
		if (s[0] == '[' && s[1] == ']')
		{
			if (len > 255)
			{
				return XML_ERROR_INITIALIZATION_ERROR;
			}
			
			indices[len++] = va_arg(list, int);
		}
		
		s++;
	}
	
	va_end(list);
	
	return xml_rw((char *)value, 0, document, query, indices);
}