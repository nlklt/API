#include "game.h"

#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <windows.h>

int first_ship_board[HEIGHT][WIDTH] = {};
int first_shots_board[HEIGHT][WIDTH] = {};

int second_ship_board[HEIGHT][WIDTH] = {};
int second_shots_board[HEIGHT][WIDTH] = {};

using namespace std;

int main()
{
    //установка кодировки вывода в UTF-8
    SetConsoleOutputCP(CP_UTF8);
    //отключение синхронизации с stdio для скорости
    std::ios::sync_with_stdio(false);
    std::cout.tie(nullptr);

    //включим VT-последовательности (ANSI) в консоли Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }

    //спрячем курсор
    CursorHide hide;

    //предкомпилируем все статические строки один раз
    precomputeLayout();

    drawBoards(first_ship_board, first_shots_board);
    
    placeShip(first_ship_board);
    placeShip(second_ship_board);
    
    while (true)
    {
        drawBoards(first_ship_board, first_shots_board);
        drawBoards(first_ship_board, first_shots_board);

        Sleep(500);

        drawBoards(second_ship_board, second_shots_board);
        drawBoards(second_ship_board, second_shots_board);
    }

    return 0;
}
