#pragma once
#include <windows.h>
#include <string>

// Funktion zur �berpr�fung, ob die App bereits im Autostart ist.
BOOL IsInStartup();

// Funktion zum Hinzuf�gen der App zum Windows Autostart.
BOOL AddToStartup(const std::wstring& appPath);
