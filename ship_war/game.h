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

//������������������� (����������) ������ UTF-8
extern std::string header_utf8;   //��������� (�����)
extern std::string sep_utf8;      //����������� ����� ��������
extern std::string endsep_utf8;   //������ �����������
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

//���������������: ��������� �������� ������ �� Cell
static const std::string cellFor(int symbol, bool showShips);

void precomputeLayout();

//��������� �����
void drawBoards(const Board& ship_board, const Board& shots_board);

void placeFinalShip(Board& ship_board, const ShipPlacement& currentShip);

void markCurrentShip(Board& temp_board, const Board& ship_board, const ShipPlacement& currentShip);

//���������� �������
void placeShip(Board& ship_board);
//�����-�� ��� ���������� ������� ? true : false
bool canPlace(Board&  ship_board, int y1, int x1, int y2, int x2);
//������� �������
void makeShot(Board& my_board, Board& ship_board, Board&  shots_board);

bool checkAndFire(Board& ship_board, Board& shots_board, FirePlacement currentFire);

int getCountOfShip(const Board& b);
