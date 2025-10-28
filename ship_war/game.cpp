#include "game.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

bool isPlace(int ship_board[11][11], int x, int y) {
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

void placeShip(int ship_board[11][11]) {
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < 11; j++) {
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

