#include "game.h"

#include <windows.h>
#include <iostream>
#include <corecrt_io.h>
#include <cstdio>
#include <locale>
#include <locale.h>
#include <fcntl.h>

int first_ship_board[HEIGHT][WIDTH] = {};
int first_shots_board[HEIGHT][WIDTH] = {};

int second_ship_board[HEIGHT][WIDTH] = {};
int second_shots_board[HEIGHT][WIDTH] = {};

using namespace std;

int main()
{
    _setmode(_fileno(stdout), _O_U16TEXT);

    setlocale(LC_CTYPE, "Russian");

    std::ios::sync_with_stdio(false);
    std::cout.tie(nullptr);

    //устанавливаем локаль для корректного вывода Unicode в Windows/Unix
    setlocale(LC_ALL, "");
    locale::global(locale(""));

    //Ускоряет вывод
    std::ios::sync_with_stdio(false); 
    std::wcout.tie(nullptr);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }

    drawBoards(first_ship_board, first_shots_board);

    placeShip(first_ship_board);

    while (true)
    {
        drawBoards(first_ship_board, first_shots_board);
        drawBoards(first_ship_board, first_shots_board);

        Sleep(5000);

        drawBoards(second_ship_board, second_shots_board);
        drawBoards(second_ship_board, second_shots_board);
    }

    return 0;
}
