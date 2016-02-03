/****************************************************************/
/*                                                              */
/*      Win32 API/ANSI Control Code(UNIX) Console Library       */
/*                                                              */
/*            Copyright (C) July 2010 Aaron Clovsky             */
/*                                                              */
/****************************************************************/

/***************************************
Required Header
***************************************/

#include "console.h"

/***************************************
Module Functions
***************************************/

#ifdef  _WIN32
	
	
	void console_clear(void)
	{
		static HANDLE              stdout = NULL;
		COORD                      coords = {0, 0};
		DWORD                      dummy;
		CONSOLE_SCREEN_BUFFER_INFO info;
		
	  	
		if (!stdout)
		{
			stdout = GetStdHandle(STD_OUTPUT_HANDLE);
		}
	  	
		GetConsoleScreenBufferInfo(stdout, &info);
		
		FillConsoleOutputCharacter(stdout, ' ', info.dwSize.X * info.dwSize.Y, coords, &dummy);
		
		SetConsoleCursorPosition(stdout, coords);
	}
	
	void console_cursor_goto(unsigned int x, unsigned int y)
	{
		static HANDLE              stdout = NULL;
		COORD                      coord;
		CONSOLE_SCREEN_BUFFER_INFO info;
		
		if (!stdout)
		{
			stdout = GetStdHandle(STD_OUTPUT_HANDLE);
		}
		
		GetConsoleScreenBufferInfo(stdout, &info);
		
		coord.X = info.dwCursorPosition.X + 1;
		coord.Y = info.dwCursorPosition.X + 1;
		
		if (x)
		{		
			coord.X = x - 1;
		}
		
		if (y)
		{		
			coord.Y = y - 1;
		}
		
		SetConsoleCursorPosition(stdout, coord);
	}
	
	void console_cursor_move(int x, int y)
	{
		static HANDLE              stdout = NULL;
		COORD                      coord;
		CONSOLE_SCREEN_BUFFER_INFO info;
		
		if (!stdout)
		{
			stdout = GetStdHandle(STD_OUTPUT_HANDLE);
		}
		
		GetConsoleScreenBufferInfo(stdout, &info);
		
		coord.X = x + info.dwCursorPosition.X ;
		coord.Y = y + info.dwCursorPosition.Y ;
		
		SetConsoleCursorPosition(stdout, coord);
	}
	
	void console_style(int style)
	{
		static HANDLE                     stdout = NULL;
		static CONSOLE_SCREEN_BUFFER_INFO defaults;
		int                               bg, fg, effect;
		unsigned char                     map[] = {0, 4, 2, 6, 1, 5, 3, 7};
		CONSOLE_SCREEN_BUFFER_INFO        info;
		
		if (!stdout)
		{
			stdout = GetStdHandle(STD_OUTPUT_HANDLE);
			GetConsoleScreenBufferInfo(stdout, &defaults);
		}
		
		if (!style)
		{
			SetConsoleTextAttribute(stdout, defaults.wAttributes);
			
			return;
		}
		
		fg = style & 15;
		bg = (style >> 8) & 15;
		effect = style >> 16;
		
		if (fg > 8 || bg > 8 || fg & ~15 || bg & ~15 || effect & ~3)
		{
			return;
		}
		
		GetConsoleScreenBufferInfo(stdout, &info);
		
		if (fg)
		{
			info.wAttributes &= ~15;
			
			fg = map[fg - 1];
			
			if (effect & 1)
			{
				fg |= 8;
			}
			
			info.wAttributes |= fg;
		}
		
		if (bg)
		{	
			info.wAttributes &= ~240;
			
			bg = map[bg - 1];
			
			info.wAttributes |= bg << 4;
		}
		
		SetConsoleTextAttribute(stdout, info.wAttributes);
	}
	
	
#else
	
	
	void console_clear(void)
	{
		system("tput clear");
	}
	
	void console_cursor_goto(unsigned int x, unsigned int y)
	{
		if (x)
		{
			printf("\033[%dG", x);
		}
		
		if (y)
		{
			printf("\033[%dd", y);
		}
	}
	
	void console_cursor_move(int x, int y)
	{
		if (x > 0)
		{
			printf("\033[%dC", x);
		}
		else
		{
			printf("\033[%dD", -x);
		}
		
		if (y > 0)
		{
			printf("\033[%dB", y);
		}
		else
		{
			printf("\033[%dA", -y);
		}
	}
	
	void console_style(int style)
	{
		int bg, fg, effect;
		
		printf("\033[0m");
		
		if (!style)
		{
			return;
		}
		
		fg = style & 15;
		bg = (style >> 8) & 15;
		effect = style >> 16;
		
		if (fg > 8 || bg > 8 || fg & ~15 || bg & ~15 || effect & ~3)
		{
			return;
		}
		
		fg += 29;
		bg += 39;
		
		if (fg != 29)
		{
			printf("\033[%dm", fg);
		}
		
		if (bg != 39)
		{
			printf("\033[%dm", bg);
		}
		
		if (effect & 1)
		{
			printf("\033[1m");
		}
	}

	
#endif