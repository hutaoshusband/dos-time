#include "gui.h"
#include "install.h"
#include <windows.h>
#include <string>

// Die Hauptfunktion f�r eine Windows-GUI-Anwendung (WinMain statt main)
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow
) {
    // Unbenutzte Parameter ignorieren
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 1. Autostart-Registrierung beim ersten Start

    // Holt den vollst�ndigen Pfad der aktuellen ausf�hrbaren Datei
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring appPath(path);

    // �berpr�ft, ob die App bereits im Autostart ist. Wenn nicht, wird sie hinzugef�gt.
    if (!IsInStartup()) {
        AddToStartup(appPath);
    }

    // 2. GUI-Setup und Ausf�hrung

    // Registriert die Fensterklasse
    if (!RegisterClockWindowClass(hInstance)) {
        // Fehlerbehandlung
        return 1;
    }

    // Erstellt das Vollbild-Fenster
    HWND hWnd = CreateClockWindow(hInstance, nCmdShow);
    if (!hWnd) {
        // Fehlerbehandlung
        return 2;
    }

    // Hauptnachrichtenschleife
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
