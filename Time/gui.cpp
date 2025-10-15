#include "gui.h"
#include "blocker.h" // NEU: Header für Task Manager Blocker
#include <windows.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm> // Für std::transform
#include <cwctype> // Hinzugefügt für std::towupper
#include <sddl.h> // NEU: Benötigt für IsElevated() - Sicherheitsfunktionen

// Handler für die benutzerdefinierte Schriftart (Es wird nur noch eine, kleine Terminal-Schrift verwendet)
HFONT g_hFont = NULL;

// Startwert für den Countdown (10 Sekunden)
int g_countdownSeconds = 10;
// Zustand, ob der Countdown aktiv ist (startet erst mit 'EXIT')
bool g_countdownActive = false;
// Gibt an, ob der Benutzer noch eingeben darf (wird bei SHUTDOWN deaktiviert)
bool g_isTypingEnabled = true;

// Zustand der Konsole
std::wstring g_inputBuffer;
std::vector<std::wstring> g_consoleHistory;
const std::wstring PROMPT = L"C:\\> ";

// Konstanten für Terminal-Meldungen
const std::wstring MSG_HELP =
L"Verfuegbare Befehle:\n"
L"  DATE  - Zeigt das aktuelle Datum an.\n"
L"  TIME  - Zeigt die aktuelle Uhrzeit an.\n"
L"  EXIT  - Startet die Systemterminierung.\n"
L"  CLEAR - Leert den Konsolenbildschirm.\n" // NEU: CLEAR Befehl hinzugefügt
L"  HELP  - Zeigt diese Hilfe an.";

/**
 * Konvertiert einen String vollständig zu Großbuchstaben.
 * Diese Änderung stellt die Unempfindlichkeit gegenüber Groß-/Kleinschreibung sicher.
 */
std::wstring ToUpper(const std::wstring& str) {
    std::wstring s = str;
    // std::transform wendet std::towupper auf jeden Charakter an
    std::transform(s.begin(), s.end(), s.begin(),
        [](wchar_t c) { return std::towupper(c); });
    return s;
}

/**
 * Fügt eine oder mehrere Zeilen zum Konsolenverlauf hinzu.
 */
void AddHistory(const std::wstring& text) {
    // Teilt Text bei Newline-Zeichen, um Zeilenumbruch zu simulieren
    std::wstringstream ss(text);
    std::wstring line;
    while (std::getline(ss, line, L'\n')) {
        g_consoleHistory.push_back(line);
    }
}

/**
 * Holt die aktuelle Uhrzeit und formatiert sie als breiten String (wchar_t*).
 * Format: HH:MM:SS
 * Diese Funktion muss deklariert sein, bevor sie von ProcessCommand verwendet wird (fix in gui.h).
 */
std::wstring GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;

    // Konvertiert time_t sicher in tm-Struktur
    if (localtime_s(&tm_buf, &now_c) != 0) {
        return L"FEHLER";
    }

    std::wstringstream ss;
    ss << std::put_time(&tm_buf, L"%H:%M:%S");
    return ss.str();
}

/**
 * Holt das aktuelle Datum und formatiert es als breiten String (wchar_t*).
 * Format: DD.MM.JJJJ
 * Diese Funktion muss deklariert sein, bevor sie von ProcessCommand verwendet wird (fix in gui.h).
 */
std::wstring GetCurrentDateString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;

    if (localtime_s(&tm_buf, &now_c) != 0) {
        return L"FEHLER";
    }

    std::wstringstream ss;
    ss << std::put_time(&tm_buf, L"%d.%m.%Y");
    return ss.str();
}

/**
 * Führt den eingegebenen Befehl aus und aktualisiert den Verlauf.
 */
void ProcessCommand(HWND hWnd, const std::wstring& command) {
    // Trimmt Leerzeichen am Ende des Befehls
    std::wstring trimmedCommand = command;
    size_t end = trimmedCommand.find_last_not_of(L" \t\n\r\f\v");
    if (end != std::wstring::npos) {
        trimmedCommand.resize(end + 1);
    }

    // upperCommand ist nun komplett in Großbuchstaben, um Case-Insensitivity zu gewährleisten
    std::wstring upperCommand = ToUpper(trimmedCommand);
    g_consoleHistory.push_back(PROMPT + trimmedCommand); // Eingegebene Zeile in den Verlauf

    if (upperCommand == L"HELP") {
        AddHistory(MSG_HELP);
    }
    else if (upperCommand == L"DATE") {
        AddHistory(L"Aktuelles Datum ist " + GetCurrentDateString());
        AddHistory(L"Geben Sie das neue Datum ein (TT.MM.JJ): ");
    }
    else if (upperCommand == L"TIME") {
        AddHistory(L"Aktuelle Zeit ist " + GetCurrentTimeString());
        AddHistory(L"Geben Sie die neue Zeit ein (HH:MM:SS): ");
    }
    else if (upperCommand == L"CLEAR") { // LOGIK FÜR CLEAR
        g_consoleHistory.clear();
        AddHistory(L"Bildschirm geloescht.");
    }
    else if (upperCommand == L"EXIT") {
        g_isTypingEnabled = false;
        g_countdownActive = true;
        AddHistory(L"EXIT.BAT: Terminierung gestartet. Bitte warten...");
        // Startet den Countdown-Timer
        SetTimer(hWnd, COUNTDOWN_TIMER_ID, 1000, NULL);
    }
    else if (!trimmedCommand.empty()) {
        // Fehlerhafte Befehle
        AddHistory(L"Ungueltiger Befehl oder Dateiname. Tippen Sie 'HELP'.");
    }

    // Eingabepuffer leeren und Display aktualisieren
    g_inputBuffer.clear();
    UpdateClockDisplay(hWnd);
}

/**
 * Registriert die benutzerdefinierte Fensterklasse.
 */
BOOL RegisterClockWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = ClockWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    // Dark Mode Terminal Look: Hintergrundfarbe auf Schwarz setzen
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    return (RegisterClassExW(&wc) != 0);
}

// NEUE HILFSFUNKTION: Prüft, ob das Programm mit erhöhten Rechten läuft (Administrator-Rechte)
BOOL IsElevated() {
    BOOL isElevated = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, cbSize, &cbSize)) {
            isElevated = elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return isElevated;
}

/**
 * Erstellt das Vollbild-Fenster ohne Rahmen.
 */
HWND CreateClockWindow(HINSTANCE hInstance, int nCmdShow) {
    // Holt die Bildschirmabmessungen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Initialisiere den Konsolenverlauf mit der Boot-Sequenz
    g_consoleHistory.push_back(L"MS-DOS Version 6.22");
    g_consoleHistory.push_back(L"(C)Copyright Microsoft Corporation 1981-1994.");
    // Autorenzeile hinzugefügt
    g_consoleHistory.push_back(L"Made with \x2764 by Gemini");
    g_consoleHistory.push_back(L"");
    g_consoleHistory.push_back(L"Tippen Sie 'HELP' fuer eine Liste der Befehle ein.");
    g_consoleHistory.push_back(L"");

    // Erstellt das Vollbildfenster ohne Rahmen (WS_POPUP) und OHNE Systemmenü (kein Alt+F4)
    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST,
        WINDOW_CLASS_NAME,
        L"Windows 3.1 Terminal Clock",
        WS_POPUP | WS_VISIBLE,
        0, 0,                            // Position (oben links)
        screenWidth, screenHeight,       // Größe (Vollbild)
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hWnd) {
        // Einheitliche, sehr kleine Schriftgröße (ca. 1/45 der Höhe) für den realistischen Terminal-Look
        g_hFont = CreateFont(
            screenHeight / 45, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, // FW_NORMAL, da der Text klein ist
            OEM_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            NONANTIALIASED_QUALITY, FF_MODERN | FIXED_PITCH, L"Terminal"
        );

        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
        // Startet nur den Timer für die Bildschirmausgabe
        SetTimer(hWnd, CLOCK_TIMER_ID, 50, NULL); // Schnellerer Timer für Cursor-Simulation/flüssigere Eingabe
        // COUNTDOWN_TIMER_ID wird erst bei 'EXIT' gestartet

        // NEU: Task Manager Blockade aktivieren
        EnableTaskMgrBlock(TRUE);

        // NEU: Rechteprüfung und Warnung in der Konsole
        if (!IsElevated()) {
            g_consoleHistory.push_back(L"");
            g_consoleHistory.push_back(L"-----------------------------------------");
            g_consoleHistory.push_back(L"WARNUNG: Systemrechte fehlen.");
            g_consoleHistory.push_back(L"Die Blockierung des Task Managers (Registry) schlaegt ohne Admin-Rechte fehl.");
            g_consoleHistory.push_back(L"Bitte als Administrator ausfuehren, um volle Konsolen-Sicherheit zu gewaehrleisten.");
            g_consoleHistory.push_back(L"-----------------------------------------");
            g_consoleHistory.push_back(L"");
        }
        else {
            g_consoleHistory.push_back(L"");
            g_consoleHistory.push_back(L"SYSTEM: Sicherheitsprotokolle aktiviert. OK.");
            g_consoleHistory.push_back(L"");
        }
    }
    return hWnd;
}


void UpdateClockDisplay(HWND hWnd) {
    // Erzwingt das Neuzeichnen des Fensters (löst WM_PAINT aus)
    InvalidateRect(hWnd, NULL, TRUE);
}

/**
 * Nachrichtenprozedur des Fensters, die alle Ereignisse behandelt.
 */
LRESULT CALLBACK ClockWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // Countdown beginnt nicht sofort.
        break;
    }

                  // WICHTIG: EraseBkgnd abfangen, um Flackern zu verhindern
    case WM_ERASEBKGND:
        return TRUE;

        // WM_CHAR fängt alle druckbaren Zeichen und Backspace ab
    case WM_CHAR: {
        if (!g_isTypingEnabled) break;

        // Backspace (löscht das letzte Zeichen)
        if (wParam == VK_BACK) {
            if (!g_inputBuffer.empty()) {
                g_inputBuffer.pop_back();
            }
            // Enter wird in WM_KEYDOWN verarbeitet, wird hier ignoriert
        }
        else if (wParam == VK_RETURN) {

            // ESC (simuliert eine Löschung der aktuellen Zeile)
        }
        else if (wParam == VK_ESCAPE) {
            g_inputBuffer.clear();

            // Alle anderen druckbaren Zeichen (z.B. A-Z, 0-9, Satzzeichen)
        }
        else if (wParam >= 32 && wParam < 255) { // 32 ist Leerzeichen
            g_inputBuffer += (wchar_t)wParam;
        }
        UpdateClockDisplay(hWnd);
        break;
    }

                // WM_KEYDOWN fängt spezielle Tasten wie Enter ab
    case WM_KEYDOWN: {
        if (wParam == VK_RETURN && g_isTypingEnabled) {
            // Enter wurde gedrückt: Befehl verarbeiten
            ProcessCommand(hWnd, g_inputBuffer);
        }
        break;
    }

                   // WM_SYSCOMMAND fängt Systembefehle wie Alt+F4 ab
    case WM_SYSCOMMAND:
        // SC_CLOSE ist der Befehl, der durch Alt+F4 oder das Schließen des Fensters ausgelöst wird
        if (wParam == SC_CLOSE) {
            // Wir ignorieren Schließversuche, solange der Countdown nicht aktiv ist
            if (!g_countdownActive) {
                // Fügt eine Fehlermeldung in den Verlauf ein
                g_consoleHistory.push_back(PROMPT + L"");
                AddHistory(L"FEHLER: Diese Applikation kann nur durch den 'EXIT'-Befehl beendet werden.");
                g_inputBuffer.clear();
                UpdateClockDisplay(hWnd);
                return 0; // Befehl nicht weiterleiten
            }
        }
        break;

    case WM_TIMER:
        if (wParam == CLOCK_TIMER_ID) {
            // Reguläres Update des Displays (zur Simulation des blinkenden Cursors)
            UpdateClockDisplay(hWnd);
        }
        else if (wParam == COUNTDOWN_TIMER_ID) {
            // Countdown-Logik (nur aktiv, wenn g_countdownActive=true)
            if (g_countdownActive) {
                if (g_countdownSeconds > 0) {
                    g_countdownSeconds--;
                    UpdateClockDisplay(hWnd); // Neuzeichnen, um den Countdown-Text zu aktualisieren
                }

                if (g_countdownSeconds <= 0) {
                    // Countdown ist abgelaufen: Timer stoppen und Fenster schließen
                    KillTimer(hWnd, COUNTDOWN_TIMER_ID);
                    DestroyWindow(hWnd);
                }
            }
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT clientRect;
        GetClientRect(hWnd, &clientRect);

        // --- FIX: Explizites Füllen mit Schwarz (löst Hintergrund- und Backspace-Problem) ---
        HBRUSH hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
        FillRect(hdc, &clientRect, hBrush);

        // --- Zeichnen im Dark Mode Terminal Stil ---

        SetTextColor(hdc, RGB(220, 220, 220));
        SetBkMode(hdc, TRANSPARENT);

        // Terminal Look: Wir nutzen die Höhe der einzigen Schrift als Zeilenhöhe
        TEXTMETRIC tm;
        HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont); // Wählt die Terminal-Schriftart
        GetTextMetrics(hdc, &tm);
        int lineHeight = tm.tmHeight + tm.tmExternalLeading;
        int xPadding = 20; // 20 Pixel Abstand vom linken Rand
        int currentLine = 1;

        // Maximal sichtbare Zeilen berechnen
        int maxVisibleLines = clientRect.bottom / lineHeight;

        // Nur die letzten (maxVisibleLines - 1) Zeilen des Verlaufs zeichnen
        int startLine = 0;
        // Wir reservieren immer eine Zeile für den aktuellen Prompt/Shutdown-Status
        if (g_consoleHistory.size() > maxVisibleLines - 1) {
            startLine = g_consoleHistory.size() - (maxVisibleLines - 1);
        }

        // Verlauf zeichnen
        for (size_t i = startLine; i < g_consoleHistory.size(); ++i) {
            RECT rect = { xPadding, lineHeight * currentLine++, clientRect.right, lineHeight * currentLine };
            DrawTextW(hdc, g_consoleHistory[i].c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
        }

        // Letzte Zeile: Aktiver Prompt oder Shutdown-Meldung

        if (g_countdownActive) {
            // Shutdown-Meldung
            std::wstringstream shutdownSS;
            if (g_countdownSeconds > 0) {
                shutdownSS << L"EXIT.BAT: Terminierung in " << g_countdownSeconds << L" Sekunde(n)...";
            }
            else {
                shutdownSS << L"EXIT.BAT: SYSTEM SHUTDOWN. Goodbye.";
            }
            RECT rect = { xPadding, lineHeight * currentLine++, clientRect.right, lineHeight * currentLine };
            DrawTextW(hdc, shutdownSS.str().c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);

        }
        else {
            // Aktiver Prompt und Eingabe
            std::wstring promptAndInput = PROMPT + g_inputBuffer;

            // Cursor-Simulation: Blinkender Unterstrich am Ende der Eingabe
            if ((GetTickCount() / 500) % 2 == 0) {
                promptAndInput += L'_';
            }
            else {
                promptAndInput += L' '; // Füllt Platz des Cursors
            }

            RECT rect = { xPadding, lineHeight * currentLine++, clientRect.right, lineHeight * currentLine };
            DrawTextW(hdc, promptAndInput.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
        }

        SelectObject(hdc, hOldFont);
        EndPaint(hWnd, &ps);
        break;
    }

    case WM_DESTROY:
        // Bereinigt Timer und Schriftarten beim Schließen
        KillTimer(hWnd, CLOCK_TIMER_ID);
        KillTimer(hWnd, COUNTDOWN_TIMER_ID);
        if (g_hFont) {
            DeleteObject(g_hFont);
            g_hFont = NULL;
        }
        // NEU: Task Manager Blockade deaktivieren (Cleanup)
        EnableTaskMgrBlock(FALSE);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}
