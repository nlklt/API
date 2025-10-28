#include "game.h"

#include <iostream>
#include <string>

void drawBoards(const int (&ship_boards)[HEIGHT][WIDTH], const int (&shots_board)[HEIGHT][WIDTH])
{
	std::wstring display;


	std::
		wcout << L"\x1b[?25l";	//исправить на очистку экрана

	display += L"  ";
	for (int j = 0; j < WIDTH; j++)
	{
		display += (wchar_t)(L'A' + j + 0xFEE0);
	}

	display += L'\t';

	for (int i = 0; i < HEIGHT; i++)
	{
		display += L"  ";
		for (int j = 0; j < WIDTH; j++)
		{
			display += ship_boards[i][j];
		}
	}

	for (int num = 1; num < HEIGHT; num++)
	{

	}
}