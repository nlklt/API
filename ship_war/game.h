#pragma once

const int WIDTH = 10;
const int HEIGHT = 10;

enum { A = 0, B = 1, C = 2, D = 3, E = 4, F = 5, G = 6, H = 7, I = 8, J = 9 };

void drawBoards(const int ($ship_boards)[HEIGHT][WIDTH], const int(&shots_board)[HEIGHT][WIDTH]);
void placeShip(int ship_board[HEIGHT][WIDTH]);
bool isPlace(int ship_board[HEIGHT][WIDTH], int x, int y);
