/****************************************************************/
/*                                                              */
/*               ANSI String Comparison Library                 */
/*                                                              */
/*          Copyright (C) December 2010 Aaron Clovsky           */
/*                                                              */
/****************************************************************/

/***************************************
Required Header and Compiler Options Check (GCC/VC++/Other)
***************************************/

#include "strcomp.h"

#ifdef  _WIN32
	#include <windows.h>
	
	#if !defined(_MSC_VER) || (defined(_MSC_VER) && !defined(_MT))
		#define malloc(x)     HeapAlloc  (GetProcessHeap(), 0, x)
		#define realloc(x, y) HeapReAlloc(GetProcessHeap(), 0, x, y)
		#define free(x)	      HeapFree   (GetProcessHeap(), 0, x)
	#endif
#endif

/***************************************
Inline Functions
***************************************/

#define MIN(a, b, c) ((a) <= (b) && (a) <= (c)) ? (a) : (((b) <= (a) && (b) <= (c)) ? (b): (c))

/***************************************
Module Functions
***************************************/

//Edit Distance
int strcomp_levensthein(const char *str1, const char *str2)
{
	int i, k, j;
	int n, m;
	int cost;
	
	if (!str1 || !str2)
	{
		return STRCOMP_ERROR;
	}
	
	#if (STRCOMP_FIXED_BUFFER > 0)
		int  d[STRCOMP_MAX_BUFFER];
	#else
		int *d;
	#endif
	
	n = strlen(str1); 
	m = strlen(str2);
	
	#if (STRCOMP_FIXED_BUFFER > 0)
		if (n && m && (sizeof(int) * (m+1) * (n+1)) <= STRCOMP_MAX_BUFFER)
	#else
		d = (int *)malloc(sizeof(int) * (m+1) * (n+1));
		
		if (n && m)
	#endif
	{
		m++;
		n++;
		
		for(k = 0; k < n; k++)
		{
			d[k] = k;
		}
		
		for(k = 0; k < m; k++)
		{
			d[k * n] = k;
		}
		
		for(i = 1; i < n; i++)
		{
			for(j = 1; j < m; j++)
			{
				if ((str1[i-1] & ~32) == (str2[j-1] & ~32))
				{
					cost = 0;
				}
				else
				{
					cost = 1;
				}
				
				d[j * n + i] = MIN(d[(j-1) * n + i] + 1, d[j * n + i - 1] + 1, d[(j-1) * n + i - 1] + cost);
			}
		}
		
		#if (STRCOMP_FIXED_BUFFER == 0)
			free(d);
		#endif
		
		return d[n * m - 1];
	}
	else 
	{
		return STRCOMP_ERROR;
	}
}

//Normalized Edit Distance
float strcomp_levensthein_normalized(const char *str1, const char *str2)
{
	int n, m;
	
	if (!str1 || !str2)
	{
		return STRCOMP_ERROR;
	}
	
	n = strlen(str1);
	m = strlen(str2);
	
	n = (n >= m) ? n : m;
	
	m = strcomp_levensthein(str1, str2);
	
	if (m == STRCOMP_ERROR)
	{
		return STRCOMP_ERROR;
	}
	else
	{
		return 1 - (float)m / n;
	}
}

//Longest common substring
int strcomp_lcs(const char *str1, const char *str2)
{
	int         tmp, max;
	const char *s, *q, *ss, *qq;
	
	if (!str1 || !str2)
	{
		return STRCOMP_ERROR;
	}
	
	max = 0;
	
	s = str1;
	
	while (*s)
	{
		q = str2;
		
		while (*q)
		{
			ss = s;
			qq = q;
			
			tmp = 0;
			
			while((*ss & ~32) == (*qq & ~32) && *qq && *ss)
			{
				tmp++;
				ss++;
				qq++;
			}
			
			if (tmp > max)
			{
				max = tmp;
			}
			
			q++;
		}
		
		s++;
	}
	
	return max;
}

//Normalized longest common substring
float strcomp_lcs_normalized(const char *str1, const char *str2)
{
	int n, m;
	
	if (!str1 || !str2)
	{
		return STRCOMP_ERROR;
	}
	
	n = strlen(str1);
	m = strlen(str2);
	
	n = (n >= m) ? n : m;
	
	return (float)strcomp_lcs(str1, str2) / n;
}