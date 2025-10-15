#pragma once
#include <windows.h>
#include <string>

// Funktion zur Überprüfung, ob die App bereits im Autostart ist.
BOOL IsInStartup();

// Funktion zum Hinzufügen der App zum Windows Autostart.
BOOL AddToStartup(const std::wstring& appPath);
