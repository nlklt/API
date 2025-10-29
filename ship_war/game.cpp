#include "game.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <unordered_map>
#include <windows.h>
#include <conio.h>
#include <thread>
#include <array>

using namespace std;

//определения внешних переменных
string header_utf8;
string sep_utf8;
string endsep_utf8;
bool precomputed = false;

//Вспомогательное: безопасно получить строку по Cell
static const string cellFor(int symbol, bool showShips)
{
    Cell c = static_cast<Cell>(symbol);
    switch (c)
    {
    case Cell::Ship:  return showShips ? u8"██" : u8"  ";
    case Cell::Hit:   return u8"✖ ";
    case Cell::Miss:  return u8"• ";
    case Cell::Kill:  return u8"☠ ";
    case Cell::Empty: return u8"  ";
    case Cell::Cursor:return u8"[]";
    default:          return u8"??";
    }
}

void precomputeLayout()
{
    if (precomputed) return;
    precomputed = true;

    //header: "   │ A│ B│ C│"
    {
        std::string s;
        s.reserve(256);
        s += "   ";
        s += u8"│";
        for (int j = 0; j < WIDTH; ++j) 
        {
            char buf[8];
            snprintf(buf, sizeof(buf), " %c", 'A' + j);
            s += buf;
            s += u8"│";
        }
        header_utf8 = std::move(s);
    }

    //sep: "───┼──┼"
    {
        std::string s;
        s.reserve(256);
        s += u8"───";
        s += u8"┼";
        for (int j = 0; j < WIDTH; ++j) {
            s += u8"──";
            if (j + 1 < WIDTH) s += u8"┼";
            else s += u8"┤";
        }
        sep_utf8 = std::move(s);
    }

    //end separator
    {
        std::string s;
        s.reserve(256);
        s += u8"───";
        s += u8"┴";
        for (int j = 0; j < WIDTH; ++j) {
            s += u8"──";
            if (j + 1 < WIDTH) s += u8"┴";
            else s += u8"┘";
        }
        endsep_utf8 = std::move(s);
    }
}

int zero_zero[HEIGHT][WIDTH] = {};
//Быстрая отрисовка: собираем большой std::string и один вызов записи
void drawBoards(const int(&ship_board)[HEIGHT][WIDTH], const int(&shots_board)[HEIGHT][WIDTH] = zero_zero)
{
    if (!precomputed) precomputeLayout();

    std::string out;
    //рассчитаем примерный размер и зарезервируем
    //каждая строк содержит: left_line (~ 3 + WIDTH*(2+1) ) + tab + right_line + '\n'
    size_t approx_per_line = 3 + (WIDTH * 3) + 1 + (WIDTH * 3) + 2; // грубая оценка
    out.reserve((HEIGHT * 2 + 6) * approx_per_line);

    //очистка экрана и возврат курсора в начало (используем VT)
    out += "\x1b[2J\x1b[H"; //clear screen + cursor home

    //заголовки
    out += header_utf8;
    out += '\t';
    out += header_utf8;
    out += '\n';

    out += sep_utf8;
    out += '\t';
    out += sep_utf8;
    out += '\n';

    //строки досок
    for (int i = 0; i < HEIGHT; ++i) {
        //номер строки (2 знака) + " │"
        char rownum[8];
        snprintf(rownum, sizeof(rownum), "%2d ", i + 1);
        out += rownum;
        out += u8"│";

        //левая доска (ship_board) — revealShips = true
        for (int j = 0; j < WIDTH; ++j) {
            const std::string& cell = cellFor(ship_board[i][j], true);
            out += cell;
            out += u8"│";
        }

        //TAB между досками
        out += '\t';

        //правая доска (shots_board) — revealShips = false
        snprintf(rownum, sizeof(rownum), "%2d ", i + 1);
        out += rownum;
        out += u8"│";

        for (int j = 0; j < WIDTH; ++j) 
        {
            const std::string& cell = cellFor(shots_board[i][j], false);
            out += cell;
            out += u8"│";
        }

        out += '\n';

        //разделитель между строками
        if (i + 1 < HEIGHT) 
        {
            out += sep_utf8;
            out += '\t';
            out += sep_utf8;
            out += '\n';
        }
        else 
        {
            out += endsep_utf8;
            out += '\t';
            out += endsep_utf8;
            out += '\n';
        }
    }

    //запись в консоль единым вызовом (Windows)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) 
    {
        DWORD written = 0;
        //WriteFile работает с байтами, а у нас UTF-8 в std::string — всё ок.
        WriteFile(hOut, out.data(), static_cast<DWORD>(out.size()), &written, nullptr);
    }
    else 
    {
        // fallback
        std::cout << "FFFFUUUUUUUUCCCCKKKKKK" << out;
        std::cout.flush();
    }
}

bool canPlace(int(&ship_board)[HEIGHT][WIDTH], int y1, int x1, int y2, int x2) {
    if (y1 < 0 || y1 >= HEIGHT || x1 < 0 || x1 >= WIDTH ||
        y2 < 0 || y2 >= HEIGHT || x2 < 0 || x2 >= WIDTH)
    {
        std::cout << "Координаты вне поля!\n";
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
                std::cout << "Между кораблями должна быть пустая клетка!" << endl;
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

    std::cout << "WASD: Перемещение, Q: Вращение, ENTER: Разместить. \n";

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

            std::cout << "\nРазместите корабль длиной " << length
                << " (" << (placedCounts[length] + 1) << " из " << SHIP_LIMITS.at(length) << "): ";

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
            case L'R': 
                currentShip.isHorizontal = !currentShip.isHorizontal;
                currentShip.row = min(currentShip.row, HEIGHT - (currentShip.isHorizontal ? 1 : length));
                currentShip.col = min(currentShip.col, WIDTH - (currentShip.isHorizontal ? length : 1));
                break;

            case L'Q': case 27: 
                exit(0);
                break;
            case '\r':
            {
                int y1 = currentShip.row;
                int x1 = currentShip.col;
                int y2 = y1 + (currentShip.isHorizontal ? 0 : length - 1);
                int x2 = x1 + (currentShip.isHorizontal ? length - 1 : 0);

                if (y1 < 0 || y1 >= HEIGHT || x1 < 0 || x1 >= WIDTH ||
                    y2 < 0 || y2 >= HEIGHT || x2 < 0 || x2 >= WIDTH) {
                    std::cout << "Ошибка: Корабль выходит за пределы поля! Попробуйте снова.\n";
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
                std::cout << symbol;
                break;
            }
        }
    }

    drawBoards(ship_board);
    std::cout << "\nВсе 10 кораблей размещены! Вы закончили расстановку! Нажмите Enter для продолжения...";

    string temp;
    getline(cin, temp);
}