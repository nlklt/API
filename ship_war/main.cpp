#include "game.h"

#include <windows.h>
#include <iostream>
#include <locale>

using namespace std;

int main()
{
    //устанавливаем локаль для корректного вывода Unicode в Windows/Unix
    setlocale(LC_ALL, "");
    locale::global(locale(""));

    //Ускоряет вывод
    std::ios::sync_with_stdio(false); 
    std::wcout.tie(nullptr);

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

    Board first_ship_board{};
    Board first_shots_board{};
    Board second_ship_board{};
    Board second_shots_board{};

    //// Первая доска
    //Board first_ship_board = { {
    //    {{0,0,0,0,0,0,1,0,0,0}},
    //    {{0,1,0,0,0,0,1,0,0,0}},
    //    {{0,0,0,1,0,0,1,0,0,0}},
    //    {{0,1,0,0,0,0,1,0,0,1}},
    //    {{0,0,0,0,0,0,0,0,0,1}},
    //    {{0,0,0,0,0,0,0,1,0,0}},
    //    {{0,1,1,0,0,0,0,0,0,0}},
    //    {{0,0,0,0,1,0,0,0,1,0}},
    //    {{1,0,0,0,1,0,0,0,1,0}},
    //    {{1,0,0,0,1,0,0,0,1,0}}
    //} };

    //// Вторая доска
    //Board second_ship_board = { {
    //    {{1,1,0,0,0,0,0,0,0,0}},
    //    {{0,0,0,0,1,1,1,1,0,0}},
    //    {{0,1,0,0,0,0,0,0,0,0}},
    //    {{0,0,0,0,0,1,0,0,0,0}},
    //    {{0,0,0,0,0,0,0,1,1,0}},
    //    {{0,1,1,0,0,0,0,0,0,0}},
    //    {{0,0,0,0,0,0,0,1,0,0}},
    //    {{0,1,0,1,0,0,0,1,0,0}},
    //    {{0,1,0,0,0,1,0,1,0,0}},
    //    {{0,1,0,0,0,0,0,0,0,0}}
    //} };

    //спрячем курсор
    //CursorHide hide;

    //предкомпилируем все статические строки один раз
    precomputeLayout();

    drawBoards(first_ship_board, first_shots_board);
    placeShip(first_ship_board);


    drawBoards(second_ship_board, second_shots_board);
    placeShip(second_ship_board);
    
    while (true) {
        drawBoards(first_ship_board, first_shots_board);
        std::cout << "\nХод первого игрока\n";
        makeShot(first_ship_board, second_ship_board, first_shots_board);

        Sleep(300);

        drawBoards(second_ship_board, second_shots_board);
        std::cout << "\nХод второго игрока\n";
        makeShot(second_ship_board, first_ship_board, second_shots_board);

        Sleep(300);
    }

    return 0;
}
