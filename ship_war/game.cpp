#include "game.h"

#include <cstdlib>
#include <windows.h>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <string>
#include <locale>
#include <array>
#include <thread>
#include <conio.h>

using namespace std;

//Возврат символа для отображения ячейки
static wstring cellDisplay(int symbol, bool showShips)
{
    Cell c = static_cast<Cell>(symbol);
    switch (c) 
    {
    case Cell::Ship:  return showShips ? L"██" : L"  ";
    case Cell::Hit:   return L"\033[43m  \033[0m";//L"✖ ";
    case Cell::Miss:  return L"\033[47m  \033[0m"; //L"• ";
    case Cell::Kill:  return L"\033[40m██\033[0m";//L"☠ ";
    case Cell::Empty: return L"  ";//L"  ";
    case Cell::Cursor: return L"✖ ";
    default:          return L"??";//L"??";
    }

}

//Создать заголовок(буквы столбцов)
static wstring makeHeader()
{
    wostringstream ss;
    ss << L"   │";
    for (int j = 0; j < WIDTH; ++j) 
    {
        ss << setw(2) << left << (wchar_t)(L'A' + j) << L"│";
    }
    return ss.str();
}

static wstring makeHorizontSeparator(bool isEnd)
{
    wostringstream ss;
    wstring val = L"";
    val += (isEnd) ? L"───┴" : L"───┼";
    ss << val;
    for (int j = 0; j < WIDTH ; ++j) {
        val = L"──";
        ss << val;
        if (j + 1 < WIDTH) 
        { 
            val = (isEnd) ? L"┴" : L"┼";
            ss << val; 
        }
        else
        {
            val = (isEnd) ? L"┘" : L"┤";
            ss << val;
        }
    }
    return ss.str();
}

static wstring makeRow(const int board_row[WIDTH], int rowIndex, bool revealShips)
{
    wostringstream ss;
    ss << setw(2) << (rowIndex + 1) << L" │";
    for (int j = 0; j < WIDTH; ++j)
    {
        wstring content = cellDisplay(board_row[j], revealShips);
        ss << content << L"│";
    }
    return ss.str();
}
int zero_zero[10][10] = {};
void drawBoards(const int(&ship_board)[HEIGHT][WIDTH], const int(&shots_board)[HEIGHT][WIDTH] = zero_zero)
{
    CursorHide hide; //скроем курсор на время отрисовки

    //очистка экрана и возврат курсора в начало

    system("cls");


    wstring header = makeHeader();
    wstring sep = makeHorizontSeparator(false);
    wstring endSep = makeHorizontSeparator(true);

    array<wstring, HEIGHT * 2 + 4> leftLines;
    array<wstring, HEIGHT * 2 + 4> rightLines;

    //заголовок
    leftLines[0] = header;
    rightLines[0] = header;

    //верхняя граница под заголовком
    leftLines[1] = sep;
    rightLines[1] = sep;

    int outIndex = 2;
    for (int i = 0; i < HEIGHT; ++i) {
        leftLines[outIndex] = makeRow(ship_board[i], i, true);      //revealShips?
        rightLines[outIndex] = makeRow(shots_board[i], i, false);   //revealShips?
        ++outIndex;

        leftLines[outIndex] = (i + 1 < HEIGHT) ? sep : endSep;
        rightLines[outIndex] = (i + 1 < HEIGHT) ? sep : endSep;
        ++outIndex;
    }

    wostringstream finalOut;
    for (int i = 0; i < outIndex; i++)
    {
        finalOut << leftLines[i] << L"\t" << rightLines[i] << L'\n';
    }

    wcout << finalOut.str();
    wcout.flush();
}

bool canPlace(int(&ship_board)[HEIGHT][WIDTH], int y1, int x1, int y2, int x2) {
    if (y1 < 0 || y1 >= HEIGHT || x1 < 0 || x1 >= WIDTH ||
        y2 < 0 || y2 >= HEIGHT || x2 < 0 || x2 >= WIDTH)
    {
        wcout << L"Координаты вне поля!\n";
        return false;
    }

    int minY = min(y1, y2);
    int minX = min(x1, x2);

    int maxY = max(y1, y2);
    int maxX = max(x1, x2);

    for (int y = minY - 1; y <= maxY + 1; y++)
    {
        for (int x = minX - 1; x <= maxX + 1; x++)
        {
            if (ship_board[y][x] == 1)
            {
                wcout << L"Между кораблями должна быть пустая клетка!" << endl;
                return false;
            }
        }
    }
    return true;
}

struct ShipPlacement {
    int row;
    int col;
    int length;
    bool isHorizontal; 
};

void placeFinalShip(int(&ship_board)[HEIGHT][WIDTH], const ShipPlacement& currentShip)
{
    for (int k = 0; k < currentShip.length; ++k) {
        int r = currentShip.row + (currentShip.isHorizontal ? 0 : k);
        int c = currentShip.col + (currentShip.isHorizontal ? k : 0);

        if (r >= 0 && r < HEIGHT && c >= 0 && c < WIDTH) {
            ship_board[r][c] = 1;
        }
    }
}

void markCurrentShip(int(&temp_board)[HEIGHT][WIDTH], const int(&ship_board)[HEIGHT][WIDTH], const ShipPlacement& currentShip) {
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            temp_board[i][j] = ship_board[i][j];
        }
    }

    if (currentShip.length > 0) {
        for (int k = 0; k < currentShip.length; ++k) {
            int r = currentShip.row + (currentShip.isHorizontal ? 0 : k);
            int c = currentShip.col + (currentShip.isHorizontal ? k : 0);

            if (r >= 0 && r < HEIGHT && c >= 0 && c < WIDTH) {
                if (temp_board[r][c] != 1) {
                    temp_board[r][c] = (int)Cell::Cursor;
                }
            }
        }
    }
}



void placeShip(int(&ship_board)[HEIGHT][WIDTH])
{
    const std::unordered_map<int, int> SHIP_LIMITS = {
    {4, 1}, 
    {3, 2}, 
    {2, 3}, 
    {1, 4}  
    };
    std::vector<int> shipLengths = { 4, 3, 3, 2, 2, 2, 1, 1, 1, 1 };

    SetConsoleOutputCP(65001);
    SetConsoleCP(1251);

    std::unordered_map<int, int> placedCounts = { {4, 0}, {3, 0}, {2, 0}, {1, 0} };
    int totalShipsPlaced = 0;

    wcout << L"WASD: Перемещение, Q: Вращение, ENTER: Разместить. \n";

    for (int length : shipLengths)
    {
        if (placedCounts[length] >= SHIP_LIMITS.at(length)) {
            continue; 
        }

        ShipPlacement currentShip = { HEIGHT / 2, WIDTH / 2 - length / 2, length, true };
        bool isPlaced = false;

        while (!isPlaced)
        {
            int temp_board[HEIGHT][WIDTH];
            markCurrentShip(temp_board, ship_board, currentShip);
            drawBoards(temp_board);

            wcout << L"\nРазместите корабль длиной " << length
                << L" (" << (placedCounts[length] + 1) << L" из " << SHIP_LIMITS.at(length) << L"): ";

            wchar_t symbol = _getch();

            int maxRow = HEIGHT - (currentShip.isHorizontal ? 1 : length);
            int maxCol = WIDTH - (currentShip.isHorizontal ? length : 1);

            int prevRow = currentShip.row;
            int prevCol = currentShip.col;
            bool prevIsHorizontal = currentShip.isHorizontal;

            switch (toupper(symbol))
            {
            case L'W': case 214: case 72: 
                currentShip.row = max(0, currentShip.row - 1);
                break;
            case L'A': case 212: case 75: 
                currentShip.col = max(0, currentShip.col - 1);
                break;
            case L'S': case 219: case 80: 
                currentShip.row = min(maxRow, currentShip.row + 1);
                break;
            case L'D': case 194: case 77: 
                currentShip.col = min(maxCol, currentShip.col + 1);
                break;

            case L'Q': 
                currentShip.isHorizontal = !currentShip.isHorizontal;
                currentShip.row = min(currentShip.row, HEIGHT - (currentShip.isHorizontal ? 1 : length));
                currentShip.col = min(currentShip.col, WIDTH - (currentShip.isHorizontal ? length : 1));
                break;

            case '\r':
            {
                int y1 = currentShip.row;
                int x1 = currentShip.col;
                int y2 = y1 + (currentShip.isHorizontal ? 0 : length - 1);
                int x2 = x1 + (currentShip.isHorizontal ? length - 1 : 0);

                if (y1 < 0 || y1 >= HEIGHT || x1 < 0 || x1 >= WIDTH ||
                    y2 < 0 || y2 >= HEIGHT || x2 < 0 || x2 >= WIDTH) {
                    wcout << L"Ошибка: Корабль выходит за пределы поля! Попробуйте снова.\n";
                    break;
                }

                if (canPlace(ship_board, y1, x1, y2, x2)) {
                    placeFinalShip(ship_board, currentShip);
                    placedCounts[length]++; 
                    totalShipsPlaced++;
                    isPlaced = true; 
                    break;
                }
                else {
                    break;
                }
            }
            default:
                break;
            }
        }
    }

    drawBoards(ship_board);
    wcout << L"\nВсе 10 кораблей размещены! Вы закончили расстановку! Нажмите Enter для продолжения...";

    string temp;
    getline(cin, temp);
}