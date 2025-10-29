#include "game.h"

#include <windows.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <locale>
#include <array>
#include <thread>

using namespace std;

//Возврат символа для отображения ячейки
static wstring cellDisplay(int symbol, bool showShips)
{
    Cell c = static_cast<Cell>(symbol);
    switch (c) {
    case Cell::Ship:  return showShips ? L"██" : L"  ";
    case Cell::Hit:   return L"\033[43m  \033[0m";//L"✖ ";
    case Cell::Miss:  return L"\033[47m  \033[0m"; //L"• ";
    case Cell::Kill:  return L"\033[40m██\033[0m";//L"☠ ";
    case Cell::Empty: return L"  ";//L"  ";
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
    wcout << L"\x1b[2J\x1b[H";

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
    wcout << L"\n✅ Все корабли размещены! Вы закончили расстановку!\n";
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
