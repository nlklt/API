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
        wcout << L"Координаты вне поля!\n";
        return false;
    }

    int minY = min(y1, y2);
    int minX = min(x1, x2);

    int maxY = max(y1, y2);
    int maxX = max(x1, x2);

    for (int y = minY - 1; y < maxY + 1; ++y)
    {
        for (int x = minX - 1; x < maxX + 1; ++x)
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

std::unordered_map<std::string, int> getCountOfShip(const int (& ships_of_type)[HEIGHT][WIDTH])
{
    int checked_field[HEIGHT][WIDTH] = {};
    unordered_map<string, int> length = {
        {"One", 0},
        {"Two", 0},
        {"Three", 0},
        {"Four", 0}
    };
    
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            checked_field[i][j] = ships_of_type[i][j];
        }
    }

    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (checked_field[i][j] == 1)
            {
                checked_field[i][j] = 0;
                int indexer_i = i, indexer_j = j;
                int ship_length = 1;
                while (checked_field[indexer_i + 1][indexer_j] == 1 || checked_field[indexer_i][indexer_j + 1] == 1)
                {
                    checked_field[indexer_i][indexer_i] = 0;
                    checked_field[indexer_i][indexer_j + 1] == 1 ? indexer_j++ : indexer_i++;
                    ship_length++;
                }
                switch (ship_length)
                {
                case 1:
                    length["One"]++;
                    break;
                case 2:
                    length["Two"]++;
                    break;
                case 3:
                    length["Three"]++;
                    break;
                case 4:
                    length["Four"]++;
                    break;
                default:
                    wcout << L"Ошибка ввода";
                    break;
                }
            }
        }
    }
    return length;
}

void placeShipd(int(&ship_board)[HEIGHT][WIDTH])
{
    pair<int, int>  ships_of_type[4] = { {1, 4}, {2, 3}, {3, 2}, {4, 1} };

    wcout << L"WASD для перемещения";
    getCountOfShip(ship_board);
    int index_i = 0, index_j = 0;
    while (true)
    {
        char symbol;
        symbol = _getche();
        switch (symbol)
        {
        case 'W':
            ship_board[index_i][index_j] = 0;
            index_i--;
            break;
        case 'A':
            ship_board[index_i][index_j] = 0;
            index_j--;
            break;
        case 'S':
            ship_board[index_i][index_j] = 0;
            index_i++;
            break;
        case 'D':
            ship_board[index_i][index_j] = 0;
            index_j++;
            break;
        case '\r':
            ship_board[index_i][index_j] = 1;
            index_i++;
            break;
        default:
            wcout << L"Ошибка ввода";
            break;
        }
        ship_board[index_i][index_j] = 9;
        drawBoards(ship_board);
    }
}

void placeShip(int(&ship_board)[HEIGHT][WIDTH])
{

    pair<int, int>  ships_of_type[4] = { {1, 4}, {2, 3}, {3, 2}, {4, 1} };

    wcout << L"Размещение: введите координату начала (буква + число) и направление (W/A/S/D). Примеры: (A3D), (b10 w)";

        for (int type = 0; type < 4; ++type)
        {
            int count = ships_of_type[type].first;
            int length = ships_of_type[type].second;

            for (int idx = 0; idx < count; idx++)
            {
                while (true) {
                    //запрос ввода
                    wcout << L"\nРазместите корабль длиной " << length << L" (" << (idx + 1) << L"/" << count << L"): ";
                    string input;
                    getline(cin, input);

                    //удаляем пробелы в начале/конце и затем все пробелы для упрощения парсинга
                    input.erase(remove_if(input.begin(), input.end(), ::isspace), input.end());

                    if (input.size() < 2) {
                        wcout << L"Неверный формат ввода! Пример: A3D\n";
                        continue;
                    }

                    //буква - первый символ
                    char letter = input[0];
                    if (!isalpha(letter)) {
                        wcout << L"Первая позиция должна быть буквой (A-J)\n";
                        continue;
                    }
                    int x1 = toupper(letter) - 'A';

                    //читаем цифры, начиная с позиции 1
                    int pos = 1;
                    int number = 0;
                    bool foundDigit = false;
                    while (pos < (int)input.size() && isdigit(input[pos])) {
                        foundDigit = true;
                        number = number * 10 + (input[pos] - '0');
                        ++pos;
                    }
                    if (!foundDigit) {
                        wcout << L"После буквы должно идти число (1-10)\n";
                        continue;
                    }
                    int y1 = number - 1; // перевод в 0-индекс

                    //направление: если есть символ после числа - берем его, иначе по умолчанию вправо
                    char dir = 'R';
                    if (pos < (int)input.size()) {
                        dir = input[pos];
                        dir = toupper(dir);
                    }

                    //определяем направления
                    int dy = 0, dx = 0;
                    if (dir == 'W')      { dy = -1; dx = 0; } //вверх
                    else if (dir == 'S') { dy = +1; dx = 0; } //вниз
                    else if (dir == 'A') { dy = 0; dx = -1; } //влево
                    else if (dir == 'D') { dy = 0; dx = +1; } //вправо
                    else { dy = 0; dx = +1; }

                    //координаты конца корабля
                    int y2 = y1 + dy * (length - 1);
                    int x2 = x1 + dx * (length - 1);

                    //проверка границ конца и начала
                    if (y1 < 0 || y1 >= HEIGHT || x1 < 0 || x1 >= WIDTH ||
                        y2 < 0 || y2 >= HEIGHT || x2 < 0 || x2 >= WIDTH) 
                    {
                        wcout << L"Корабль выходит за пределы поля. Попробуйте снова.\n";
                        continue;
                    }

                    //проверка на возможность поставить (включая окружение)
                    if (!canPlace(ship_board, y1, x1, y2, x2)) 
                    {
                        continue;
                    }

                    //ставим корабль
                    for (int k = 0; k < length; ++k) 
                    {
                        int yy = y1 + dy * k;
                        int xx = x1 + dx * k;
                        ship_board[yy][xx] = 1;
                    }

                    drawBoards(ship_board);
                    wcout << L"Корабль успешно размещён.\n";
                    break; // выходим из цикла ввода для этого корабля
                } // конец while(true) ввода данного корабля
            } // конец всех кораблей данного типа
        } // конец по типам
    wcout << L"\nВсе корабли размещены! Вы закончили расстановку!\n";
}

bool isShipCellAround(const int(&ship_board)[HEIGHT][WIDTH], int y, int x)
{
    if (
        ship_board[y - 1][x - 1] == 1 || ship_board[y - 1][x] == 1 || ship_board[y - 1][x + 1] == 1 ||
        ship_board[y + 1][x - 1] == 1 || ship_board[y + 1][x] == 1 || ship_board[y + 1][x + 1] == 1 ||
        ship_board[y][x - 1] == 1 || ship_board[y][x] == 1 || ship_board[y][x + 1] == 1
        )
    {
        return true;
    }
    return false;
}

bool makeShot(int(&shots_board)[HEIGHT][WIDTH], const int(&ship_board)[HEIGHT][WIDTH])
{
    bool fire = false;
    while (!fire)
    {
        wcout << L"Введите координаты для выстрела поля!\n";

        string input;
        getline(cin, input);

        //удаляем пробелы в начале/конце и затем все пробелы для упрощения парсинга
        input.erase(remove_if(input.begin(), input.end(), ::isspace), input.end());

        if (input.size() < 2) {
            wcout << L"Неверный формат ввода! Пример: A3D\n";
            continue;
        }

        //буква - первый символ
        char letter = input[0];
        if (!isalpha(letter)) {
            wcout << L"Первая позиция должна быть буквой (A-J)\n";
            continue;
        }
        int x1 = toupper(letter) - 'A';

        //читаем цифры, начиная с позиции 1
        int pos = 1;
        int number = 0;
        bool foundDigit = false;
        while (pos < (int)input.size() && isdigit(input[pos])) {
            foundDigit = true;
            number = number * 10 + (input[pos] - '0');
            ++pos;
        }
        if (!foundDigit) {
            wcout << L"После буквы должно идти число (1-10)\n";
            continue;
        }
        int y1 = number - 1;


        if (y1 < 0 || y1 >= HEIGHT || x1 < 0 || x1 >= WIDTH)
        {
            wcout << L"Координаты вне поля!\n";
            continue;
        }


        if (ship_board[y1][x1] == 1)
        {
            if (isShipCellAround(ship_board, y1, x1))
            {
                shots_board[y1][x1] = 2;
            }
            else
            {
                shots_board[y1][x1] = 3;
            }
            return true;
        }
        else
        {
            shots_board[y1][x1] = 4;
            return false;
        }
    }
    return true;
}
