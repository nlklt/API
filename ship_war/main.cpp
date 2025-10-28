#include "game.h"

#include <iostream>
#include <corecrt_io.h>
#include <fcntl.h>
#include "game.h"

int ship_board[HEIGHT][WIDTH], shots_board[HEIGHT][WIDTH];

using namespace std;

int main()
{
	_setmode(_fileno(stdout), _O_U16TEXT);

    setlocale(LC_CTYPE, "Russian");
    placeShip(ship_board);

    wcout << L"\nПоле кораблей:\n";
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < 11; j++) {
            wcout << ship_board[i][j] << " ";
        }
        wcout << endl;
    }

    return 0;
}