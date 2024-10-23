#ifndef COMMON_H
#define COMMON_H

#define IOCTL_GET_PROCESS_NAME CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS)

#define DEVICE_NAME L"\\Device\\GetProcNameByPidKm"
#define SYMLINK_NAME L"\\DosDevices\\GetProcNameByPidKm"

constexpr auto GETPROCNAMEBYPID_DEV_NAME = L"\\\\.\\GetProcNameByPidKm";

#endif // COMMON_H