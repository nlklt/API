#include "game.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace std;


void drawBoards(const int(&ship_board)[HEIGHT][WIDTH], const int(&shots_board)[HEIGHT][WIDTH])
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


bool isPlace(int(&ship_board)[HEIGHT][WIDTH], int x, int y){
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        wcout << L"Координаты вне поля!" << endl;
        return false;
    }
    if (ship_board[x][y] == 1) {
        wcout << L"Здесь уже есть корабль!" << endl;
        return false;
    }
    return true;
}

void placeShip(int(&ship_board)[HEIGHT][WIDTH]) {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            ship_board[i][j] = 0;
        }
    }

    int ships_to_place[4] = { 1, 2, 3, 4 };

    while (true) {
        bool done = true;
        for (int i = 0; i < 4; i++) {
            if (ships_to_place[i] > 0) {
                done = false;
                break;
            }
        }
        if (done) {
            wcout << L"\n✅ Все корабли размещены! Выход из режима расстановки.\n" << endl;
            break;
        }

        wcout << L"\nВведите координаты начала и конца корабля (например: A1 A4): ";
        string input;
        getline(cin, input);

        if (input.length() < 5) {
            wcout << L"Неверный формат ввода!" << endl;
            continue;
        }

        char col1 = toupper(input[0]);
        char col2 = toupper(input[3]);
        int row1 = input[1] - '1';
        int row2 = input[4] - '1';
        int x1 = col1 - 'A';
        int x2 = col2 - 'A';
        int y1 = row1;
        int y2 = row2;

        if (x1 < 0 || x1 >= WIDTH || x2 < 0 || x2 >= WIDTH ||
            y1 < 0 || y1 >= HEIGHT || y2 < 0 || y2 >= HEIGHT) {
            wcout << L"Координаты вне поля!" << endl;
            continue;
        }

        if (x1 != x2 && y1 != y2) {
            wcout << L"Корабль должен быть строго по горизонтали или вертикали!" << endl;
            continue;
        }

        int dx = (x1 == x2) ? 0 : (x2 > x1 ? 1 : -1);
        int dy = (y1 == y2) ? 0 : (y2 > y1 ? 1 : -1);
        int length = max(abs(x2 - x1), abs(y2 - y1)) + 1;

        if (length < 1 || length > 4) {
            wcout << L"Недопустимая длина корабля!" << endl;
            continue;
        }

        int index = 4 - length;
        if (ships_to_place[index] <= 0) {
            wcout << L"Все корабли длины " << length << L" уже размещены!" << endl;
            continue;
        }

        bool can_place = true;
        int cx = x1, cy = y1;
        for (int i = 0; i < length; i++) {
            if (ship_board[cy][cx] == 1) {
                can_place = false;
                break;
            }
            cx += dx;
            cy += dy;
        }

        if (!can_place) {
            wcout << L"Корабль пересекается с другим!" << endl;
            continue;
        }

        cx = x1; cy = y1;
        for (int i = 0; i < length; i++) {
            ship_board[cy][cx] = 1;
            cx += dx;
            cy += dy;
        }

        ships_to_place[index]--;
        wcout << L"Корабль длины " << length << L" размещён!" << endl;
    }
}
