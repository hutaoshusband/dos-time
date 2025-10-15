#pragma once

// KORREKTUR: Winsock2 MUSS vor windows.h eingebunden werden, um Neudefinitionsfehler zu vermeiden.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <string> // Wird für std::wstring benötigt

// Fensterklasse und Timer-IDs
#define WINDOW_CLASS_NAME L"TimeClockConsoleClass"
#define CLOCK_TIMER_ID 1
#define COUNTDOWN_TIMER_ID 2

// Prototypen für gui.cpp
BOOL RegisterClockWindowClass(HINSTANCE hInstance);
HWND CreateClockWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK ClockWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void UpdateClockDisplay(HWND hWnd);

// Deklarationen für die Zeitfunktionen, die in ProcessCommand verwendet werden
std::wstring GetCurrentTimeString();
std::wstring GetCurrentDateString();
