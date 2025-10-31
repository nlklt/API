#define _CRT_SECURE_NO_WARNINGS

#include "/API/ship_war/game.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <sstream>
#include <locale>
//#include <stdexcept>
//#include <cstdint>

#pragma comment(lib, "Ws2_32.lib")

bool send_all(SOCKET s, const char* buf, int total)
{
    int offset = 0;
    while (offset < total)
    {
        int n = send(s, buf + offset, total - offset, 0);
        if (n == SOCKET_ERROR) { return false; }
        offset += n;
    }
    return true;
}

bool recv_all(SOCKET s, char* buf, int len)
{
    int offset = 0;
    while (offset < len)
    {
        int n = recv(s, buf + offset, len - offset, 0);
        if (n == 0) { return false; }
        if (n == SOCKET_ERROR) { return false; }
        offset += n;
    }
    return true;
}

bool send_message(SOCKET s, const std::string& msg)
{
    int len = msg.size();
    int netlen = htonl(len);

    if (!send_all(s, (const char*)&netlen, 4)) { return false; }
    if (len == 0) { return true; }
    return send_all(s, msg.data(), len);
}

bool recv_message(SOCKET s, std::string& out)
{
    uint32_t netlen;
    if (!recv_all(s, reinterpret_cast<char*>(&netlen), 4)) return false;
    uint32_t len = ntohl(netlen);

    out.resize(len);
    if (len == 0) { return true; }
    return recv_all(s, &out[0], len);
}

Board ship_board[2];
Board shots_board[2];
std::mutex boards_mtx;
std::atomic<bool> ready_flag[2];

void handle_client(SOCKET clientSock, int id)
{
    std::cout << " you " << id << " connected\n";
    send_message(clientSock, std::string("START_PLACING"));

    while (true) {
        std::string req;
        if (!recv_message(clientSock, req)) { std::cerr << "player disconnected\n"; return; }
        std::istringstream iss(req);
        std::string cmd; iss >> cmd;
        if (cmd == "PLACE")
        {
            int y1, x1, y2, x2; iss >> y1 >> x1 >> y2 >> x2;

            std::vector<std::pair<int, int>> cells;
            bool ok = false;

            {
                std::lock_guard<std::mutex> lg(boards_mtx);

                if (canPlace(ship_board[id], y1, x1, y2, x2))
                {
                    ShipPlacement sp;
                    sp.length = ((y1 == y2) ? (abs(x2 - x1) + 1) : (abs(y2 - y1) + 1));
                    sp.isHorizontal = (y1 == y2);
                    sp.row = min(y1, y2);
                    sp.col = min(x1, x2);
                    placeFinalShip(ship_board[id], sp);
                    ok = true;
                }
                else { ok = false; }
            }
            send_message(clientSock, ok ? "PLACED" : "INVALIDYOU");
        }
        else if (cmd == "DONE") {
            ready_flag[id].store(true);
            send_message(clientSock, "OK_DONE_PLACING");
            return; // handler done
        }
        else {
            send_message(clientSock, "INVALID");
        }
    }
}

int main()
{

    //устанавливаем локаль для корректного вывода Unicode в Windows/Unix
    setlocale(LC_ALL, "");
    std::locale::global(std::locale(""));

    //Ускоряет вывод
    std::ios::sync_with_stdio(false);
    std::wcout.tie(nullptr);

    //установка кодировки вывода в UTF-8
    SetConsoleOutputCP(CP_UTF8);
    //отключение синхронизации с stdio для скорости
    std::ios::sync_with_stdio(false);
    std::cout.tie(nullptr);

    //включим VT-последовательности (ANSI) в консоли Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    WSADATA w;
    if (WSAStartup(MAKEWORD(2, 2), &w) != 0) { return 1; }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in a{}; a.sin_family = AF_INET; inet_pton(AF_INET, "127.0.0.1", &a.sin_addr); a.sin_port = htons(54000);
    
    bind(listenSock, (sockaddr*)&a, sizeof(a));
    listen(listenSock, SOMAXCONN);
    
    std::cout << "Waiting two players...\n";

    SOCKET clients[2];
    for (int i = 0; i < 2; i++) {
        sockaddr_in ca; int len = sizeof(ca);
        clients[i] = accept(listenSock, (sockaddr*)&ca, &len);
        std::cout << "Player " << (i) << " connected\n";
    }

    for (int i = 0; i < 2; i++) 
    {
        for (int r = 0; r < HEIGHT; r++) 
            for (int c = 0; c < WIDTH; c++) 
            { 
                ship_board[i][r][c] = (int)Cell::Empty; shots_board[i][r][c] = (int)Cell::Empty; 
            }
        ready_flag[i].store(false);
    }

    std::thread t0(handle_client, clients[0], 0);
    std::thread t1(handle_client, clients[1], 1);

    while (!(ready_flag[0].load() && ready_flag[1].load())) Sleep(50);

    send_message(clients[0], "GAME_START");
    send_message(clients[1], "GAME_START");
    std::cout << "Both ready — starting turns\n";

    //// Первая доска
    //Board first_ship_board = { {
    //    {{0,0,0,0,0,0,1,0,0,0}},
    //    {{0,1,0,0,0,0,1,0,0,0}},
    //    {{0,0,0,1,0,0,1,0,0,0}},
    //    {{0,1,0,0,0,0,1,0,0,1}},
    //    {{0,0,0,0,0,0,0,0,0,1}},
    //    {{0,0,0,0,0,0,0,1,0,0}},
    //    {{0,1,1,0,0,0,0,0,0,0}},
    //    {{0,0,0,0,1,0,0,0,1,0}},
    //    {{1,0,0,0,1,0,0,0,1,0}},
    //    {{1,0,0,0,1,0,0,0,1,0}}
    //} };

    //// Вторая доска
    //Board second_ship_board = { {
    //    {{1,1,0,0,0,0,0,0,0,0}},
    //    {{0,0,0,0,1,1,1,1,0,0}},
    //    {{0,1,0,0,0,0,0,0,0,0}},
    //    {{0,0,0,0,0,1,0,0,0,0}},
    //    {{0,0,0,0,0,0,0,1,1,0}},
    //    {{0,1,1,0,0,0,0,0,0,0}},
    //    {{0,0,0,0,0,0,0,1,0,0}},
    //    {{0,1,0,1,0,0,0,1,0,0}},
    //    {{0,1,0,0,0,1,0,1,0,0}},
    //    {{0,1,0,0,0,0,0,0,0,0}}
    //} };

    int turn = 0; // player index who shoots
    bool game_over = false;

    while (!game_over)
    {
        // Уведомим игроков о текущем состоянии хода
        send_message(clients[turn], "YOUR_TURN");
        send_message(clients[1 - turn], "OPPONENT_TURN");

        // Получаем сообщение от игрока, ожидаем "SHOT y x"
        std::string shot_req;
        if (!recv_message(clients[turn], shot_req)) {
            std::cerr << "player disconnected\n";
            break;
        }

        // Парсим команду
        std::istringstream iss(shot_req);
        std::string cmd;
        iss >> cmd;
        if (cmd != "SHOT") {
            send_message(clients[turn], "INVALID");
            continue; // ждём корректной команды
        }

        int y = -1, x = -1;
        if (!(iss >> y >> x)) {
            send_message(clients[turn], "INVALID");
            continue;
        }

        // Проверка границ
        if (y < 0 || y >= HEIGHT || x < 0 || x >= WIDTH) {
            send_message(clients[turn], "INVALID");
            continue;
        }

        // Снимки "до"
        Board opp_before;
        Board shooter_before;
        {
            std::lock_guard<std::mutex> lg(boards_mtx);
            opp_before = ship_board[1 - turn];
            shooter_before = shots_board[turn];
        }

        // Выполнить выстрел (checkAndFire меняет ship_board[opponent] и shots_board[shooter])
        bool repeatShot = false;
        {
            std::lock_guard<std::mutex> lg(boards_mtx);
            FirePlacement fp{ y, x };
            repeatShot = checkAndFire(ship_board[1 - turn], shots_board[turn], fp);
        }

        // Снимки "после"
        Board opp_after;
        Board shooter_after;
        {
            std::lock_guard<std::mutex> lg(boards_mtx);
            opp_after = ship_board[1 - turn];
            shooter_after = shots_board[turn];
        }

        // Проходим по всему полю (используем int, чтобы не было предупреждений)
        std::string shooter_batch;   // для игрока, который стрелял (clients[turn])
        std::string opponent_batch;  // для игрока, чей board изменился (clients[1-turn])

        for (int r = 0; r < HEIGHT; ++r) {
            for (int c = 0; c < WIDTH; ++c) {
                int before_sh = shooter_before[r][c];
                int after_sh = shooter_after[r][c];
                if (before_sh != after_sh) {
                    // формат: "S <TYPE> r c\n"  (S = Shooter view)
                    if (after_sh == (int)Cell::Hit)   shooter_batch += "HIT " + std::to_string(r) + " " + std::to_string(c) + "\n";
                    if (after_sh == (int)Cell::Kill)  shooter_batch += "KILL " + std::to_string(r) + " " + std::to_string(c) + "\n";
                    if (after_sh == (int)Cell::Miss)  shooter_batch += "MISS " + std::to_string(r) + " " + std::to_string(c) + "\n";
                }

                int before_opp = opp_before[r][c];
                int after_opp = opp_after[r][c];
                if (before_opp != after_opp) {
                    // формат: "INCOMING_* r c\n"
                    if (after_opp == (int)Cell::Hit)  opponent_batch += "INCOMING_HIT " + std::to_string(r) + " " + std::to_string(c) + "\n";
                    if (after_opp == (int)Cell::Kill) opponent_batch += "INCOMING_KILL " + std::to_string(r) + " " + std::to_string(c) + "\n";
                    if (after_opp == (int)Cell::Miss) opponent_batch += "INCOMING_MISS " + std::to_string(r) + " " + std::to_string(c) + "\n";
                }
            }
        }

        // Отправляем пачками (если есть что отправить)
        if (!shooter_batch.empty())  send_message(clients[turn], "BATCH\n" + shooter_batch);
        if (!opponent_batch.empty()) send_message(clients[1 - turn], "BATCH\n" + opponent_batch);

        // Проверим победу
        {
            std::lock_guard<std::mutex> lg(boards_mtx);
            int remaining = getCountOfShip(ship_board[1 - turn]);
            if (remaining == 0) {
                send_message(clients[turn], "WIN");
                send_message(clients[1 - turn], "LOSE");
                game_over = true;
                break;
            }
        }

        // Смена хода (если не повторный выстрел)
        if (!repeatShot) turn = 1 - turn;
    }

    // cleanup
    t0.join(); t1.join();
    closesocket(clients[0]); closesocket(clients[1]); closesocket(listenSock);
    WSACleanup();
    return 0;
}