#include "game.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace std;


void drawBoards(const int (&ship_boards)[HEIGHT][WIDTH], const int (&shots_board)[HEIGHT][WIDTH])
{
	std::wstring display;


	std::
		wcout << L"\x1b[?25l";	

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

bool isPlace(int ship_board[HEIGHT][WIDTH], int x, int y) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        wcout << L"���������� ��� ����!" << endl;
        return false;
    }
    if (ship_board[x][y] == 1) {
        wcout << L"����� ��� ���� �������!" << endl;
        return false;
    }
    return true;
}


void placeShip(int ship_board[HEIGHT][WIDTH]) {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            ship_board[i][j] = 0;
        }
    }

    int ships_to_place[4] = { 1, 2, 3, 4 }; 

    while (true) {
        wcout << L"\n������� ���������� X Y, ������ � ������ ������� (��������: 2 3 1 4): ";
        string user_input;
        getline(cin, user_input);

        stringstream ss(user_input);
        int x, y, w, h;
        if (!(ss >> x >> y >> w >> h)) {
            wcout << L"�������� ������ �����!" << endl;
            continue;
        }

        int size = max(w, h);
        if (size < 1 || size > 4) {
            wcout << L"������������ ������ �������!" << endl;
            continue;
        }

        int index = 4 - size;
        if (ships_to_place[index] <= 0) {
            wcout << L"��� ������� ������� " << size << L" ��� ���������!" << endl;
            continue;
        }

        if (x < 0 || y < 0 || x + w > WIDTH || y + h > HEIGHT) {
            wcout << L"������� ������� �� ������� ����!" << endl;
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
            wcout << L"������� ������������ � ������!" << endl;
            continue;
        }

        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                ship_board[y + i][x + j] = 1;
            }
        }

        ships_to_place[index]--;
        wcout << L"������� " << w << L"x" << h << L" �������� � (" << x << L", " << y << L")" << endl;

        bool done = true;
        for (int i = 0; i < 4; i++) {
            if (ships_to_place[i] > 0) {
                done = false;
                break;
            }
        }
        if (done) {
            wcout << L"\n��� ������� ���������!" << endl;
            break;
        }
    }
}