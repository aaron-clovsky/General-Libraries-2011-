/****************************************************************/
/*                                                              */
/*                 MP3 ID3 Tag Reading Library                  */
/*                                                              */
/*            Copyright (C) July 2010 Aaron Clovsky             */
/*                                                              */
/****************************************************************/

/***************************************
Required Header and Compiler Options Check (GCC/VC++/Other)
***************************************/

#include "id3.h"

#ifdef  _WIN32
	#include <windows.h>
	
	#if !defined(_MSC_VER) || (defined(_MSC_VER) && !defined(_MT))
		#define malloc(x)     HeapAlloc  (GetProcessHeap(), 0, x)
		#define realloc(x, y) HeapReAlloc(GetProcessHeap(), 0, x, y)
		#define free(x)	      HeapFree   (GetProcessHeap(), 0, x)
	#endif
#endif

/***************************************
Structure/Type Declarations
***************************************/

typedef struct 
{
	unsigned char tag[3];
	unsigned char title[30];
	unsigned char artist[30];
	unsigned char album[30];
	unsigned char year[4];
	unsigned char comments[28];
	unsigned char zero;
	unsigned char track;
	unsigned char genre;
} id3_v1;

typedef struct 
{
	unsigned char tag[3];
	unsigned char version;
	unsigned char revision;
	unsigned char flags;
	unsigned char size[4];
} id3_v2_header;

typedef struct 
{
	unsigned char name[4];
	unsigned char size[4];
	unsigned char flags[2];
} id3_v2_frame_header;

typedef struct 
{
	id3_v2_header  header;
	unsigned char *frame;
	unsigned long  frame_size;
	char          *title;
	unsigned long  title_length;
	char          *album;
	unsigned long  album_length;
	char          *artist;
	unsigned long  artist_length;
	char          *year;
	unsigned long  year_length;
	char          *track;
	unsigned long  track_length;
	void          *image;
	int            image_size;
	int            image_type;
	int            encoding;
} id3_v2;

/***************************************
Module Functions
***************************************/

static int id3_v1_allocate(id3_v1 *v1, id3 *tag)
{
	int   size, artist_length, album_length, title_length;
	char *s;
	
	artist_length = v1->artist[29] ? 30 : strlen(v1->artist);
	album_length = v1->album[29] ? 30 : strlen(v1->album);
	title_length = v1->title[29] ? 30 : strlen(v1->title);
	
	size = artist_length + album_length + title_length + 3;
	size += (v1->year[0] && v1->year[0] != ' ') ? 5 : 0;
	size += (!v1->zero) ? 4 : 0;
	
	s = (char *)malloc(size);
	
	if (!s)
	{
		return ID3_ERROR_MALLOC_ERROR;
	}
	
	tag->artist = s;
	memcpy(tag->artist, v1->artist, artist_length);
	tag->artist[artist_length] = '\0';
	
	s += artist_length + 1;
	
	tag->album = s;
	memcpy(tag->album, v1->album, album_length);
	tag->album[album_length] = '\0';
	
	s += album_length + 1;
	
	tag->title = s;
	memcpy(tag->title, v1->title, title_length);
	tag->title[title_length] = '\0';
	
	s += title_length + 1;
	
	if (v1->year[0] && v1->year[0] != ' ')
	{
		tag->year = s;
		memcpy(tag->year, v1->year, 4);
		tag->year[4]  = '\0';
		
		s += 5;
	}
	else
	{
		tag->year = NULL;
	}
	
	if (!v1->zero)
	{
		tag->track = s;
		
		if ((int)v1->track > 99)
		{
			tag->track[0] = (v1->track / 100) + 48;
			tag->track[1] = ((v1->track % 100) / 10) + 48;
			tag->track[2] = ((v1->track % 100) % 10) + 48;
			tag->track[3] = '\0';
		}
		else if ((int)v1->track > 9)
		{
			tag->track[0] = (v1->track / 10) + 48;
			tag->track[1] = (v1->track % 10) + 48;
			tag->track[2] = '\0';
		}
		else
		{
			tag->track[0] = '0';
			tag->track[1] = v1->track + 48;
			tag->track[2] = '\0';
		}
	}
	else
	{
		tag->track = NULL;
	}
	
	s = tag->artist;
	while (*s) *s++ = *s == '\n' ? ' ' : *s;
	s = tag->album;
	while (*s) *s++ = *s == '\n' ? ' ' : *s;
	s = tag->title;
	while (*s) *s++ = *s == '\n' ? ' ' : *s;
	
	return ID3_ERROR_NONE;
}

static unsigned long id3_v2_size(unsigned char *sz)
{
	unsigned long size = 0;

	int i;

	for (i = 3; i >= 0; i--)
	{
		if (!(sz[i] & 128))
		{
			size |= (sz[i] & 127) << ((3 - i) * 7);
		}
		else
		{
			break;
		}
	}

	return size;
}

static void id3_v2_parse(id3_v2 *tag, int flags)
{
	unsigned char *pos;
	
	unsigned long i, size;
	
	id3_v2_frame_header *header;
	
	i = 0;
	
	if (tag->header.flags & 64)
	{
		if (tag->header.version == 4)
		{
			size = id3_v2_size(tag->frame);
		}
		else
		{
			((unsigned char *)&size)[0] = tag->frame[3];
			((unsigned char *)&size)[1] = tag->frame[2];
			((unsigned char *)&size)[2] = tag->frame[1];
			((unsigned char *)&size)[3] = tag->frame[0];
		}
		
		i += size;
	}
	
	while (i < tag->frame_size)
	{
		header = (id3_v2_frame_header *)(&tag->frame[i]);
		
		if (tag->header.version == 4)
		{
			size = id3_v2_size(header->size);
		}
		else
		{
			((unsigned char *)&size)[0] = header->size[3];
			((unsigned char *)&size)[1] = header->size[2];
			((unsigned char *)&size)[2] = header->size[1];
			((unsigned char *)&size)[3] = header->size[0];
		}
		
		if (!memcmp(header->name, "APIC", 4))
		{
			if (flags & ID3_ACQUIRE_IMAGE)
			{
				if (!strcmp(&tag->frame[i + 11], "png") || !strcmp(&tag->frame[i + 11], "image/png"))
				{
					tag->image_type = 1;
				}
				else if (!strcmp(&tag->frame[i + 11], "jpeg") || !strcmp(&tag->frame[i + 11], "image/jpeg")
				     ||  !strcmp(&tag->frame[i + 11], "jpg")  || !strcmp(&tag->frame[i + 11], "image/jpg"))
				{
					tag->image_type = 0;
				}
				
				pos = &tag->frame[i + 11];
				
				while (*pos++);
				
				pos++;
				
				while (*pos++);
				
				if (tag->frame[i + 10] && tag->frame[i + 10] != 3)
				{
					pos++;
				}
				
				tag->image = pos;
				tag->image_size = (size - (pos - &tag->frame[i + 10]));
			}
		}		
		else
		{
			if (tag->frame[i + 10] && tag->frame[i + 10] != 3)
			{
				break;
			}
			else
			{
				tag->encoding = tag->frame[i + 10] ? ID3_ENCODING_UTF8 : ID3_ENCODING_ISO88591;
			}
			
			if (!memcmp(header->name, "TIT2", 4))
			{
				tag->title = &tag->frame[i + 11];
				tag->title_length = size - 1;
			}
			else if (!memcmp(header->name, "TALB", 4))
			{
				tag->album = &tag->frame[i + 11];
				tag->album_length = size - 1;
			}
			else if (!memcmp(header->name, "TPE1", 4))
			{
				tag->artist = &tag->frame[i + 11];
				tag->artist_length = size - 1;
			}
			else if (!memcmp(header->name, "TRCK", 4))
			{
				tag->track = &tag->frame[i + 11];
				
				pos = tag->track;
				
				while (pos < (&tag->frame[i + 11] + size - 1) && *pos && *pos != '/') pos++;
				
				tag->track_length = pos - &tag->frame[i + 11];
			}
			else if (!memcmp(header->name, "TYER", 4) || !memcmp(header->name, "TDRC", 4))
			{
				tag->year = &tag->frame[i + 11];
				tag->year_length = size - 1;
			}
			else if (!memcmp(header->name, "\0\0\0\0", 4))
			{
				break;
			}
		}

		i += size + 10;
	}	
}

void id3_free(id3 info)
{
	free(info.artist);
	
	if (info.image)
	{
		free(info.image);
	}
}

int id3_read(const char *file_name, id3 *info, int flags)
{
	char  *s;
	id3_v2  v2;
	id3_v1  v1;
	FILE  *input;
	
	if (!file_name || !info)
	{
		return ID3_ERROR_NULL_POINTER_ERROR;
	}
	
	input = fopen(file_name, "rb");
	
	if (!input)
	{
		return ID3_ERROR_CANNOT_OPEN_FILE;
	}
	
	memset(info, 0, sizeof(id3));
	
	if (!(flags & ID3_ONLY_READ_V2))
	{
		fseek(input, -0x80, SEEK_END);
		fread(&v1, sizeof(id3_v1), 1, input);
		
		if (!memcmp(v1.tag, "TAG", 3) && v1.artist[0] && v1.album[0] && v1.title[0])
		{
			info->version = 1;
			
			info->image = NULL;
			
			if (flags & ID3_ONLY_READ_V1 || (!(flags & ID3_ACQUIRE_IMAGE) && !v1.artist[29] && !v1.album[29] && !v1.title[29]))
			{
				fclose(input);
				
				if (id3_v1_allocate(&v1, info))
				{
					return ID3_ERROR_MALLOC_ERROR;
				}
				
				return ID3_ERROR_NONE;
			}
		}
	}
	
	if (flags & ID3_ONLY_READ_V1 && !(flags & ID3_ONLY_READ_V2) && !(flags & ID3_ACQUIRE_IMAGE))
	{
		fclose(input);
		
		return ID3_ERROR_NO_TAG;
	}
	
	memset(&v2, 0, sizeof(id3_v2));
	
	fseek(input, 0, SEEK_SET);
	fread(&v2.header, sizeof(id3_v2_header), 1, input);
	
	if (memcmp(v2.header.tag, "ID3", 3) || v2.header.version < 3 || v2.header.version > 4)
	{
		fclose(input);
		
		if (info->version == 1)
		{
			if (id3_v1_allocate(&v1, info))
			{
				return ID3_ERROR_MALLOC_ERROR;
			}
			
			return ID3_ERROR_NONE;
		}
	
		return ID3_ERROR_NO_TAG;
	}
	else
	{
		v2.frame_size = id3_v2_size((unsigned char *)&v2.header.size);
		
		v2.frame = (unsigned char *)malloc(v2.frame_size);
		
		if (!v2.frame)
		{
			fclose(input);
			
			return ID3_ERROR_MALLOC_ERROR;
		}
		
		fseek(input, sizeof(id3_v2_header), SEEK_SET);
		fread(v2.frame, v2.frame_size, 1, input);
		
		id3_v2_parse(&v2, flags);
		
		if (!v2.artist || !v2.album || !v2.title)
		{
			if (info->version == 1)
			{
				free(v2.frame);
				
				if (id3_v1_allocate(&v1, info))
				{
					fclose(input);
					
					return ID3_ERROR_MALLOC_ERROR;
				}
			}
			else
			{
				free(v2.frame);
				
				return ID3_ERROR_NO_TAG;
			}
		}
		else
		{
			info->artist = malloc(v2.artist_length + v2.album_length + v2.title_length + (v2.year ? v2.year_length + 1 : 0) + (v2.track ? v2.track_length  + 1 : 0) + 3);
			
			if (!info->artist)
			{
				fclose(input);
				
				return ID3_ERROR_MALLOC_ERROR;
			}
			
			memcpy(info->artist, v2.artist, v2.artist_length);
			info->artist[v2.artist_length] = '\0';
			
			info->album = info->artist + v2.artist_length + 1;
			memcpy(info->album, v2.album, v2.album_length);
			info->album[v2.album_length] = '\0';
			
			info->title = info->album + v2.album_length + 1;
			memcpy(info->title, v2.title, v2.title_length);
			info->title[v2.title_length] = '\0';
			
			if (v2.year)
			{
				info->year = info->title + v2.title_length + 1;	
				memcpy(info->year, v2.year, v2.year_length);
				info->year[v2.year_length] = '\0';
			}
			
			if (v2.track)
			{
				if (v2.year)
				{
					info->track = info->year + v2.year_length + 1;
					
					if (v2.track_length == 1)
					{
						*info->track++ = '0';
					}
					
					memcpy(info->track, v2.track, v2.track_length);
					info->track[v2.track_length] = '\0';
				}
				else
				{
					info->track = info->title + v2.title_length + 1;
					
					if (v2.track_length == 1)
					{
						*info->track++ = '0';
					}
					
					memcpy(info->track, v2.track, v2.track_length);
					info->track[v2.track_length] = '\0';
				}
			}
			
			if (v2.image)
			{
				memmove(v2.frame, v2.image, v2.image_size);
				
				info->image = realloc(v2.frame, v2.image_size);
				
				if (!info->image)
				{
					free(info->artist);
					
					free(v2.frame);
					
					fclose(input);
				
					return ID3_ERROR_MALLOC_ERROR;
				}
				
				info->image_size = v2.image_size;
				info->image_type = v2.image_type;
			}
			else
			{
				free(v2.frame);
			}
			
			info->encoding = v2.encoding;
			
			info->version = 2;
		}
	}
	
	fclose(input);
	
	return ID3_ERROR_NONE;
}