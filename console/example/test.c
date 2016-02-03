#include <stdio.h>
#include "console.h"

void main () 
{
	console_clear();
	printf("Coardinates is misspelled?\n");
	console_cursor_move(2, -1);
	printf("o");
	console_cursor_move(-3, 1);
	printf("nope!\n");
	console_cursor_move(6, 3);
	printf("it's fine!\n");
	console_cursor_goto(10, 10);
	console_style(CONSOLE_FG_COLOR_RED);
	printf("this ");
	console_style(CONSOLE_FG_COLOR_GREEN | CONSOLE_BG_COLOR_WHITE);
	printf("is at");
	console_style(0);
	printf(" (10, 10)\n");
	console_cursor_move(3, 1);
	console_style(CONSOLE_FG_COLOR_BLUE);
	printf("Seriously");
	console_style(CONSOLE_FG_COLOR_BLUE | CONSOLE_STYLE_BOLD);
	printf(" Now!\n\n");
	console_style(CONSOLE_STYLE_DEFAULT);
}