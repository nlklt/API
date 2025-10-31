// client.cpp (исправлено под Board из game.h)
#define _CRT_SECURE_NO_WARNINGS
#include "C:\Users\user\work_space\Laboratory_work\OOP_3SEM\lab_10\API\ship_war\game.h"             // или путь "/API/ship_war/game.h" если у теб€ так
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <locale>
#include <conio.h>

#pragma comment(lib, "Ws2_32.lib")

constexpr uint16_t DEFAULT_PORT = 54000;
const char* DEFAULT_ADDR = "127.0.0.1";

// network helpers
bool send_all(SOCKET s, const char* buf, int total) {
    int sent = 0;
    while (sent < total) {
        int n = send(s, buf + sent, total - sent, 0);
        if (n == SOCKET_ERROR) return false;
        sent += n;
    }
    return true;
}
bool recv_all(SOCKET s, char* buf, int len) {
    int got = 0;
    while (got < len) {
        int n = recv(s, buf + got, len - got, 0);
        if (n <= 0) return false;
        got += n;
    }
    return true;
}
bool send_message(SOCKET s, const std::string& msg) {
    uint32_t len = (uint32_t)msg.size();
    uint32_t netlen = htonl(len);
    if (!send_all(s, reinterpret_cast<const char*>(&netlen), 4)) return false;
    if (len == 0) return true;
    return send_all(s, msg.data(), (int)len);
}
bool recv_message(SOCKET s, std::string& out) {
    uint32_t netlen = 0;
    if (!recv_all(s, reinterpret_cast<char*>(&netlen), 4)) return false;
    uint32_t len = ntohl(netlen);
    out.clear();
    out.resize(len);
    if (len == 0) return true;
    return recv_all(s, &out[0], (int)len);
}

// client-side state (Board Ч как у теб€ в game.h)
Board my_board;       // свои корабли (лева€)
Board shots_board;    // куда стрел€л (права€)

std::mutex boards_mtx;
std::atomic<bool> game_started(false);
std::atomic<bool> your_turn(false);
std::atomic<bool> running(true);

// shot result sync
std::mutex shot_mtx;
std::condition_variable shot_cv;
std::string last_shot_result;
bool last_shot_ready = false;

// ----------------- UTF-8 -> Console helpers -----------------
#include <windows.h>
#include <string>

static std::wstring utf8_to_wstring(const std::string& s) {
    if (s.empty()) return std::wstring();
    int required = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    if (required <= 0) return std::wstring();
    std::wstring w; w.resize(required);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], required);
    return w;
}

static void print_utf8_to_console(const std::string& utf8) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        // fallback
        std::cout << utf8;
        return;
    }
    DWORD mode;
    if (GetConsoleMode(hOut, &mode)) {
        // Console supports wide-char writes
        std::wstring w = utf8_to_wstring(utf8);
        DWORD written = 0;
        WriteConsoleW(hOut, w.c_str(), (DWORD)w.size(), &written, nullptr);
    }
    else {
        // If not a console (redirected), write raw bytes
        DWORD written = 0;
        WriteFile(hOut, utf8.data(), (DWORD)utf8.size(), &written, nullptr);
    }
}
// --------------------------------------------------------------


// ќбработчик одиночного сообщени€ с сервера
void apply_update_line(const std::string& line) {
    // пример: "HIT r c" или "INCOMING_MISS r c" или "KILL r c" и т.д.
    std::istringstream iss(line);
    std::string cmd; iss >> cmd;
    int r, c;
    if (!(iss >> r >> c)) return;
    std::lock_guard<std::mutex> lg(boards_mtx);
    if (cmd == "HIT") { shots_board[r][c] = (int)Cell::Hit; }
    else if (cmd == "KILL") { shots_board[r][c] = (int)Cell::Kill; }
    else if (cmd == "MISS") { shots_board[r][c] = (int)Cell::Miss; }

    else if (cmd == "INCOMING_HIT") { my_board[r][c] = (int)Cell::Hit; }
    else if (cmd == "INCOMING_KILL") { my_board[r][c] = (int)Cell::Kill; }
    else if (cmd == "INCOMING_MISS") { if (my_board[r][c] == (int)Cell::Empty) my_board[r][c] = (int)Cell::Miss; }
}
void handle_server_message(const std::string& msg) {
    if (msg.rfind("BATCH\n", 0) == 0) {
        std::istringstream batch(msg.substr(6));
        std::string line;

        std::string shooter_result;
        std::string best_result;
        int best_r = -1, best_c = -1;

        while (std::getline(batch, line)) {
            if (line.empty()) continue;

            std::istringstream lss(line);
            std::string cmd; int r, c;
            lss >> cmd >> r >> c;

            apply_update_line(line);

            // приоритетный выбор: KILL > HIT > MISS
            int priority = 0;
            if (cmd == "MISS") priority = 1;
            else if (cmd == "HIT") priority = 2;
            else if (cmd == "KILL") priority = 3;

            if (priority > 0 && priority > (best_result == "KILL" ? 3 : best_result == "HIT" ? 2 : best_result == "MISS" ? 1 : 0)) {
                best_result = cmd;
                best_r = r;
                best_c = c;
            }
        }

        // ѕерерисовываем доски
        Board copy_my, copy_shots;
        {
            std::lock_guard<std::mutex> lg(boards_mtx);
            copy_my = my_board;
            copy_shots = shots_board;
        }
        drawBoards(copy_my, copy_shots);

        // ”ведомл€ем makeShotNetwork
        if (!best_result.empty()) {
            std::lock_guard<std::mutex> lk(shot_mtx);
            last_shot_result = best_result + " " + std::to_string(best_r) + " " + std::to_string(best_c);
            last_shot_ready = true;
            shot_cv.notify_one();
        }
        return;
    }

    // остальна€ обработка (YOUR_TURN, HIT, MISS, KILL, INCOMING_*, WIN/LOSE и т.д.)
    std::istringstream iss(msg);
    std::string cmd; iss >> cmd;

    if (cmd == "YOUR_TURN") {
        your_turn.store(true);
    }
    else if (cmd == "OPPONENT_TURN") {
        your_turn.store(false);
    }
    else if (cmd == "GAME_START") {
        game_started.store(true);
    }
    else if (cmd == "WIN" || cmd == "LOSE") {
        // покажем финал и остановим run-loop
        Board copy_my, copy_shots;
        {
            std::lock_guard<std::mutex> lg(boards_mtx);
            copy_my = my_board;
            copy_shots = shots_board;
        }
        drawBoards(copy_my, copy_shots);
        print_utf8_to_console(std::string("\n*** ") + cmd + " ***\n");
        running.store(false);
        shot_cv.notify_one();
    }
    else {
        // одиночные обновлени€ (удобно держать дл€ обратной совместимости)
        int r, c;
        if (cmd == "HIT" || cmd == "KILL" || cmd == "MISS") {
            iss >> r >> c;
            {
                std::lock_guard<std::mutex> lg(boards_mtx);
                if (cmd == "HIT") shots_board[r][c] = (int)Cell::Hit;
                if (cmd == "KILL") shots_board[r][c] = (int)Cell::Kill;
                if (cmd == "MISS") shots_board[r][c] = (int)Cell::Miss;
            }

            // уведомим ожидание выстрела
            {
                std::lock_guard<std::mutex> lk(shot_mtx);
                last_shot_result = cmd + std::string(" ") + std::to_string(r) + " " + std::to_string(c);
                last_shot_ready = true;
                shot_cv.notify_one();
            }
        }
        else if (cmd == "INCOMING_HIT" || cmd == "INCOMING_KILL" || cmd == "INCOMING_MISS") {
            iss >> r >> c;
            std::lock_guard<std::mutex> lg(boards_mtx);
            if (cmd == "INCOMING_HIT") my_board[r][c] = (int)Cell::Hit;
            if (cmd == "INCOMING_KILL") my_board[r][c] = (int)Cell::Kill;
            if (cmd == "INCOMING_MISS") if (my_board[r][c] == (int)Cell::Empty) my_board[r][c] = (int)Cell::Miss;
        }

        // перерисуем компактно
        Board copy_my, copy_shots;
        {
            std::lock_guard<std::mutex> lg(boards_mtx);
            copy_my = my_board;
            copy_shots = shots_board;
        }
        drawBoards(copy_my, copy_shots);
    }
}


// поток приЄма сообщений от сервера
void receiver_thread(SOCKET sock) {
    std::string msg;
    while (running.load()) {
        if (!recv_message(sock, msg)) {
            std::cerr << "Disconnected from server\n";
            running.store(false);
            shot_cv.notify_one();
            break;
        }
        handle_server_message(msg);
        // перерисовать доску
        {
            std::lock_guard<std::mutex> lg(boards_mtx);
            drawBoards(my_board, shots_board);
        }
    }
}

// блокирующее ожидание одного текстового ответа (используетс€ в фазе placement)
std::string wait_reply_blocking(SOCKET sock) {
    std::string reply;
    if (!recv_message(sock, reply)) return std::string("DISCONNECT");
    return reply;
}

// сетевое размещение (вместо локального placeShip)
void placeShipNetwork(SOCKET sock) {
    const std::unordered_map<int, int> SHIP_LIMITS = { {4,1},{3,2},{2,3},{1,4} };
    std::vector<int> shipLengths = { 4,3,3,2,2,2,1,1,1,1 };
    SetConsoleCP(CP_UTF8);

    std::unordered_map<int, int> placedCounts = { {4,0},{3,0},{2,0},{1,0} };

    for (int length : shipLengths) {
        if (placedCounts[length] >= SHIP_LIMITS.at(length)) continue;

        ShipPlacement currentShip = { HEIGHT / 2, WIDTH / 2 - length / 2, length, true };
        bool isPlaced = false;

        while (!isPlaced && running.load()) {
            Board temp_board;
            {
                std::lock_guard<std::mutex> lg(boards_mtx);
                temp_board = my_board; // Board supports copy
            }
            markCurrentShip(temp_board, my_board, currentShip);
            Board xeroxzero = {};
            drawBoards(temp_board, xeroxzero);

            std::ostringstream prompt;
            prompt << "–азместите корабль длиной " << length << " (" << (placedCounts[length] + 1) << " из " << SHIP_LIMITS.at(length) << "): ";
            std::cout << prompt.str();

            wchar_t symbol = _getch();

            int maxRow = HEIGHT - (currentShip.isHorizontal ? 1 : length);
            int maxCol = WIDTH - (currentShip.isHorizontal ? length : 1);

            switch (toupper(symbol)) {
            case L'W': case 214: case 72: currentShip.row = max(0, currentShip.row - 1); break;
            case L'A': case 212: case 75: currentShip.col = max(0, currentShip.col - 1); break;
            case L'S': case 219: case 80: currentShip.row = min(maxRow, currentShip.row + 1); break;
            case L'D': case 194: case 77: currentShip.col = min(maxCol, currentShip.col + 1); break;
            case L'R':
                currentShip.isHorizontal = !currentShip.isHorizontal;
                currentShip.row = min(currentShip.row, HEIGHT - (currentShip.isHorizontal ? 1 : currentShip.length));
                currentShip.col = min(currentShip.col, WIDTH - (currentShip.isHorizontal ? currentShip.length : 1));
                break;
            case '\r': {
                int y1 = currentShip.row;
                int x1 = currentShip.col;
                int y2 = y1 + (currentShip.isHorizontal ? 0 : length - 1);
                int x2 = x1 + (currentShip.isHorizontal ? length - 1 : 0);

                // send PLACE
                std::ostringstream ss;
                ss << "PLACE " << y1 << ' ' << x1 << ' ' << y2 << ' ' << x2;
                if (!send_message(sock, ss.str())) { std::cerr << "send failed\n"; running.store(false); return; }

                // wait reply (blocking) Ч receiver thread ещЄ не запущ
                std::string reply = wait_reply_blocking(sock);
                if (reply == "PLACED") {
                    ShipPlacement sp;
                    sp.length = length;
                    sp.isHorizontal = currentShip.isHorizontal;
                    sp.row = min(y1, y2);
                    sp.col = min(x1, x2);
                    {
                        std::lock_guard<std::mutex> lg(boards_mtx);
                        placeFinalShip(my_board, sp);
                    }
                    placedCounts[length]++;
                    isPlaced = true;
                }
                else {
                    std::cout << "\nServer: " << reply << "\n";
                    std::this_thread::sleep_for(std::chrono::milliseconds(600));
                }
                break;
            }
            case L'Q': case 27:
                running.store(false);
                send_message(sock, "QUIT");
                return;
            default:
                break;
            }
        }
    }

    // finished local placement -> tell server
    if (!send_message(sock, "DONE")) { std::cerr << "send DONE failed\n"; running.store(false); return; }
    std::string r = wait_reply_blocking(sock); // OK_DONE_PLACING
    if (r != "OK_DONE_PLACING") {
        // возможно сервер сразу прислал GAME_START; handle it
        if (r == "GAME_START") game_started.store(true);
    }
    else {
        // wait possibly next msg GAME_START
        std::string g;
        if (recv_message(sock, g)) {
            if (g == "GAME_START") game_started.store(true);
        }
    }
}

// сетевой выстрел (использует condition_variable дл€ результата)
void makeShotNetwork(SOCKET sock) {
    FirePlacement currentFire = { HEIGHT / 2, WIDTH / 2 };

    while (running.load()) {
        // ждЄм флага your_turn
        while (!your_turn.load() && running.load()) std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (!running.load()) return;

        while (your_turn.load() && running.load()) {
            {
                std::lock_guard<std::mutex> lg(boards_mtx);
                Board temp = shots_board;
                if (currentFire.row >= 0 && currentFire.row < HEIGHT && currentFire.col >= 0 && currentFire.col < WIDTH)
                    temp[currentFire.row][currentFire.col] = (int)Cell::Cursor;
                drawBoards(my_board, temp);
            }

            std::cout << "\n“вой ход! WASD - двигайс€, ENTER - стрел€ть\n";
            wchar_t symbol = _getch();
            int maxRow = HEIGHT - 1;
            int maxCol = WIDTH - 1;

            switch (toupper(symbol)) {
            case L'W': case 214: case 72: currentFire.row = max(0, currentFire.row - 1); break;
            case L'A': case 212: case 75: currentFire.col = max(0, currentFire.col - 1); break;
            case L'S': case 219: case 80: currentFire.row = min(maxRow, currentFire.row + 1); break;
            case L'D': case 194: case 77: currentFire.col = min(maxCol, currentFire.col + 1); break;
            case '\r': {
                std::ostringstream ss; ss << "SHOT " << currentFire.row << ' ' << currentFire.col;
                if (!send_message(sock, ss.str())) { std::cerr << "send SHOT failed\n"; running.store(false); return; }

                // ждЄм ответ, который выдаст receiver_thread через condition_variable
                std::unique_lock<std::mutex> lk(shot_mtx);
                shot_cv.wait(lk, [] { return last_shot_ready || !running.load(); });
                if (!running.load()) return;
                std::string result = last_shot_result;
                last_shot_ready = false;
                lk.unlock();

                std::cout << "\nServer: " << result << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(400));

                // если промах Ч завершаем ход (сервер должен прислать YOUR_TURN/OPPONENT_TURN контрольный пакет)
                if (result.rfind("MISS", 0) == 0 || result.rfind("ALREADY", 0) == 0) {
                    your_turn.store(false);
                    break;
                }
                // при HIT/KILL предполагаем, что сервер пришлЄт YOUR_TURN снова дл€ продолжени€ или не придЄт -> rely на YOUR_TURN флаг
                break;
            }
            case L'Q': case 27:
                running.store(false);
                send_message(sock, "QUIT");
                return;
            default:
                break;
            }
        }
    }
}

int main(int argc, char* argv[]) {


    //устанавливаем локаль дл€ корректного вывода Unicode в Windows/Unix
    setlocale(LC_ALL, "");
    std::locale::global(std::locale(""));

    //”скор€ет вывод
    std::ios::sync_with_stdio(false);
    std::wcout.tie(nullptr);

    //установка кодировки вывода в UTF-8
    SetConsoleOutputCP(CP_UTF8);
    //отключение синхронизации с stdio дл€ скорости
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

    const char* addr = DEFAULT_ADDR;
    uint16_t port = DEFAULT_PORT;
    if (argc >= 2) addr = argv[1];
    if (argc >= 3) port = (uint16_t)std::stoi(argv[2]);

    WSADATA w;
    if (WSAStartup(MAKEWORD(2, 2), &w) != 0) { std::cerr << "WSAStartup failed\n"; return 1; }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) { std::cerr << "socket failed\n"; WSACleanup(); return 1; }

    sockaddr_in svr{}; svr.sin_family = AF_INET; inet_pton(AF_INET, addr, &svr.sin_addr); svr.sin_port = htons(port);
    if (connect(sock, reinterpret_cast<sockaddr*>(&svr), sizeof(svr)) == SOCKET_ERROR) {
        std::cerr << "connect failed, err=" << WSAGetLastError() << "\n"; closesocket(sock); WSACleanup(); return 1;
    }

    // »нициализаци€ Board Ч делаем €вную инициализацию по элементам (Board Ч фиксированна€ структура)
    {
        std::lock_guard<std::mutex> lg(boards_mtx);
        for (int r = 0; r < HEIGHT; ++r) for (int c = 0; c < WIDTH; ++c) {
            my_board[r][c] = (int)Cell::Empty;
            shots_board[r][c] = (int)Cell::Empty;
        }
    }

    // ожидаем START_PLACING
    std::string msg;
    if (!recv_message(sock, msg)) { std::cerr << "failed initial recv\n"; closesocket(sock); WSACleanup(); return 1; }
    if (msg != "START_PLACING") {
        std::cerr << "Unexpected initial message: " << msg << "\n";
    }
    else {
        std::cout << "Server: START_PLACING\n";
    }

    // размещаемс€, в процессе placeShipNetwork использую blocking recv дл€ ответов PLACED/INVALIDYOU
    placeShipNetwork(sock);

    // теперь игра стартует Ч запускаем приЄмный поток
    std::thread recvThr(receiver_thread, sock);

    // запускаем сетевой режим выстрела (блокирует пока игра идЄт)
    makeShotNetwork(sock);

    // cleanup
    running.store(false);
    if (recvThr.joinable()) recvThr.join();
    closesocket(sock);
    WSACleanup();
    return 0;
}
