#include <windows.h>
#include <iostream>
#include <cstdio>
#include <string>

// Определение размера буфера для событий
#define EVENT_BUFFER_SIZE 128

// Прототип функции для обработки событий мыши
void MouseEventProc(MOUSE_EVENT_RECORD mer);

int main() {
    setlocale(LC_ALL, "rus");
    // Получаем хэндл стандартного ввода (консоли)
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE) {
        std::cerr << "GetStdHandle Error: " << GetLastError() << std::endl;
        return 1;
    }

    // Сохраняем текущий режим консоли, чтобы потом его восстановить
    DWORD fdwSaveOldMode;
    if (!GetConsoleMode(hStdin, &fdwSaveOldMode)) {
        std::cerr << "GetConsoleMode Error: " << GetLastError() << std::endl;
        return 1;
    }

    // --- Настройка режима консоли ---
    // Включаем события окна и мыши.
    // ENABLE_QUICK_EDIT_MODE обычно отключается при включении ENABLE_MOUSE_INPUT,
    // чтобы приложение получало события, а не Windows для выделения текста.
    DWORD fdwMode = ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT;

    if (!SetConsoleMode(hStdin, fdwMode)) {
        std::cerr << "SetConsoleMode Error: " << GetLastError() << std::endl;
        // Продолжаем, но без обработки событий мыши, если SetConsoleMode не сработал
    }

    // --- Подготовка к чтению событий ---
    INPUT_RECORD irInBuf[EVENT_BUFFER_SIZE];
    DWORD cNumRead;

    // Выводим инструкции
    std::cout << "--- Mouse Tracker CLI ---" << std::endl;
    std::cout << "Перемещайте мышь или кликайте ЛКМ в окне консоли." << std::endl;
    std::cout << "Нажмите 'Q' или 'Esc' для выхода." << std::endl;

    // --- Главный цикл обработки событий ---
    while (true) {
        // Ожидаем события в буфере консольного ввода
        if (!ReadConsoleInput(
            hStdin,      // Хэндл ввода
            irInBuf,     // Буфер для чтения
            EVENT_BUFFER_SIZE, // Размер буфера
            &cNumRead))  // Количество прочитанных записей
        {
            std::cerr << "ReadConsoleInput Error: " << GetLastError() << std::endl;
            break;
        }

        // Обрабатываем все полученные события
        for (DWORD i = 0; i < cNumRead; i++) {
            switch (irInBuf[i].EventType) {
            case MOUSE_EVENT:
                MouseEventProc(irInBuf[i].Event.MouseEvent);
                break;

            case KEY_EVENT:
                // Проверяем, была ли нажата клавиша (не отпущена)
                if (irInBuf[i].Event.KeyEvent.bKeyDown) {
                    WORD keyCode = irInBuf[i].Event.KeyEvent.wVirtualKeyCode;
                    // Проверяем Q или ESC для выхода
                    if (keyCode == 'Q' || keyCode == VK_ESCAPE) {
                        // Восстанавливаем исходный режим консоли и выходим
                        SetConsoleMode(hStdin, fdwSaveOldMode);
                        return 0;
                    }
                }
                break;
            }
        }
    }

    // Восстанавливаем исходный режим консоли
    SetConsoleMode(hStdin, fdwSaveOldMode);
    return 0;
}

/**
 * @brief Обрабатывает записи событий мыши.
 * @param mer Структура с данными о событии мыши.
 */
void MouseEventProc(MOUSE_EVENT_RECORD mer) {
    // Координаты курсора в ячейках (относительно верхнего левого угла буфера консоли)
    int x = mer.dwMousePosition.X;
    int y = mer.dwMousePosition.Y;

    // Используем '\r', чтобы обновить вывод на той же строке
    // и очищаем строку, чтобы удалить старый текст
    printf("\r%-40s", ""); // Очистка
    printf("\rКурсор: (%03d, %03d) | ", x, y);

    // Проверяем тип события
    switch (mer.dwEventFlags) {
    case MOUSE_MOVED:
        // Событие MOUSE_MOVED срабатывает при перемещении курсора
        printf("Действие: Перемещение");
        break;
    case 0: // Это событие клика (кнопка была нажата или отпущена)
        // Проверяем состояние кнопок
        if (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
            // FROM_LEFT_1ST_BUTTON_PRESSED - флаг нажатия ЛКМ
            printf("Действие: **ЛКМ НАЖАТА!**");
        }
        else if (mer.dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
            printf("Действие: ПКМ НАЖАТА");
        }
        // Проверяем, была ли кнопка отпущена (только если не было других флагов)
        else if (mer.dwButtonState == 0) {
            printf("Действие: Кнопка отпущена");
        }
        break;
    case DOUBLE_CLICK:
        // Событие двойного клика
        printf("Действие: ДВОЙНОЙ КЛИК ЛКМ");
        break;
        // Другие события (прокрутка колеса и т.д.) можно добавить здесь
    default:
        //printf("Действие: Другое событие (%04X)", mer.dwEventFlags);
        break;
    }
}