#include <stdio.h>
#include "id3.h"

void id3_print(id3 tag);

int main(int argc, char **argv)
{
	char  choice;
	char *image_name;
	FILE *image_file;
	id3   info;
	
	if (argc != 2)
	{
		printf("Usage: %s <file name>\n", (strrchr(argv[0], '/') ? strrchr(argv[0], '/') : strrchr(argv[0], '\\')) + 1);
		
		exit(0);
	}
	
	if (id3_read(argv[1], &info, ID3_ACQUIRE_IMAGE))
	{
		printf("Error reading mp3 tag!\n");
	}
	else
	{
		id3_print(info);
		
		if (info.image)
		{
			printf("Extract image file (y/n): ");
			
			choice = getchar();
			
			if (choice == 'y')
			{
				image_name = malloc(strlen(argv[1]) + 5);
				
				strcpy(image_name, argv[1]);
				
				*strrchr(image_name, '.') = '\0';
				
				strcat(image_name, info.image_type ? ".png" : ".jpg");
				
				image_file = fopen(image_name, "wb");
				
				fwrite(info.image, info.image_size, 1, image_file);
				
				fclose(image_file);
				
				free(image_name);
			}
		}
		//CRASH CAN HAPPEN HERE!!! WTF???
		id3_free(info);
	}
	
	return 0;
}

void id3_print(id3 tag)
{
	printf("Tag Information\n");
	
	if (tag.encoding == ID3_ENCODING_ISO88591 || tag.encoding == ID3_ENCODING_UTF8)
	{
		printf("\tArtist\t%s\n", tag.artist);
		printf("\tAlbum\t%s\n", tag.album);
		printf("\tTitle\t%s\n", tag.title);
		
		if (tag.year)
		{
			printf("\tYear\t%s\n", tag.year);
		}
		if (tag.track)
		{
			printf("\tTrack\t#%s\n", tag.track);
		}
		if (tag.image)
		{
			printf("Found %s album art\n", tag.image_type ? "png" : "jpeg");
		}
		
		printf("Tag version read: %s\n", tag.version == 1 ? "1.x" : "2.x");
		
		printf("Tag Encoding: %s\n", tag.encoding ==  ID3_ENCODING_UTF8 ? "UTF-8" : "ISO8859-1");
	}
	else
	{
		printf("Unprintable tag data\n");
	}
}