/****************************************************************/
/*                                                              */
/*      Win32 API/ANSI Control Code(UNIX) Console Library       */
/*                                                              */
/*            Copyright (C) July 2010 Aaron Clovsky             */
/*                                                              */
/****************************************************************/

#ifndef CONSOLE_MODULE
#define CONSOLE_MODULE

/***************************************
Required Headers
***************************************/

#ifdef  _WIN32	
	#include <windows.h>
#else
	#include <stdio.h>
	#include <stdlib.h>
#endif

/***************************************
Constant Definitions
***************************************/

#define CONSOLE_FG_COLOR_BLACK   1
#define CONSOLE_FG_COLOR_RED     2
#define CONSOLE_FG_COLOR_GREEN   3
#define CONSOLE_FG_COLOR_YELLOW  4
#define CONSOLE_FG_COLOR_BLUE    5
#define CONSOLE_FG_COLOR_MAGENTA 6
#define CONSOLE_FG_COLOR_CYAN    7
#define CONSOLE_FG_COLOR_WHITE   8

#define CONSOLE_BG_COLOR_BLACK   (CONSOLE_FG_COLOR_BLACK << 8)
#define CONSOLE_BG_COLOR_RED     (CONSOLE_FG_COLOR_RED << 8)
#define CONSOLE_BG_COLOR_GREEN   (CONSOLE_FG_COLOR_GREEN << 8)
#define CONSOLE_BG_COLOR_YELLOW  (CONSOLE_FG_COLOR_YELLOW << 8)
#define CONSOLE_BG_COLOR_BLUE    (CONSOLE_FG_COLOR_BLUE << 8)
#define CONSOLE_BG_COLOR_MAGENTA (CONSOLE_FG_COLOR_MAGENTA << 8)
#define CONSOLE_BG_COLOR_CYAN    (CONSOLE_FG_COLOR_CYAN << 8)
#define CONSOLE_BG_COLOR_WHITE   (CONSOLE_FG_COLOR_WHITE << 8)

#define CONSOLE_STYLE_DEFAULT    0
#define CONSOLE_STYLE_BOLD       (1 << 16)

/***************************************
Function Declarations
***************************************/

//Clear the console
void console_clear(void);

//Move cursor to a given position
void console_cursor_goto(unsigned int x, unsigned int y);
//Move cursor position by a given offset
void console_cursor_move(int x, int y);

//Set or reset console text background/foreground color and style attributes
void console_style(int style);

#endif