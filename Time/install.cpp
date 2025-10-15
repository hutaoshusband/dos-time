#include "install.h"
#include <winreg.h>
#include <shlwapi.h>

// Registrierungspfad f�r den Benutzer-Autostart
const wchar_t* const RUN_KEY_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
// Name des Eintrags in der Registrierung
const wchar_t* const APP_REG_NAME = L"OldSchoolClock";

/**
 * �berpr�ft, ob die Anwendung bereits in den Autostart-Einstellungen des Benutzers eingetragen ist.
 */
BOOL IsInStartup() {
    HKEY hKey;
    // �ffnet den Run-Schl�ssel mit Lesezugriff
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY_PATH, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return FALSE;
    }

    // Versucht, den Wert abzufragen
    wchar_t value[MAX_PATH];
    DWORD size = sizeof(value);
    result = RegQueryValueExW(hKey, APP_REG_NAME, NULL, NULL, (LPBYTE)value, &size);

    RegCloseKey(hKey);

    return result == ERROR_SUCCESS;
}

/**
 * F�gt den Pfad der Anwendung dem Autostart-Schl�ssel in der Registrierung hinzu.
 */
BOOL AddToStartup(const std::wstring& appPath) {
    HKEY hKey;
    // �ffnet den Run-Schl�ssel mit Schreibzugriff
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY_PATH, 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS) {
        return FALSE;
    }

    result = RegSetValueExW(
        hKey,
        APP_REG_NAME,
        0,
        REG_SZ,
        (const BYTE*)appPath.c_str(),
        (DWORD)(appPath.size() + 1) * sizeof(wchar_t) 
    );

    RegCloseKey(hKey);

    return result == ERROR_SUCCESS;
}
 