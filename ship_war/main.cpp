#include "game.h"

#include <windows.h>
#include <iostream>
#include <corecrt_io.h>
#include <fcntl.h>

int ship_board[HEIGHT][WIDTH] = {};
int shots_board[HEIGHT][WIDTH] = {};

using namespace std;

int main()
{
	_setmode(_fileno(stdout), _O_U16TEXT);

    setlocale(LC_CTYPE, "Russian");

    wcout << L"\nПоле кораблей:\n";
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            wcout << ship_board[i][j] << " ";
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != INVALID_HANDLE_VALUE) {
                DWORD dwMode = 0;
                if (GetConsoleMode(hOut, &dwMode)) {
                    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                    SetConsoleMode(hOut, dwMode);
                }
            }

            drawBoards(ship_board, shots_board);

            placeShip(ship_board);

            drawBoards(ship_board, shots_board);

            return 0;
        }
    }
}