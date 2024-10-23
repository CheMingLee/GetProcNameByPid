#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "common.h"

static std::vector<DWORD> parsePIDs(const std::string& input) {
    std::vector<DWORD> pids;
    std::stringstream ss(input);
    std::string temp;

    while (std::getline(ss, temp, ' ')) {
        try {
            pids.push_back(std::stoul(temp));
        }
        catch (...) {
            std::cerr << "Invalid PID: " << temp << std::endl;
        }
    }

    return pids;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <pid1> <pid2> ... <pidN>" << std::endl;
        return 1;
    }

    std::vector<DWORD> pids;
    for (int i = 1; i < argc; ++i) {
        try {
            pids.push_back(std::stoul(argv[i]));
        }
        catch (...) {
            std::cerr << "Invalid PID: " << argv[i] << std::endl;
            return 1;
        }
    }

    HANDLE hDevice = CreateFileW(GETPROCNAMEBYPID_DEV_NAME,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open device. Error: " << GetLastError() << std::endl;
        return 1;
    }

    for (DWORD pid : pids) {
        char processName[32] = { 0 };
        DWORD bytesReturned;

        BOOL result = DeviceIoControl(hDevice,
                                      IOCTL_GET_PROCESS_NAME,
                                      &pid,               // InputBuffer (PID)
                                      sizeof(pid),        // InputBufferSize
                                      processName,        // OutputBuffer
                                      sizeof(processName),// OutputBufferSize
                                      &bytesReturned,
                                      NULL);

        if (!result) {
            std::cerr << "DeviceIoControl failed for PID " << pid << ". Error: " << GetLastError() << std::endl;
        } else {
            std::cout << "PID: " << pid << " - Process Name: " << processName << std::endl;
        }
    }

    CloseHandle(hDevice);
    return 0;
}