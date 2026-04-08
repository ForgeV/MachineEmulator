#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "config.h"
#include "worker.h"

HANDLE hShutdownEvent = NULL;
HANDLE hConsoleMutex = NULL;

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT) {
        WaitForSingleObject(hConsoleMutex, INFINITE);
        printf("\n[MAIN] Получен сигнал прерывания. Инициируем безопасную остановку потоков...\n");
        ReleaseMutex(hConsoleMutex);
        if (hShutdownEvent != NULL) {
            SetEvent(hShutdownEvent);
        }
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char *argv[]) {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    AppConfig config = {0};

    if (!load_configuration(argc, argv, &config)) {
        printf("[ERROR] Не удалось загрузить конфигурацию. Выход.\n");
        return EXIT_FAILURE;
    }

    hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    hConsoleMutex = CreateMutex(NULL, FALSE, NULL);

    if (hShutdownEvent == NULL || hConsoleMutex == NULL) {
        printf("[ERROR] Ошибка создания примитивов синхронизации.\n");
        return EXIT_FAILURE;
    }

    if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE)) {
        printf("[ERROR] Не удалось установить обработчик консоли.\n");
        return EXIT_FAILURE;
    }

    printf("[MAIN] Эмулятор станка запущен. Нажмите Ctrl+C для выхода.\n");

    HANDLE hThreads[2];
    DWORD dwThreadIdArray[2];

    hThreads[0] = CreateThread(NULL, 0, operate_worker, &config, 0, &dwThreadIdArray[0]);
    hThreads[1] = CreateThread(NULL, 0, error_worker, &config, 0, &dwThreadIdArray[1]);

    if (hThreads[0] == NULL || hThreads[1] == NULL) {
        printf("[ERROR] Ошибка создания потоков.\n");
        SetEvent(hShutdownEvent);
    } else {
        WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);
        printf("[MAIN] Все рабочие потоки успешно завершены.\n");
    }
    if (hThreads[0]) CloseHandle(hThreads[0]);
    if (hThreads[1]) CloseHandle(hThreads[1]);
    CloseHandle(hShutdownEvent);
    CloseHandle(hConsoleMutex);

    printf("[MAIN] Программа завершена.\n");
    return EXIT_SUCCESS;
}

