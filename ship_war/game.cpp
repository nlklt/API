#include "game.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace std;


void drawBoards(const int (&ship_board)[HEIGHT][WIDTH], const int (&shots_board)[HEIGHT][WIDTH])
{
	std::wstring display;


	std::wcout << L"\x1b[?25l";	//исправить на очистку экрана
    display += L"\x1b[H";

    //Поле с корабликами
	display += L"  ";
	for (int j = 0; j < WIDTH; j++)
	{
		display += (wchar_t)(L'A' + j + 0xFEE0);
	}

    //Разрыв между полями
	display += L"\t┃\t";

    //Поле с выстрелами
    display += L"  ";
    for (int j = 0; j < WIDTH; j++)
    {
        display += (wchar_t)(L'A' + j + 0xFEE0);
    }

	display += L'\n';

    wstring number;
	for (int i = 0; i < HEIGHT; i++)
	{
        //Строка с корабликами
        number = to_wstring(i + 1);
        display += number;
        if (i + 1 < 10) { display += L' '; }

		for (int j = 0; j < WIDTH; j++)
		{
			display += (ship_board[i][j]) ? L"██" : L"  ";
		}

        //Разрыв между полями
        display += L" \t┃\t";

        //Строка с корабликами
        number = to_wstring(i + 1);
        display += number;
        if (i + 1 < 10) { display += L' '; }

		for (int j = 0; j < WIDTH; j++)
		{
			display += (ship_board[i][j]) ? L".." : L"  ";
		}
        display += L'\n';
	}

    wcout << display;
}

bool isPlace(int (&ship_board)[HEIGHT][WIDTH], int x, int y) {
    if (x < 0 || x >= 11 || y < 0 || y >= 11) {
        wcout << L"Координаты вне поля!" << endl;
        return false;
    }
    if (ship_board[x][y] == 1) {
        wcout << L"Здесь уже есть корабль!" << endl;
        return false;
    }
    return true;
}

void placeShip(int (&ship_board)[HEIGHT][WIDTH]) {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            ship_board[i][j] = 0;
        }
    }

    while (true) {
        wcout << L"Введите координаты корабля (x y), например: 4 3: ";
        string user_input;
        getline(cin, user_input);

        stringstream ss(user_input);
        int x, y;
        if (!(ss >> x >> y)) {
            wcout << L"Неверный формат ввода!" << endl;
            continue;
        }

        if (isPlace(ship_board, x, y)) {
            ship_board[x][y] = 1;
            wcout << L"Корабль размещён в точке (" << x << ", " << y << ")" << endl;
            break;
        }
    }
}

