#pragma once

#if defined(_MSC_VER)
#include <crtdbg.h>
#include <cstdlib>
#endif

// Failed assertions in standalone tests must report to the terminal, without
// opening the Microsoft CRT assertion or Windows error-reporting dialogs.
inline void UseConsoleTestDiagnostics() {
#if defined(_MSC_VER)
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
}
