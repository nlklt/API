#pragma once

#include <array>
#include <vector>
#include <string>
#include <unordered_map>

const int WIDTH = 10;
const int HEIGHT = 10;

enum class Cell : int {
    Empty   = 0,
    Ship    = 1,
    Hit     = 2,
    Kill    = 3,
    Miss    = 4,
    Cursor  = 9,
};

using Board = std::array<std::array<int, WIDTH>, HEIGHT>;

//предкомпилированные (глобальные) строки UTF-8
extern std::string header_utf8;   //заголовок (буквы)
extern std::string sep_utf8;      //разделитель между строками
extern std::string endsep_utf8;   //нижний разделитель
extern bool precomputed;

struct ShipPlacement {
    int row;
    int col;
    int length;
    bool isHorizontal;
};

struct FirePlacement {
    int row;
    int col;
};

//struct CursorHide {
//    CursorHide() { std::cout << "\x1b[?25l"; std::cout.flush(); }    //hide
//    ~CursorHide() { std::cout << "\x1b[?25h"; std::cout.flush(); }   //show
//};

//¬спомогательное: безопасно получить строку по Cell
static const std::string cellFor(int symbol, bool showShips);

void precomputeLayout();

//ќтрисовка полей
void drawBoards(const Board& ship_board, const Board& shots_board);

void placeFinalShip(Board& ship_board, const ShipPlacement& currentShip);

void markCurrentShip(Board& temp_board, const Board& ship_board, const ShipPlacement& currentShip);

//–азместить корабль
void placeShip(Board& ship_board);
//ћожем-ли так разместить корабль ? true : false
bool canPlace(Board&  ship_board, int y1, int x1, int y2, int x2);
//—делать выстрел
void makeShot(Board& my_board, Board& ship_board, Board&  shots_board);

bool checkAndFire(Board& ship_board, Board& shots_board, FirePlacement currentFire);

int getCountOfShip(const Board& b);
