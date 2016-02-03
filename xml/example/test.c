#include <stdio.h>
#include "xml.h"

#define BUFFER_SIZE 1024

void main(void)
{
	char buffer[BUFFER_SIZE];
	char *value;
	xml  document;
	
	if (xml_read(&document, "input.xml"))
	{
		printf("Read Error!\n");
		
		return;
	}
	
	if (!xml_parse(document, buffer, BUFFER_SIZE, "unattend.settings.component[].DefaultSecurityDescriptor", 16))
	{
		printf("Retrieved value of\n\t\"unattend.settings.component[16].DefaultSecurityDescriptor\"\nas\n\t\"%s\"\n", buffer);
	}
	else
	{
		printf("Error!\n");
		
		return;
	}
	
	value = "Security is cool!";
	
	if (!xml_modify(&document, value, "unattend.settings.component[].DefaultSecurityDescriptor", 16))
	{
		printf("Set value of\n\t\"unattend.settings.component[16].DefaultSecurityDescriptor\"\nto\n\t\"%s\"\n", value);
	}
	else
	{
		printf("Error!\n");
		
		return;
	}
	
	if (xml_write(document, "new.xml"))
	{
		printf("Write Error!\n");
	}
	
	xml_destroy(&document);
}