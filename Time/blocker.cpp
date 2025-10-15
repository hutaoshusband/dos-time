#include "blocker.h"
#include <winreg.h> // F�r Registry-Funktionen

/**
 * Pfad zum Registry-Schl�ssel, der die Task Manager-Richtlinie enth�lt.
 * HKCU\Software\Microsoft\Windows\CurrentVersion\Policies\System
 */
const wchar_t* const POLICY_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
const wchar_t* const VALUE_NAME = L"DisableTaskMgr";

/**
 * Aktiviert oder deaktiviert die Blockade des Task Managers
 * durch Setzen des DisableTaskMgr-Registry-Werts.
 * * @param enable Wenn TRUE, wird Task Manager deaktiviert (Wert 1). Wenn FALSE, wird er re-aktiviert (Wert 0).
 */
void EnableTaskMgrBlock(BOOL enable) {
    HKEY hKey;
    DWORD dwValue = enable ? 1 : 0; // 1 = Deaktivieren, 0 = Aktivieren

    // 1. �ffnen oder Erstellen des Policy-Schl�ssels
    // Wir verwenden HKEY_CURRENT_USER (HKCU), da dies keine Admin-Rechte erfordert.
    LONG lResult = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        POLICY_KEY,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE, // Wir ben�tigen nur Schreibzugriff
        NULL,
        &hKey,
        NULL
    );

    if (lResult == ERROR_SUCCESS) {
        // 2. Setzen des Registry-Werts
        lResult = RegSetValueExW(
            hKey,
            VALUE_NAME,
            0,
            REG_DWORD, // DWORD ist der Typ f�r den Wert 0 oder 1
            (const BYTE*)&dwValue,
            sizeof(dwValue)
        );

        // 3. Schlie�en des Registry-Schl�ssels
        RegCloseKey(hKey);
    }
}
