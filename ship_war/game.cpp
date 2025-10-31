#include "game.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <windows.h>
#include <conio.h>

using namespace std;

//определения внешних переменных
string header_utf8;
string sep_utf8;
string endsep_utf8;
bool precomputed = false;

//Вспомогательное: безопасно получить строку по Cell
static const std::string cellFor(int symbol, bool showShips)
{
    Cell c = static_cast<Cell>(symbol);
    switch (c)
    {
    case Cell::Ship:  return showShips ? u8"██\x1b[0m" : u8"  \x1b[0m";
    //case Cell::Hit:   return u8"✖ \x1b[0m";
    case Cell::Hit:   return u8"\x1b[33m██\x1b[0m";
    //case Cell::Miss:  return u8"• \x1b[0m";
    case Cell::Miss:  return u8"\x1b[46m  \x1b[0m";
    //case Cell::Kill:  return u8"\x1b[91m☠ \x1b[0m";
    case Cell::Kill:  return u8"\x1b[91m██\x1b[0m";
    case Cell::Empty: return u8"  \x1b[0m";
    case Cell::Cursor:return u8"[]\x1b[0m";
    default:          return u8"??\x1b[0m";
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

static Board zero_zero = {};

void drawBoards(const Board& ship_board, const Board& shots_board = zero_zero)
{
    if (!precomputed) precomputeLayout();

    std::string out;
    int approx_per_line = 3 + (WIDTH * 3) + 1 + (WIDTH * 3) + 2;
    out.reserve((HEIGHT * 2 + 6) * approx_per_line);

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
        //fallback
        std::cout << "FuCk!\n" << out;
        std::cout.flush();
    }
}

bool canPlace(Board& ship_board, int y1, int x1, int y2, int x2) {
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
            if (y >= 0 && y < HEIGHT && x >= 0 && x < WIDTH)
            {
                if (ship_board[y][x] == (int)Cell::Ship)
                {
                    std::cout << "Между кораблями должна быть пустая клетка!" << endl;
                    return false;
                }
            }
        }
    }
    return true;
}

void placeFinalShip(Board& ship_board, const ShipPlacement& currentShip)
{
    for (int k = 0; k < currentShip.length; ++k) 
    {
        int r = currentShip.row + (currentShip.isHorizontal ? 0 : k);
        int c = currentShip.col + (currentShip.isHorizontal ? k : 0);
    
        if (r >= 0 && r < HEIGHT && c >= 0 && c < WIDTH) 
        {
            ship_board[r][c] = (int)Cell::Ship;
        }
    }
}

void markCurrentShip(Board& temp_board, const Board& ship_board, const ShipPlacement& currentShip)
{
    temp_board = ship_board;

    if (currentShip.length <= 0) { return; }

    for (int k = 0; k < currentShip.length; ++k) {
        int r = currentShip.row + (currentShip.isHorizontal ? 0 : k);
        int c = currentShip.col + (currentShip.isHorizontal ? k : 0);

        if (r >= 0 && r < HEIGHT && c >= 0 && c < WIDTH) 
        {
            if (temp_board[r][c] != (int)Cell::Ship) 
            {
                temp_board[r][c] = (int)Cell::Cursor;
            }
        }
    }
}

void placeShip(Board& ship_board)
{
    //ограничение по типам кораблей и порядку размещения
    const std::unordered_map<int, int> SHIP_LIMITS = {
    {4, 1}, {3, 2}, {2, 3}, {1, 4}
    };

    std::vector<int> shipLengths = { 4, 3, 3, 2, 2, 2, 1, 1, 1, 1 };

    std::cout << "WASD: Перемещение, R: Поворот, ENTER: Разместить, Q: Выход\n";

    //SetConsoleOutputCP(65001);
    SetConsoleCP(CP_UTF8);

    std::unordered_map<int, int> placedCounts = { {4, 0}, {3, 0}, {2, 0}, {1, 0} };
    //int totalShipsPlaced = 0;

    for (int length : shipLengths)
    {
        if (placedCounts[length] >= SHIP_LIMITS.at(length)) { continue; }

        ShipPlacement currentShip = { HEIGHT / 2, WIDTH / 2 - length / 2, length, true };
        bool isPlaced = false;

        while (!isPlaced)
        {
            Board temp_board = ship_board;
            markCurrentShip(temp_board, ship_board, currentShip);
            drawBoards(temp_board);

            int val1 = placedCounts[length] + 1;
            int val2 = SHIP_LIMITS.at(length);

            string out = "";
            out += "Разместите корабль длиной " + length;
            out += " (" + val1;
            out += " из " + val2;
            out += "): ";

            std::cout << out;

            wchar_t symbol = _getch();

            int maxRow = HEIGHT - (currentShip.isHorizontal ? 1 : length);
            int maxCol = WIDTH - (currentShip.isHorizontal ? length : 1);

            //int prevRow = currentShip.row;
            //int prevCol = currentShip.col;
            //bool prevIsHorizontal = currentShip.isHorizontal;

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
                currentShip.row = min(currentShip.row, HEIGHT - (currentShip.isHorizontal ? 1 : currentShip.length));
                currentShip.col = min(currentShip.col, WIDTH - (currentShip.isHorizontal ? currentShip.length : 1));
                break;
            case '\r':
            {
                int y1 = currentShip.row;
                int x1 = currentShip.col;
                int y2 = y1 + (currentShip.isHorizontal ? 0 : length - 1);
                int x2 = x1 + (currentShip.isHorizontal ? length - 1 : 0);

                if (canPlace(ship_board, y1, x1, y2, x2))
                {
                    placeFinalShip(ship_board, currentShip);
                    placedCounts[length]++;
                    isPlaced = true;
                }
                break;
            }
            case L'Q': case 27:
                exit(0);
                break;
            default:
                //тотали игнорик ~
                break;
            }
        }
    }

    drawBoards(ship_board);
    std::cout << "\nКорабли размещены! Нажмите Enter...\n";

    string temp;
    getline(std::cin, temp);
}

bool checkAndFire(Board& ship_board, Board& shots_board, FirePlacement currentFire)
{
    int r = currentFire.row;
    int c = currentFire.col;

    if (r < 0 || r >= HEIGHT || c < 0 || c >= WIDTH) { return false; }

    int cur = shots_board[r][c];

    if (cur == (int)Cell::Miss || cur == (int)Cell::Hit || cur == (int)Cell::Kill) {
        // Уже стреляли сюда
        std::cout << "Сюда уже стреляли!\n";
        return false; // считать как промах/нет нового хода
    }

    if (ship_board[r][c] == (int)Cell::Ship) 
    {
        shots_board[r][c] = (int)Cell::Hit;
        ship_board[r][c] = (int)Cell::Hit;

        int left = c, right = c, up = r, down = r;
        // горизонтально
        int cc = c - 1;
        while (cc >= 0 && ship_board[r][cc] == (int)Cell::Ship ||
            cc >= 0 && ship_board[r][cc] == (int)Cell::Hit) { left = cc; --cc; }
        cc = c + 1;
        while (cc < WIDTH && ship_board[r][cc] == (int)Cell::Ship ||
            cc < WIDTH && ship_board[r][cc] == (int)Cell::Hit) { right = cc; ++cc; }
        // вертикально
        int rr = r - 1;
        while (rr >= 0 && ship_board[rr][c] == (int)Cell::Ship ||
            rr >= 0 && ship_board[rr][c] == (int)Cell::Hit) { up = rr; --rr; }
        rr = r + 1;
        while (rr < HEIGHT && ship_board[rr][c] == (int)Cell::Ship ||
            rr < HEIGHT && ship_board[rr][c] == (int)Cell::Hit) { down = rr; ++rr; }

        bool horizontal = (right - left) >= (down - up);

        std::vector<std::pair<int, int>> parts;

        if (horizontal) 
        {
            for (int x = left; x <= right; ++x) { parts.emplace_back(r, x); }
        }
        else
        {
            for (int y = up; y <= down; ++y) { parts.emplace_back(y, c); }
        }

        bool anyAlive = false;
        for (pair<int, int> yrxc : parts) {
            // alive если на ship_board есть Ship и shots_board ещё не Hit/Kill
            if (ship_board[yrxc.first][yrxc.second]  == (int)Cell::Ship &&
                shots_board[yrxc.first][yrxc.second] != (int)Cell::Hit  &&
                shots_board[yrxc.first][yrxc.second] != (int)Cell::Kill)
            {
                anyAlive = true;
                break;
            }
        }
        if (!anyAlive) {
            // Потоплен — пометим все части как Kill
            for (pair<int, int> yrxc : parts) {
                shots_board[yrxc.first][yrxc.second] = (int)Cell::Kill;
                ship_board[yrxc.first][yrxc.second] = (int)Cell::Kill;
            }

            // И все клетки вокруг каждой части корабля пометим как Miss (если там нет другого корабля и не помечено)
            for (pair<int, int> yrxc : parts) {
                int yr = yrxc.first;
                int xc = yrxc.second;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int ny = yr + dy;
                        int nx = xc + dx;
                        if (ny < 0 || ny >= HEIGHT || nx < 0 || nx >= WIDTH) { continue; }
                        //не переписываем части корабля
                        if (ship_board[ny][nx] == (int)Cell::Ship) { continue; }
                        //не трогаем уже помеченные Hit/Kill
                        if (shots_board[ny][nx] == (int)Cell::Empty) 
                        {
                            shots_board[ny][nx] = (int)Cell::Miss;
                        }
                    }
                }
            }

            std::cout << "Корабль потоплен!\n";
        }
        else {
            std::cout << "Попадание!\n";
        }

        return true; // попал -> игрок стреляет снова
    }
    else {
        // Промах
        shots_board[r][c] = (int)Cell::Miss;
        std::cout << "Мимо.\n";
        return false;
    }
}

void makeShot(Board& my_board, Board& ship_board, Board& shots_board)
{
    FirePlacement currentFire = { HEIGHT / 2, WIDTH / 2 };

    bool shotResult = false;
    while (true)
    {
        Board temp_board = shots_board;

        if (currentFire.row >= 0 && currentFire.row < HEIGHT && currentFire.col >= 0 && currentFire.col < WIDTH)
        {
            temp_board[currentFire.row][currentFire.col] = (int)Cell::Cursor;
        }

        drawBoards(my_board, temp_board);

        int win = getCountOfShip(ship_board);
        
        if (win == 0) { std::cout << "\nПОЗДРАВЛЯЮ!"; system("pause"); }

        std::cout << "\nНАВОДИСЬ МАТЬ ТВОЮ!";

        wchar_t symbol = _getch();

        int maxRow = HEIGHT - 1;
        int maxCol = WIDTH - 1;

        switch (toupper(symbol))
        {
        case L'W': case 214: case 72:
            currentFire.row = max(0, currentFire.row - 1);
            break;
        case L'A': case 212: case 75:
            currentFire.col = max(0, currentFire.col - 1);
            break;
        case L'S': case 219: case 80:
            currentFire.row = min(maxRow, currentFire.row + 1);
            break;
        case L'D': case 194: case 77:
            currentFire.col = min(maxCol, currentFire.col + 1);
            break;
        case '\r':
            shotResult = checkAndFire(ship_board, shots_board, currentFire);

            if (!shotResult) { return; }
            
            break;
        case L'Q': case 27:
            exit(0);
            break;
        }
    }
}

int getCountOfShip(const Board& b)
{
    int count = 0;
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (b[i][j] == (int)Cell::Ship)
            {
                count++;
            }
        }
    }
    return count;
}
