#include "gui.h"

// Spezifische Header für diese Implementierungsdatei
#include <wininet.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <psapi.h>
#include <tcpmib.h>
#include <tlhelp32.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <cmath>
#include <fstream>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// Handler für die benutzerdefinierte Schriftart
HFONT g_hFont = NULL;

// Countdown-Variablen
int g_countdownSeconds = 10;
bool g_countdownActive = false;
bool g_isTypingEnabled = true;

// Konsolen-Variablen
std::wstring g_inputBuffer;
std::vector<std::wstring> g_consoleHistory;
const std::wstring PROMPT = L"C:\\> ";
bool g_awaitingUpdateConfirmation = false;
int g_scrollOffset = 0; // Für Scroll-Funktionalität

// Konstante für PI
const double PI = 3.14159265358979323846;

// Hilfetext mit neuen Befehlen
const std::wstring MSG_HELP =
L"Verfuegbare Befehle:\n"
L"  DATE           - Zeigt das aktuelle Datum an.\n"
L"  TIME           - Zeigt die aktuelle Uhrzeit an.\n"
L"  CLEAR / CLS    - Leert den Konsolenbildschirm.\n"
L"  UPDATE         - Sucht und installiert automatisch Updates.\n"
L"  EXIT           - Startet die Systemterminierung.\n"
L"  HELP           - Zeigt diese Hilfe an.\n"
L"  ECHO <text>    - Gibt den angegebenen Text aus.\n"
L"  VER            - Zeigt die Version an.\n"
L"  VOL            - Zeigt die Datentraegerbezeichnung an.\n"
L"  DIR            - Listet den Inhalt des aktuellen Verzeichnisses auf.\n"
L"  TYPE <file>    - Zeigt den Inhalt einer Textdatei an.\n"
L"  HOSTNAME       - Zeigt den Computernamen an.\n"
L"  WHOAMI         - Zeigt den aktuellen Benutzernamen an.\n"
L"  UPTIME         - Zeigt die Systemlaufzeit an.\n\n"
L"Netzwerk-Tools:\n"
L"  PING <host>    - Sendet ICMP-Anfragen an einen Host.\n"
L"  IPCONFIG       - Zeigt die Netzwerkkonfiguration an.\n"
L"  NETSTAT        - Zeigt aktive TCP-Verbindungen an.\n\n"
L"System-Tools:\n"
L"  SYSTEMINFO     - Zeigt Systeminformationen an.\n"
L"  TASKLIST       - Listet laufende Prozesse auf.\n\n"
L"Mathematik & Konvertierung:\n"
L"  SQRT, POW, LOG, LOG10, SIN, COS, TAN, HEX, DEC";


// Hilfsfunktion zur Konvertierung von Grad in Radiant
double DegToRad(double degrees) {
    return degrees * PI / 180.0;
}

/**
 * Konvertiert einen String vollständig zu Großbuchstaben.
 */
std::wstring ToUpper(const std::wstring& str) {
    std::wstring s = str;
    std::transform(s.begin(), s.end(), s.begin(),
        [](wchar_t c) { return std::towupper(c); });
    return s;
}

/**
 * Fügt eine oder mehrere Zeilen zum Konsolenverlauf hinzu und setzt den Scroll-Offset zurück.
 */
void AddHistory(const std::wstring& text) {
    std::wstringstream ss(text);
    std::wstring line;
    while (std::getline(ss, line, L'\n')) {
        g_consoleHistory.push_back(line);
    }
    g_scrollOffset = 0; // Beim Hinzufügen neuer Inhalte nach unten scrollen
}

// *** NEUE BEFEHLSFUNKTIONEN ***

void Ping(const std::wstring& host, HWND hWnd);
void IpConfig(HWND hWnd);
void TaskList(HWND hWnd);
void SystemInfo(HWND hWnd);
void Netstat(HWND hWnd);
void Vol(HWND hWnd);
void Dir(HWND hWnd);
void Type(const std::wstring& filename, HWND hWnd);
void Hostname(HWND hWnd);
void Whoami(HWND hWnd);
void Uptime(HWND hWnd);

/**
 * IPCONFIG-Implementierung
 */
void IpConfig(HWND hWnd) {
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
    if (pAdapterInfo == NULL) {
        AddHistory(L"FEHLER: Speicherzuweisung fehlgeschlagen.");
        return;
    }

    if (GetAdaptersInfo(pAdapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
        if (pAdapterInfo == NULL) {
            AddHistory(L"FEHLER: Speicherzuweisung fehlgeschlagen.");
            return;
        }
    }

    if (GetAdaptersInfo(pAdapterInfo, &bufferSize) == NO_ERROR) {
        AddHistory(L"Windows IP-Konfiguration");
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            std::string desc_s(pAdapter->Description);
            std::string ip_s(pAdapter->IpAddressList.IpAddress.String);
            std::string mask_s(pAdapter->IpAddressList.IpMask.String);
            std::string gateway_s(pAdapter->GatewayList.IpAddress.String);

            std::wstringstream ss;
            ss << L"\nEthernet-Adapter " << std::wstring(desc_s.begin(), desc_s.end()) << L":\n";
            ss << L"  IP-Adresse. . . . . . . . . . . : " << std::wstring(ip_s.begin(), ip_s.end()) << L"\n";
            ss << L"  Subnetzmaske. . . . . . . . . . : " << std::wstring(mask_s.begin(), mask_s.end()) << L"\n";
            ss << L"  Standardgateway . . . . . . . . : " << std::wstring(gateway_s.begin(), gateway_s.end());
            AddHistory(ss.str());
            pAdapter = pAdapter->Next;
        }
    }
    else {
        AddHistory(L"FEHLER: Netzwerkinformationen konnten nicht abgerufen werden.");
    }

    if (pAdapterInfo) {
        free(pAdapterInfo);
    }
}

void SystemInfo(HWND hWnd) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    OSVERSIONINFOEXW osInfo;
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
#pragma warning(suppress : 4996)
    GetVersionExW((LPOSVERSIONINFOW)&osInfo);

    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);

    std::wstringstream ss;
    ss << L"Systeminformationen:\n";
    ss << L"  Betriebssystem: Windows (Version " << osInfo.dwMajorVersion << L"." << osInfo.dwMinorVersion << L")\n";
    ss << L"  Prozessortyp: " << sysInfo.dwProcessorType << L"\n";
    ss << L"  Anzahl der Prozessoren: " << sysInfo.dwNumberOfProcessors << L"\n";
    ss << L"  Speicher (RAM): " << memStatus.ullTotalPhys / (1024 * 1024) << L" MB";
    AddHistory(ss.str());
}

void TaskList(HWND hWnd) {
    AddHistory(L"Abbildname              PID");
    AddHistory(L"========================= ========");

    HANDLE hProcessSnap;
    PROCESSENTRY32W pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        AddHistory(L"FEHLER: Prozess-Snapshot konnte nicht erstellt werden.");
        return;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hProcessSnap, &pe32)) {
        do {
            std::wstringstream ss;
            ss << std::left << std::setw(25) << pe32.szExeFile
                << std::right << std::setw(8) << pe32.th32ProcessID;
            AddHistory(ss.str());
        } while (Process32NextW(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
}

const wchar_t* TcpStateToString(DWORD state) {
    switch (state) {
    case MIB_TCP_STATE_CLOSED: return L"CLOSED";
    case MIB_TCP_STATE_LISTEN: return L"LISTEN";
    case MIB_TCP_STATE_SYN_SENT: return L"SYN-SENT";
    case MIB_TCP_STATE_SYN_RCVD: return L"SYN-RECEIVED";
    case MIB_TCP_STATE_ESTAB: return L"ESTABLISHED";
    case MIB_TCP_STATE_FIN_WAIT1: return L"FIN-WAIT-1";
    case MIB_TCP_STATE_FIN_WAIT2: return L"FIN-WAIT-2";
    case MIB_TCP_STATE_CLOSE_WAIT: return L"CLOSE-WAIT";
    case MIB_TCP_STATE_CLOSING: return L"CLOSING";
    case MIB_TCP_STATE_LAST_ACK: return L"LAST-ACK";
    case MIB_TCP_STATE_TIME_WAIT: return L"TIME-WAIT";
    case MIB_TCP_STATE_DELETE_TCB: return L"DELETE-TCB";
    default: return L"UNKNOWN";
    }
}

void Netstat(HWND hWnd) {
    PMIB_TCPTABLE pTcpTable;
    DWORD dwSize = 0;
    wchar_t szLocalAddr[128];
    wchar_t szRemoteAddr[128];
    struct in_addr IpAddr;

    pTcpTable = (MIB_TCPTABLE*)malloc(sizeof(MIB_TCPTABLE));
    if (pTcpTable == NULL) {
        AddHistory(L"FEHLER: Speicherzuweisung fehlgeschlagen.");
        return;
    }

    if (GetTcpTable(pTcpTable, &dwSize, TRUE) == ERROR_INSUFFICIENT_BUFFER) {
        free(pTcpTable);
        pTcpTable = (MIB_TCPTABLE*)malloc(dwSize);
        if (pTcpTable == NULL) {
            AddHistory(L"FEHLER: Speicherzuweisung fehlgeschlagen.");
            return;
        }
    }

    if (GetTcpTable(pTcpTable, &dwSize, TRUE) == NO_ERROR) {
        AddHistory(L"Aktive Verbindungen");
        AddHistory(L"  Proto  Lokale Adresse         Remoteadresse          Status");

        for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
            IpAddr.S_un.S_addr = (u_long)pTcpTable->table[i].dwLocalAddr;
            InetNtopW(AF_INET, &IpAddr, szLocalAddr, 128);

            IpAddr.S_un.S_addr = (u_long)pTcpTable->table[i].dwRemoteAddr;
            InetNtopW(AF_INET, &IpAddr, szRemoteAddr, 128);

            std::wstringstream ss;
            ss << L"  TCP    " << std::left << std::setw(21) << (std::wstring(szLocalAddr) + L":" + std::to_wstring(ntohs((u_short)pTcpTable->table[i].dwLocalPort)))
                << std::left << std::setw(21) << (std::wstring(szRemoteAddr) + L":" + std::to_wstring(ntohs((u_short)pTcpTable->table[i].dwRemotePort)))
                << TcpStateToString(pTcpTable->table[i].dwState);
            AddHistory(ss.str());
        }
    }
    else {
        AddHistory(L"FEHLER: TCP-Tabelle konnte nicht abgerufen werden.");
    }

    if (pTcpTable != NULL) {
        free(pTcpTable);
    }
}

void Vol(HWND hWnd) {
    wchar_t volumeName[MAX_PATH + 1] = { 0 };
    wchar_t fileSystemName[MAX_PATH + 1] = { 0 };
    DWORD serialNumber = 0;
    DWORD maxComponentLen = 0;
    DWORD fileSystemFlags = 0;

    if (GetVolumeInformationW(L"C:\\", volumeName, ARRAYSIZE(volumeName), &serialNumber, &maxComponentLen, &fileSystemFlags, fileSystemName, ARRAYSIZE(fileSystemName))) {
        std::wstringstream ss;
        ss << L" Volume in Laufwerk C hat die Bezeichnung " << (wcslen(volumeName) > 0 ? volumeName : L"") << L".\n";
        ss << L" Volumeseriennummer ist " << std::hex << std::uppercase << serialNumber;
        AddHistory(ss.str());
    }
    else {
        AddHistory(L"FEHLER: Volumeninformationen konnten nicht abgerufen werden.");
    }
}

void Dir(HWND hWnd) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    *wcsrchr(path, L'\\') = L'\0';

    std::wstring searchPath = std::wstring(path) + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        AddHistory(L"FEHLER: Verzeichnisinhalt konnte nicht gelesen werden.");
        return;
    }

    AddHistory(L" Inhalt von " + std::wstring(path));
    do {
        if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
            std::wstringstream ss;
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                ss << L"<DIR>    " << findData.cFileName;
            }
            else {
                ss << L"         " << findData.cFileName;
            }
            AddHistory(ss.str());
        }
    } while (FindNextFileW(hFind, &findData) != 0);

    FindClose(hFind);
}

void Type(const std::wstring& filename, HWND hWnd) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    *wcsrchr(path, L'\\') = L'\0';
    std::wstring fullPath = std::wstring(path) + L"\\" + filename;

    std::wifstream file(fullPath);
    if (file.is_open()) {
        std::wstring line;
        while (getline(file, line)) {
            AddHistory(line);
        }
        file.close();
    }
    else {
        AddHistory(L"FEHLER: Datei nicht gefunden oder konnte nicht geoeffnet werden.");
    }
}

void Hostname(HWND hWnd) {
    wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(computerName, &size)) {
        AddHistory(computerName);
    }
    else {
        AddHistory(L"FEHLER: Computername konnte nicht abgerufen werden.");
    }
}

void Whoami(HWND hWnd) {
    wchar_t userName[UNLEN + 1];
    DWORD size = UNLEN + 1;
    if (GetUserNameW(userName, &size)) {
        AddHistory(userName);
    }
    else {
        AddHistory(L"FEHLER: Benutzername konnte nicht abgerufen werden.");
    }
}

void Uptime(HWND hWnd) {
    ULONGLONG ticks = GetTickCount64();
    ULONGLONG seconds = ticks / 1000;
    ULONGLONG minutes = seconds / 60;
    ULONGLONG hours = minutes / 60;
    ULONGLONG days = hours / 24;

    std::wstringstream ss;
    ss << L"System-Uptime: " << days << L" Tage, "
        << hours % 24 << L" Stunden, "
        << minutes % 60 << L" Minuten, "
        << seconds % 60 << L" Sekunden";
    AddHistory(ss.str());
}

LRESULT CALLBACK BlackoutWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ERASEBKGND:
        // Verhindert ein Flackern und zeigt einen schwarzen Hintergrund
        return TRUE;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);

        // Füllt das Fenster explizit mit einem schwarzen Pinsel
        HBRUSH hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
        FillRect(hdc, &clientRect, hBrush);

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_CLOSE:
    case WM_DESTROY:
        // Wir wollen nicht, dass diese Fenster die Anwendung beenden
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}


BOOL RegisterBlackoutWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = BlackoutWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // Stellt den schwarzen Hintergrund sicher
    wc.lpszClassName = BLACKOUT_CLASS_NAME;
    return (RegisterClassExW(&wc) != 0);
}

/**
 * Enum-Callback-Funktion, um fuer jeden erkannten Monitor (ausser dem Hauptmonitor)
 * ein schwarzes, randloses Fenster zu erstellen, um den Bildschirm abzudecken.
 */
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    HWND hWndMain = *(HWND*)dwData;
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(monitorInfo);

    if (GetMonitorInfo(hMonitor, &monitorInfo)) {
        // Pruefen, ob es der Primaermonitor ist
        if (!(monitorInfo.dwFlags & MONITORINFOF_PRIMARY)) {
            // Dies ist ein Sekundaermonitor -> Erstelle ein abdeckendes Fenster

            int x = monitorInfo.rcMonitor.left;
            int y = monitorInfo.rcMonitor.top;
            int width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
            int height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

            // Erstelle ein Fenster, das diesen Monitor abdeckt
            CreateWindowExW(
                WS_EX_TOPMOST,          // Immer im Vordergrund
                BLACKOUT_CLASS_NAME,    // KORREKTUR: Verwende die dedizierte Blackout-Klasse
                NULL,
                WS_POPUP | WS_VISIBLE,  // Randlos und sichtbar
                x, y, width, height,
                hWndMain,               // Hauptfenster als Owner/Parent
                NULL, NULL, NULL
            );
        }
    }
    return TRUE; // Weiter zur Aufzaehlung des naechsten Monitors
}

void CoverSecondaryMonitors(HWND hWndMain) {
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&hWndMain);
}

/**
 * PING-Implementierung
 */
void Ping(const std::wstring& host, HWND hWnd) {
    AddHistory(L"Pinging " + host + L"...");
    UpdateClockDisplay(hWnd);

    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        AddHistory(L"FEHLER: IcmpCreateFile fehlgeschlagen.");
        return;
    }

    // Hostname zu IP-Adresse auflösen
    ADDRINFOW* result = nullptr;
    if (GetAddrInfoW(host.c_str(), NULL, NULL, &result) != 0) {
        AddHistory(L"FEHLER: Host konnte nicht aufgeloest werden.");
        IcmpCloseHandle(hIcmpFile);
        return;
    }

    char sendData[32] = "Data Buffer";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8;
    BYTE* replyBuffer = new BYTE[replySize];

    SOCKADDR_IN* sockaddr_ipv4 = (struct sockaddr_in*)result->ai_addr;
    IPAddr ipaddr = sockaddr_ipv4->sin_addr.S_un.S_addr;

    for (int i = 0; i < 4; i++) {
        DWORD dwRetVal = IcmpSendEcho(hIcmpFile, ipaddr, sendData, sizeof(sendData), NULL, replyBuffer, replySize, 1000);
        if (dwRetVal != 0) {
            PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)replyBuffer;
            std::wstringstream ss;
            // KORREKTUR: Konvertieren der IP-Adresse mit der modernen Funktion InetNtopW
            IN_ADDR addr;
            addr.S_un.S_addr = pEchoReply->Address;
            wchar_t ip_wstr_buffer[INET_ADDRSTRLEN];

            if (InetNtopW(AF_INET, &addr, ip_wstr_buffer, INET_ADDRSTRLEN)) {
                ss << L"Antwort von " << ip_wstr_buffer << L": Zeit=" << pEchoReply->RoundTripTime << L"ms";
                AddHistory(ss.str());
            }
        }
        else {
            AddHistory(L"Zeitueberschreitung der Anforderung.");
        }
        UpdateClockDisplay(hWnd);
        Sleep(1000);
    }

    delete[] replyBuffer;
    FreeAddrInfoW(result);
    IcmpCloseHandle(hIcmpFile);
}

// ... Weitere neue Befehlsfunktionen wie IPCONFIG, TASKLIST, SYSTEMINFO etc. ...

// *** INTELLIGENTE UPDATE-FUNKTIONEN ***

void PerformUpdate(HWND hWnd) {
    AddHistory(L"Update wird heruntergeladen...");
    UpdateClockDisplay(hWnd);

    HINTERNET hInternet = InternetOpen(L"ClockUpdater", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        AddHistory(L"FEHLER: Internetverbindung fehlgeschlagen (InternetOpen).");
        return;
    }

    HINTERNET hUrl = InternetOpenUrlW(hInternet, L"http://wallbangbros.com/clock/time.exe", NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
    if (!hUrl) {
        AddHistory(L"FEHLER: Update-Server nicht erreichbar (InternetOpenUrl).");
        InternetCloseHandle(hInternet);
        return;
    }

    wchar_t localPath[MAX_PATH];
    GetModuleFileNameW(NULL, localPath, MAX_PATH);
    std::wstring tempPath = std::wstring(localPath) + L".tmp";
    std::wstring oldPath = std::wstring(localPath) + L".old";

    HANDLE hFile = CreateFileW(tempPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        AddHistory(L"FEHLER: Temporaere Update-Datei konnte nicht erstellt werden.");
        InternetCloseHandle(hUrl); InternetCloseHandle(hInternet);
        return;
    }

    BYTE buffer[4096];
    DWORD bytesRead = 1;
    BOOL downloadOk = TRUE;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        DWORD bytesWritten;
        if (!WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL) || bytesWritten != bytesRead) {
            AddHistory(L"FEHLER: Fehler beim Schreiben der Update-Datei.");
            downloadOk = FALSE;
            break;
        }
    }
    CloseHandle(hFile);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (!downloadOk) {
        DeleteFileW(tempPath.c_str());
        return;
    }

    DeleteFileW(oldPath.c_str());

    if (!MoveFileW(localPath, oldPath.c_str())) {
        AddHistory(L"FEHLER: Aktuelle Version konnte nicht umbenannt werden.");
        DeleteFileW(tempPath.c_str());
        return;
    }

    if (!MoveFileW(tempPath.c_str(), localPath)) {
        AddHistory(L"FEHLER: Update konnte nicht aktiviert werden. Stelle alte Version wieder her.");
        MoveFileW(oldPath.c_str(), localPath);
        DeleteFileW(tempPath.c_str());
        return;
    }

    AddHistory(L"Update erfolgreich! Die Anwendung wird jetzt neu gestartet...");
    UpdateClockDisplay(hWnd);
    Sleep(2000);

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessW(localPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        DestroyWindow(hWnd);
    }
    else {
        AddHistory(L"FEHLER: Neustart fehlgeschlagen. Bitte manuell neu starten.");
    }
}


void CheckForUpdate(HWND hWnd) {
    AddHistory(L"Suche nach Updates...");
    UpdateClockDisplay(hWnd);

    wchar_t localPath[MAX_PATH];
    GetModuleFileNameW(NULL, localPath, MAX_PATH);
    HANDLE hFile = CreateFileW(localPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        AddHistory(L"FEHLER: Lokale Datei konnte nicht gelesen werden.");
        return;
    }
    DWORD localSize = GetFileSize(hFile, NULL);
    CloseHandle(hFile);

    HINTERNET hInternet = InternetOpen(L"ClockUpdater", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        AddHistory(L"FEHLER: Internetverbindung fehlgeschlagen.");
        return;
    }

    HINTERNET hUrl = InternetOpenUrlW(hInternet, L"http://wallbangbros.com/clock/time.exe", NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
    if (!hUrl) {
        AddHistory(L"FEHLER: Update-Server nicht erreichbar.");
        InternetCloseHandle(hInternet);
        return;
    }

    wchar_t sizeBuffer[32] = { 0 };
    DWORD sizeBufferLen = sizeof(sizeBuffer);
    DWORD remoteSize = 0;

    if (HttpQueryInfoW(hUrl, HTTP_QUERY_CONTENT_LENGTH, sizeBuffer, &sizeBufferLen, NULL)) {
        remoteSize = _wtoi(sizeBuffer);
    }
    else {
        AddHistory(L"FEHLER: Groesse der Update-Datei konnte nicht ermittelt werden.");
        InternetCloseHandle(hUrl); InternetCloseHandle(hInternet);
        return;
    }

    InternetCloseHandle(hUrl); InternetCloseHandle(hInternet);

    if (remoteSize > 0 && remoteSize != localSize) {
        AddHistory(L"Update gefunden!");
        AddHistory(L"Moechten Sie jetzt aktualisieren? (Y/N)");
        g_awaitingUpdateConfirmation = true;
    }
    else {
        AddHistory(L"Ihre Version ist auf dem neuesten Stand.");
    }
}


std::wstring GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    if (localtime_s(&tm_buf, &now_c) != 0) return L"FEHLER";
    std::wstringstream ss;
    ss << std::put_time(&tm_buf, L"%H:%M:%S");
    return ss.str();
}

std::wstring GetCurrentDateString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    if (localtime_s(&tm_buf, &now_c) != 0) return L"FEHLER";
    std::wstringstream ss;
    ss << std::put_time(&tm_buf, L"%d.%m.%Y");
    return ss.str();
}

/**
 * Führt den eingegebenen Befehl aus und aktualisiert den Verlauf.
 */
void ProcessCommand(HWND hWnd, const std::wstring& command) {
    std::wstring trimmedCommand = command;
    size_t end = trimmedCommand.find_last_not_of(L" \t\n\r\f\v");
    if (end != std::wstring::npos) trimmedCommand.resize(end + 1);

    if (g_awaitingUpdateConfirmation) {
        std::wstring upperInput = ToUpper(trimmedCommand);
        AddHistory(PROMPT + trimmedCommand);
        if (upperInput == L"Y") {
            PerformUpdate(hWnd);
        }
        else if (upperInput == L"N") {
            AddHistory(L"Update abgebrochen.");
        }
        else {
            AddHistory(L"Bitte 'Y' oder 'N' eingeben.");
        }
        g_awaitingUpdateConfirmation = false;
        g_inputBuffer.clear(); UpdateClockDisplay(hWnd);
        return;
    }

    AddHistory(PROMPT + trimmedCommand);
    std::wstring upperCommand = ToUpper(trimmedCommand);

    std::wstringstream iss(upperCommand);
    std::wstringstream orig_iss(trimmedCommand);
    std::wstring cmd, arg1, arg2;
    iss >> cmd >> arg1 >> arg2;

    if (cmd == L"HELP") {
        AddHistory(MSG_HELP);
    }
    else if (cmd == L"DATE") {
        AddHistory(L"Aktuelles Datum ist " + GetCurrentDateString());
    }
    else if (cmd == L"TIME") {
        AddHistory(L"Aktuelle Zeit ist " + GetCurrentTimeString());
    }
    else if (cmd == L"CLEAR" || cmd == L"CLS") {
        g_consoleHistory.clear();
        g_scrollOffset = 0;
    }
    else if (cmd == L"UPDATE") {
        CheckForUpdate(hWnd);
    }
    else if (cmd == L"EXIT") {
        g_isTypingEnabled = false;
        g_countdownActive = true;
        AddHistory(L"EXIT.BAT: Terminierung gestartet. Bitte warten...");
        SetTimer(hWnd, COUNTDOWN_TIMER_ID, 1000, NULL);
    }
    else if (cmd == L"PING") {
        if (arg1.empty()) AddHistory(L"FEHLER: Hostname oder IP-Adresse erforderlich.");
        else Ping(arg1, hWnd);
    }
    else if (cmd == L"IPCONFIG") {
        IpConfig(hWnd);
    }
    else if (cmd == L"SYSTEMINFO") {
        SystemInfo(hWnd);
    }
    else if (cmd == L"TASKLIST") {
        TaskList(hWnd);
    }
    else if (cmd == L"NETSTAT") {
        Netstat(hWnd);
    }
    else if (cmd == L"VOL") {
        Vol(hWnd);
    }
    else if (cmd == L"DIR") {
        Dir(hWnd);
    }
    else if (cmd == L"TYPE") {
        if (arg1.empty()) AddHistory(L"FEHLER: Dateiname erforderlich.");
        else Type(arg1, hWnd);
    }
    else if (cmd == L"HOSTNAME") {
        Hostname(hWnd);
    }
    else if (cmd == L"WHOAMI") {
        Whoami(hWnd);
    }
    else if (cmd == L"UPTIME") {
        Uptime(hWnd);
    }
    else if (cmd == L"VER") {
        AddHistory(L"Terminal Clock Version 1.1");
    }
    else if (cmd == L"ECHO") {
        size_t echo_pos = trimmedCommand.find_first_of(L" \t");
        if (echo_pos != std::wstring::npos) {
            AddHistory(trimmedCommand.substr(echo_pos + 1));
        }
    }
    else if (cmd == L"HEX") {
        std::wstring dec_str;
        orig_iss >> cmd >> dec_str;
        if (dec_str.empty()) {
            AddHistory(L"FEHLER: Fehlender Dezimal-Parameter.");
        }
        else {
            try {
                long long dec_val = std::stoll(dec_str);
                std::wstringstream ss;
                ss << std::hex << std::uppercase << dec_val;
                AddHistory(dec_str + L" (DEC) = " + ss.str() + L" (HEX)");
            }
            catch (...) {
                AddHistory(L"FEHLER: Ungueltige Dezimalzahl '" + dec_str + L"'.");
            }
        }
    }
    else if (cmd == L"DEC") {
        std::wstring hex_str;
        orig_iss >> cmd >> hex_str;
        if (hex_str.empty()) {
            AddHistory(L"FEHLER: Fehlender HEX-Parameter.");
        }
        else {
            try {
                long long dec_val;
                std::wstringstream ss;
                ss << std::hex << hex_str;
                ss >> dec_val;
                if (ss.fail()) throw std::runtime_error("Konvertierungsfehler");
                AddHistory(hex_str + L" (HEX) = " + std::to_wstring(dec_val) + L" (DEC)");
            }
            catch (...) {
                AddHistory(L"FEHLER: Ungueltiger HEX-String '" + hex_str + L"'.");
            }
        }
    }
    else {
        try {
            if (cmd == L"SQRT") {
                if (arg1.empty()) throw std::runtime_error("Fehlender Parameter.");
                double n = std::stod(arg1);
                if (n < 0) throw std::runtime_error("Wurzel aus negativer Zahl nicht definiert.");
                AddHistory(L"sqrt(" + arg1 + L") = " + std::to_wstring(sqrt(n)));
            }
            else if (cmd == L"POW") {
                if (arg1.empty() || arg2.empty()) throw std::runtime_error("Zwei Parameter benoetigt.");
                double base = std::stod(arg1);
                double exp = std::stod(arg2);
                AddHistory(arg1 + L"^" + arg2 + L" = " + std::to_wstring(pow(base, exp)));
            }
            else if (cmd == L"LOG") {
                if (arg1.empty()) throw std::runtime_error("Fehlender Parameter.");
                double n = std::stod(arg1);
                if (n <= 0) throw std::runtime_error("Logarithmus nur fuer positive Zahlen definiert.");
                AddHistory(L"ln(" + arg1 + L") = " + std::to_wstring(log(n)));
            }
            else if (cmd == L"LOG10") {
                if (arg1.empty()) throw std::runtime_error("Fehlender Parameter.");
                double n = std::stod(arg1);
                if (n <= 0) throw std::runtime_error("Logarithmus nur fuer positive Zahlen definiert.");
                AddHistory(L"log10(" + arg1 + L") = " + std::to_wstring(log10(n)));
            }
            else if (cmd == L"SIN" || cmd == L"COS" || cmd == L"TAN") {
                if (arg1.empty()) throw std::runtime_error("Fehlender Winkel-Parameter (in Grad).");
                double deg = std::stod(arg1);
                double rad = DegToRad(deg);
                double result;
                if (cmd == L"SIN") result = sin(rad);
                else if (cmd == L"COS") result = cos(rad);
                else result = tan(rad);
                AddHistory(cmd + L"(" + arg1 + L" deg) = " + std::to_wstring(result));
            }
            else if (!trimmedCommand.empty()) {
                AddHistory(L"Ungueltiger Befehl oder Dateiname. Tippen Sie 'HELP'.");
            }
        }
        catch (const std::exception& e) {
            AddHistory(L"FEHLER: " + std::wstring(e.what(), e.what() + strlen(e.what())));
        }
        catch (...) {
            AddHistory(L"FEHLER: Ungueltige numerische Eingabe.");
        }
    }

    g_inputBuffer.clear();
    UpdateClockDisplay(hWnd);
}

BOOL RegisterClockWindowClass(HINSTANCE hInstance) {

    if (!RegisterBlackoutWindowClass(hInstance)) {
        return FALSE;
    }
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = ClockWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    // Hier fügen Sie das Icon hinzu (nachdem Sie die Ressource erstellt haben)
    // wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    // wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    return (RegisterClassExW(&wc) != 0);
}

HWND CreateClockWindow(HINSTANCE hInstance, int nCmdShow) {
    // Initialisiere Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // Fehlerbehandlung, z.B. eine Nachricht anzeigen und beenden
        MessageBoxW(NULL, L"WSAStartup fehlgeschlagen", L"Fehler", MB_OK | MB_ICONERROR);
        return NULL;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    g_consoleHistory.push_back(L"MS-DOS Version 6.22");
    g_consoleHistory.push_back(L"(C)Copyright Microsoft Corporation 1981-1994.");
    g_consoleHistory.push_back(L"Made with \x2764 by HUTAOSHUSBAND");
    g_consoleHistory.push_back(L"");
    g_consoleHistory.push_back(L"Tippen Sie 'HELP' fuer eine Liste der Befehle ein.");
    g_consoleHistory.push_back(L"");

    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST,
        WINDOW_CLASS_NAME,
        L"Windows 3.1 Terminal Clock",
        WS_POPUP | WS_VISIBLE,
        0, 0, screenWidth, screenHeight,
        NULL, NULL, hInstance, NULL
    );

    if (hWnd) {
        g_hFont = CreateFont(
            screenHeight / 45, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            OEM_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            NONANTIALIASED_QUALITY, FF_MODERN | FIXED_PITCH, L"Terminal"
        );
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
        SetTimer(hWnd, CLOCK_TIMER_ID, 50, NULL);
        CoverSecondaryMonitors(hWnd);
    }
    return hWnd;
}

void UpdateClockDisplay(HWND hWnd) {
    InvalidateRect(hWnd, NULL, TRUE);
}

LRESULT CALLBACK ClockWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        break;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_CHAR: {
        if (!g_isTypingEnabled) break;
        if (wParam == VK_BACK) {
            if (!g_inputBuffer.empty()) g_inputBuffer.pop_back();
        }
        else if (wParam == VK_ESCAPE) {
            g_inputBuffer.clear();
        }
        else if (wParam >= 32 && wParam < 255) {
            g_inputBuffer += (wchar_t)wParam;
        }
        UpdateClockDisplay(hWnd);
        break;
    }

    case WM_KEYDOWN: {
        if (wParam == VK_RETURN && g_isTypingEnabled) {
            ProcessCommand(hWnd, g_inputBuffer);
        }
        // NEU: Scroll-Logik
        else if (wParam == VK_PRIOR) { // Page Up
            g_scrollOffset += 10;
            if (!g_consoleHistory.empty() && g_scrollOffset > static_cast<int>(g_consoleHistory.size()) - 1) {
                g_scrollOffset = g_consoleHistory.size() - 1;
            }
            UpdateClockDisplay(hWnd);
        }
        else if (wParam == VK_NEXT) { // Page Down
            g_scrollOffset -= 10;
            if (g_scrollOffset < 0) g_scrollOffset = 0;
            UpdateClockDisplay(hWnd);
        }
        break;
    }

    case WM_SYSCOMMAND:
        if (wParam == SC_CLOSE) {
            if (!g_countdownActive) {
                AddHistory(PROMPT + L"FEHLER: Diese Applikation kann nur durch den 'EXIT'-Befehl beendet werden.");
                g_inputBuffer.clear();
                UpdateClockDisplay(hWnd);
                return 0;
            }
        }
        break;

    case WM_TIMER:
        if (wParam == CLOCK_TIMER_ID) {
            UpdateClockDisplay(hWnd);
        }
        else if (wParam == COUNTDOWN_TIMER_ID) {
            if (g_countdownActive) {
                if (g_countdownSeconds > 0) {
                    g_countdownSeconds--;
                    UpdateClockDisplay(hWnd);
                }
                if (g_countdownSeconds <= 0) {
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

        HBRUSH hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
        FillRect(hdc, &clientRect, hBrush);

        SetTextColor(hdc, RGB(220, 220, 200));
        SetBkMode(hdc, TRANSPARENT);

        TEXTMETRIC tm;
        HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont);
        GetTextMetrics(hdc, &tm);
        int lineHeight = tm.tmHeight + tm.tmExternalLeading;
        int xPadding = 20;

        int maxVisibleLines = clientRect.bottom / lineHeight;

        // NEU: Scroll-Logik beim Zeichnen
        size_t historySize = g_consoleHistory.size();
        int endLine = static_cast<int>(historySize) - g_scrollOffset;
        int startLine = endLine - (maxVisibleLines - 1);
        if (startLine < 0) startLine = 0;


        int currentLineY = lineHeight;

        for (int i = startLine; i < endLine; ++i) {
            if (i < 0 || static_cast<size_t>(i) >= historySize) continue; // KORREKTUR: Typsichere Prüfung
            RECT rect = { xPadding, currentLineY, clientRect.right, currentLineY + lineHeight };
            DrawTextW(hdc, g_consoleHistory[i].c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
            currentLineY += lineHeight;
        }

        if (g_countdownActive) {
            std::wstringstream shutdownSS;
            if (g_countdownSeconds > 0) {
                shutdownSS << L"EXIT.BAT: Terminierung in " << g_countdownSeconds << L" Sekunde(n)...";
            }
            else {
                shutdownSS << L"EXIT.BAT: SYSTEM SHUTDOWN. Goodbye.";
            }
            RECT rect = { xPadding, currentLineY, clientRect.right, currentLineY + lineHeight };
            DrawTextW(hdc, shutdownSS.str().c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
        }
        else if (g_scrollOffset == 0) { // Prompt nur anzeigen, wenn nicht gescrollt wird
            std::wstring promptAndInput = PROMPT + g_inputBuffer;
            if ((GetTickCount64() / 500) % 2 == 0) {
                promptAndInput += L'_';
            }
            else {
                promptAndInput += L' ';
            }
            RECT rect = { xPadding, currentLineY, clientRect.right, currentLineY + lineHeight };
            DrawTextW(hdc, promptAndInput.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
        }

        SelectObject(hdc, hOldFont);
        EndPaint(hWnd, &ps);
        break;
    }

    case WM_DESTROY:
        KillTimer(hWnd, CLOCK_TIMER_ID);
        KillTimer(hWnd, COUNTDOWN_TIMER_ID);
        if (g_hFont) {
            DeleteObject(g_hFont);
            g_hFont = NULL;
        }
        // Winsock-Cleanup
        WSACleanup();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

