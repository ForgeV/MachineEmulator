#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "worker.h"
#include "ui_helper.h"

// Глобальные "ручки" (хэндлы) для управления всей системой.
// Event — это по сути "флаг" или "красная кнопка". Подняли флаг и все потоки видят, что пора остановиться.
HANDLE hShutdownEvent = NULL;
// Mutex здесь нужен, чтобы потоки не перебивали друг друга, когда пишут в консоль.
HANDLE hConsoleMutex = NULL;

// Это "обработчик сигналов". Вызывается, когда пользователь жмет Ctrl+C или закрывает крестиком.
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT) {
        // Захватываем мьютекс консоли.
        WaitForSingleObject(hConsoleMutex, INFINITE);
        printf("\n[MAIN] Получен сигнал прерывания. Инициируем безопасную остановку потоков...\n");
        ReleaseMutex(hConsoleMutex);

        if (hShutdownEvent != NULL) {
            // "Нажимаем кнопку" — переводим событие в сигнальное состояние.
            SetEvent(hShutdownEvent);
        }
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char *argv[]) {
    // Старт замеров времени работы программы
    LARGE_INTEGER frequency;
    LARGE_INTEGER start_time, end_time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start_time);
    // 65001 кодовая страница UTF-8.
    // Без этого кириллица в консоли превратится в иероглифы.
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    AppConfig config = {0}; // Обнуляем структуру, чтобы там не было мусора из памяти.

    // Загружаем настройки. Если конфиг битый выходим.
    if (!load_configuration(argc, argv, &config)) {
        printf("[ERROR] Не удалось загрузить конфигурацию. Выход.\n");
        return EXIT_FAILURE;
    }

    // Создаем событие.
    hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    // Создаем мьютекс. FALSE значит, что при создании мы его сразу не захватываем.
    hConsoleMutex = CreateMutex(NULL, FALSE, NULL);

    if (hShutdownEvent == NULL || hConsoleMutex == NULL) {
        printf("[ERROR] Ошибка создания примитивов синхронизации.\n");
        return EXIT_FAILURE;
    }

    // Регистрируем наш обработчик Ctrl+C в системе.
    if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE)) {
        printf("[ERROR] Не удалось установить обработчик консоли.\n");
        return EXIT_FAILURE;
    }

    // Массивы под пути к файлам шаблонов.
    char op_path[MAX_PATH], err_path[MAX_PATH];

    // Вызываем диалоговые окна.
    prompt_for_template_file(op_path, "Choose a template OPERATE file");
    prompt_for_template_file(err_path, "Choose a template ERROR file");

    // Если пользователь нажал "Отмена", подставляем дефолты.
    if (strlen(op_path) <= 0) {
        strcpy(op_path,"../source/source_operate.txt");
    }
    if (strlen(err_path) <= 0) {
        strcpy(err_path,"../source/source_error.txt");
    }

    // Сохраняем выбранные пути в общую структуру конфига.
    strcpy(config.path_source_op, op_path);
    strcpy(config.path_source_err, err_path);

    printf("[MAIN] Эмулятор станка запущен. Нажмите Ctrl+C для выхода.\n");

    // Хэндлы для двух потоков.
    HANDLE hThreads[2];
    DWORD dwThreadIdArray[2];

    // CreateThread создает параллельную ветку выполнения.
    // Поток для логов операций.
    hThreads[0] = CreateThread(NULL, 0, operate_worker, &config, 0, &dwThreadIdArray[0]);
    // Поток для логов ошибок.
    hThreads[1] = CreateThread(NULL, 0, error_worker, &config, 0, &dwThreadIdArray[1]);

    if (hThreads[0] == NULL || hThreads[1] == NULL) {
        printf("[ERROR] Ошибка создания потоков.\n");
        SetEvent(hShutdownEvent); // Если один не создался, останавливаем оба.
    } else {
        // Main-поток засыпает и ждет, пока оба рабочих потока не завершатся.
        WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);
        printf("[MAIN] Все рабочие потоки успешно завершены.\n");
    }

    // Закрываем все потоки.
    if (hThreads[0]) CloseHandle(hThreads[0]);
    if (hThreads[1]) CloseHandle(hThreads[1]);
    CloseHandle(hShutdownEvent);
    CloseHandle(hConsoleMutex);

    // Считаем фактическое время в миллисекундах
    QueryPerformanceCounter(&end_time);
    double total_actual_ms = (double)(end_time.QuadPart - start_time.QuadPart) * 1000.0 / frequency.QuadPart;


    printf("[MAIN] Программа завершена.\n");
    printf("[MAIN] Средняя скорость записи: %f б/мс", totalBytes/total_actual_ms);
    return EXIT_SUCCESS;
}