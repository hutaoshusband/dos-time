#pragma once

#include <windows.h>

/**
 * Aktiviert oder deaktiviert die Blockade des Task Managers
 * durch Setzen des DisableTaskMgr-Registry-Werts.
 * * @param enable Wenn TRUE, wird Task Manager deaktiviert. Wenn FALSE, wird er re-aktiviert.
 */
void EnableTaskMgrBlock(BOOL enable);
