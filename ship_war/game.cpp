#include "game.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

wstring horizonSeparator()
{
    wstring separator = L"";
    for (int j = 0; j < WIDTH + 1; j++)
    {
        separator += L"——┼";
    }
    return separator;
}

void drawBoards(const int(&ship_board)[HEIGHT][WIDTH], const int(&shots_board)[HEIGHT][WIDTH])
{
	std::wstring display;

	std::wcout << L"\x1b[?25l";	//исправить на очистку экрана
    display += L"\x1b[H";

    //Поле с корабликами
	display += L"  │";
	for (int j = 0; j < WIDTH; j++)
	{
        display += (wchar_t)(L'A' + j + 0xFEE0);
        display += L'│';
	}

    //Разрыв между полями
	display += L"\t";

    //Поле с выстрелами
    display += L"  │";
    for (int j = 0; j < WIDTH; j++)
    {
        display += (wchar_t)(L'A' + j + 0xFEE0);
        display += L'│';
    }

	display += L'\n';

    //Поле с корабликами
    display += horizonSeparator();

    //Разрыв между полями
    display += L"\t";

    //Поле с выстрелами
    display += horizonSeparator();

	display += L'\n';

    wstring number;
	for (int i = 0; i < HEIGHT; i++)
	{
        //Строка с корабликами
        number = to_wstring(i + 1);
        display += number;
        if (i + 1 < 10) { display += L' '; }
        display += L"│";

		for (int j = 0; j < WIDTH; j++)
		{
			display += (ship_board[i][j]) ? L"╳╳│" : L"XX│";
		}

        //Разрыв между полями
        display += L" \t";

        //Строка с корабликами
        number = to_wstring(i + 1);
        display += number;
        if (i + 1 < 10) { display += L' '; }
        display += L"│";

		for (int j = 0; j < WIDTH; j++)
		{
			display += (shots_board[i][j]) ? L"..│" : L"SS│";
		}

        display += L'\n';
        display += horizonSeparator();

        //Разрыв между полями
        display += L" \t";

        display += horizonSeparator();
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


void placeShip(int(&ship_board)[HEIGHT][WIDTH]){
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            ship_board[i][j] = 0;
        }
    }

    int ships_to_place[4] = { 1, 2, 3, 4 };

    while (true) {
        wcout << L"\nВведите координаты X Y, ширину и высоту корабля (например: 2 3 1 4): ";
        string user_input;
        getline(cin, user_input);

        stringstream ss(user_input);
        int x, y, w, h;
        if (!(ss >> x >> y >> w >> h)) {
            wcout << L"Неверный формат ввода!" << endl;
            continue;
        }

        int size = max(w, h);
        if (size < 1 || size > 4) {
            wcout << L"Недопустимый размер корабля!" << endl;
            continue;
        }

        int index = 4 - size;
        if (ships_to_place[index] <= 0) {
            wcout << L"Все корабли размера " << size << L" уже размещены!" << endl;
            continue;
        }

        if (x < 0 || y < 0 || x + w > WIDTH || y + h > HEIGHT) {
            wcout << L"Корабль выходит за границы поля!" << endl;
            continue;
        }

        bool can_place = true;
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                if (ship_board[y + i][x + j] == 1) {
                    can_place = false;
                    break;
                }
            }
        }

        if (!can_place) {
            wcout << L"Корабль пересекается с другим!" << endl;
            continue;
        }

        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                ship_board[y + i][x + j] = 1;
            }
        }

        ships_to_place[index]--;
        wcout << L"Корабль " << w << L"x" << h << L" размещён в (" << x << L", " << y << L")" << endl;

        bool done = true;
        for (int i = 0; i < 4; i++) {
            if (ships_to_place[i] > 0) {
                done = false;
                break;
            }
        }
        if (done) {
            wcout << L"\nВсе корабли размещены!" << endl;
            break;
        }
        break;
    }
}