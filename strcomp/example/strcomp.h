/****************************************************************/
/*                                                              */
/*               ANSI String Comparison Library                 */
/*                                                              */
/*          Copyright (C) December 2010 Aaron Clovsky           */
/*                                                              */
/****************************************************************/

#ifndef STRCOMP_MODULE
#define STRCOMP_MODULE

/***************************************
Required Headers
***************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/***************************************
Compile Time Options
***************************************/

#ifndef STRCOMP_FIXED_BUFFER
	#define STRCOMP_FIXED_BUFFER 0
#endif

/***************************************
Error Constant Definitions
***************************************/

#define STRCOMP_ERROR                 -1

/***************************************
Function Declarations
***************************************/

//Compute edit distance between two strings
int strcomp_levensthein(const char *str1, const char *str2);

//Compute longest common substring of two strings
int strcomp_lcs(const char *str1, const char *str2);

//Compute normalized edit distance between two strings
float strcomp_levensthein_normalized(const char *str1, const char *str2);

//Compute normalized longest common substring of two strings
float strcomp_lcs_normalized(const char *str1, const char *str2);

#endif