#pragma once
#include <ntifs.h>

#define DBG_PREFIX "GetProcNameeByPidKm: "
#define DbgPrintLine(s, ...) DbgPrint(DBG_PREFIX s "\n", __VA_ARGS__)
