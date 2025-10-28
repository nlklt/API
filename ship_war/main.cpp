#include <iostream>
#include <corecrt_io.h>
#include <fcntl.h>

int ship_board[11][11], shots_board[11][11];

int main()
{
	_setmode(_fileno(stdout), _O_U16TEXT);

	return 0;
}