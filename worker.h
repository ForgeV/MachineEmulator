#ifndef WORKER_H
#define WORKER_H

#include <windows.h>

DWORD WINAPI operate_worker(LPVOID lpParam);
DWORD WINAPI error_worker(LPVOID lpParam);

#endif