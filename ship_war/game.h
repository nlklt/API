#pragma once

#include <iostream>
#include <string>

const int WIDTH = 10;
const int HEIGHT = 10;

enum { A = 0, B = 1, C = 2, D = 3, E = 4, F = 5, G = 6, H = 7, I = 8, J = 9 };

enum class Cell : int {
    Empty = 0,
    Ship = 1,
    Hit = 2,
    Kill = 3,
    Miss = 4
};



struct CursorHide {
    CursorHide() { std::wcout << L"\x1b[?25l"; std::wcout.flush(); }    //hide
    ~CursorHide() { std::wcout << L"\x1b[?25h"; std::wcout.flush(); }   //show
};

void drawBoards(const int (&ship_board)[HEIGHT][WIDTH], const int(&shots_board)[HEIGHT][WIDTH]);
static std::wstring makeHeader();
static std::wstring makeHorizontSeparator(bool isEnd);
static std::wstring makeRow(const int board_row[WIDTH], int rowIndex, bool revealShips);

void placeShip(int (&ship_board)[HEIGHT][WIDTH]);
bool canPlace(int (&ship_board)[HEIGHT][WIDTH], int y1, int x1, int y2, int x2);
bool isShipCellAround(const int(&ship_board)[HEIGHT][WIDTH], int y, int x);

//bool checkWin();
bool makeShot(int(&shots_board)[HEIGHT][WIDTH], const int(&ship_board)[HEIGHT][WIDTH]);
