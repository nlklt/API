#include <windows.h>
#include <iostream>
#include <cstdio>
#include <string>

// ����������� ������� ������ ��� �������
#define EVENT_BUFFER_SIZE 128

// �������� ������� ��� ��������� ������� ����
void MouseEventProc(MOUSE_EVENT_RECORD mer);

int main() {
    setlocale(LC_ALL, "rus");
    // �������� ����� ������������ ����� (�������)
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE) {
        std::cerr << "GetStdHandle Error: " << GetLastError() << std::endl;
        return 1;
    }

    // ��������� ������� ����� �������, ����� ����� ��� ������������
    DWORD fdwSaveOldMode;
    if (!GetConsoleMode(hStdin, &fdwSaveOldMode)) {
        std::cerr << "GetConsoleMode Error: " << GetLastError() << std::endl;
        return 1;
    }

    // --- ��������� ������ ������� ---
    // �������� ������� ���� � ����.
    // ENABLE_QUICK_EDIT_MODE ������ ����������� ��� ��������� ENABLE_MOUSE_INPUT,
    // ����� ���������� �������� �������, � �� Windows ��� ��������� ������.
    DWORD fdwMode = ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT;

    if (!SetConsoleMode(hStdin, fdwMode)) {
        std::cerr << "SetConsoleMode Error: " << GetLastError() << std::endl;
        // ����������, �� ��� ��������� ������� ����, ���� SetConsoleMode �� ��������
    }

    // --- ���������� � ������ ������� ---
    INPUT_RECORD irInBuf[EVENT_BUFFER_SIZE];
    DWORD cNumRead;

    // ������� ����������
    std::cout << "--- Mouse Tracker CLI ---" << std::endl;
    std::cout << "����������� ���� ��� �������� ��� � ���� �������." << std::endl;
    std::cout << "������� 'Q' ��� 'Esc' ��� ������." << std::endl;

    // --- ������� ���� ��������� ������� ---
    while (true) {
        // ������� ������� � ������ ����������� �����
        if (!ReadConsoleInput(
            hStdin,      // ����� �����
            irInBuf,     // ����� ��� ������
            EVENT_BUFFER_SIZE, // ������ ������
            &cNumRead))  // ���������� ����������� �������
        {
            std::cerr << "ReadConsoleInput Error: " << GetLastError() << std::endl;
            break;
        }

        // ������������ ��� ���������� �������
        for (DWORD i = 0; i < cNumRead; i++) {
            switch (irInBuf[i].EventType) {
            case MOUSE_EVENT:
                MouseEventProc(irInBuf[i].Event.MouseEvent);
                break;

            case KEY_EVENT:
                // ���������, ���� �� ������ ������� (�� ��������)
                if (irInBuf[i].Event.KeyEvent.bKeyDown) {
                    WORD keyCode = irInBuf[i].Event.KeyEvent.wVirtualKeyCode;
                    // ��������� Q ��� ESC ��� ������
                    if (keyCode == 'Q' || keyCode == VK_ESCAPE) {
                        // ��������������� �������� ����� ������� � �������
                        SetConsoleMode(hStdin, fdwSaveOldMode);
                        return 0;
                    }
                }
                break;
            }
        }
    }

    // ��������������� �������� ����� �������
    SetConsoleMode(hStdin, fdwSaveOldMode);
    return 0;
}

/**
 * @brief ������������ ������ ������� ����.
 * @param mer ��������� � ������� � ������� ����.
 */
void MouseEventProc(MOUSE_EVENT_RECORD mer) {
    // ���������� ������� � ������� (������������ �������� ������ ���� ������ �������)
    int x = mer.dwMousePosition.X;
    int y = mer.dwMousePosition.Y;

    // ���������� '\r', ����� �������� ����� �� ��� �� ������
    // � ������� ������, ����� ������� ������ �����
    printf("\r%-40s", ""); // �������
    printf("\r������: (%03d, %03d) | ", x, y);

    // ��������� ��� �������
    switch (mer.dwEventFlags) {
    case MOUSE_MOVED:
        // ������� MOUSE_MOVED ����������� ��� ����������� �������
        printf("��������: �����������");
        break;
    case 0: // ��� ������� ����� (������ ���� ������ ��� ��������)
        // ��������� ��������� ������
        if (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
            // FROM_LEFT_1ST_BUTTON_PRESSED - ���� ������� ���
            printf("��������: **��� ������!**");
        }
        else if (mer.dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
            printf("��������: ��� ������");
        }
        // ���������, ���� �� ������ �������� (������ ���� �� ���� ������ ������)
        else if (mer.dwButtonState == 0) {
            printf("��������: ������ ��������");
        }
        break;
    case DOUBLE_CLICK:
        // ������� �������� �����
        printf("��������: ������� ���� ���");
        break;
        // ������ ������� (��������� ������ � �.�.) ����� �������� �����
    default:
        //printf("��������: ������ ������� (%04X)", mer.dwEventFlags);
        break;
    }
}