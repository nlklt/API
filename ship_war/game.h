#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>

const int WIDTH = 10;
const int HEIGHT = 10;

enum class Cell : int {
    Empty = 0,
    Ship = 1,
    Hit = 2,
    Kill = 3,
    Miss = 4,
    Cursor = 9,
};

//предкомпилированные (глобальные) строки UTF-8
extern std::string header_utf8;   //заголовок (буквы)
extern std::string sep_utf8;      //разделитель между строками
extern std::string endsep_utf8;   //нижний разделитель
extern bool precomputed;

void precomputeLayout();

struct CursorHide {
    CursorHide() { std::wcout << L"\x1b[?25l"; std::wcout.flush(); }    //hide
    ~CursorHide() { std::wcout << L"\x1b[?25h"; std::wcout.flush(); }   //show
};

void drawBoards(const int (&ship_board)[HEIGHT][WIDTH], const int(&shots_board)[HEIGHT][WIDTH]);

void placeShip(int(&ship_board)[HEIGHT][WIDTH]);
bool canPlace(int (&ship_board)[HEIGHT][WIDTH], int y1, int x1, int y2, int x2);
bool isShipCellAround(const int(&ship_board)[HEIGHT][WIDTH], int y, int x);
std::unordered_map<std::string, int> getCountOfShip(const int(&ships_of_type)[HEIGHT][WIDTH]);

//bool checkWin();
bool makeShot(int(&shots_board)[HEIGHT][WIDTH], const int(&ship_board)[HEIGHT][WIDTH]);
