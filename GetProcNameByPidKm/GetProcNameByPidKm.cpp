#include "GetProcNameByPidKm.h"
#include "common.h"

/*************************************************************************
    Structures
*************************************************************************/

typedef UCHAR* (*PFNPSGETPROCESSIMAGEFILENAME)(__in PEPROCESS Process);

/*************************************************************************
    Global Variables
*************************************************************************/

static PFNPSGETPROCESSIMAGEFILENAME pPsGetProcessImageFileName = nullptr;

/*************************************************************************
    Prototypes
*************************************************************************/

EXTERN_C_START
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
void UnloadDriver(PDRIVER_OBJECT DriverObject);
NTSTATUS DefaultDispatchHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DeviceControlHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp);
EXTERN_C_END
NTSTATUS GetProcessNameByPid(ULONG pid, UCHAR* outBuffer, ULONG outBufferSize);

/*************************************************************************
    Routines
*************************************************************************/

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    DbgPrintLine("DriverEntry called");
    
    UNREFERENCED_PARAMETER(RegistryPath);

    UNICODE_STRING uniFunctionName = RTL_CONSTANT_STRING(L"PsGetProcessImageFileName");
    PDEVICE_OBJECT DeviceObject = NULL;
    UNICODE_STRING deviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
    UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMLINK_NAME);

    pPsGetProcessImageFileName = (PFNPSGETPROCESSIMAGEFILENAME)MmGetSystemRoutineAddress(&uniFunctionName);
    if (pPsGetProcessImageFileName == nullptr) {
        DbgPrintLine("Failed to get address of PsGetProcessImageFileName");
        return STATUS_UNSUCCESSFUL;
    }

    NTSTATUS status = IoCreateDevice(DriverObject,
                                     0,
                                     &deviceName,
                                     FILE_DEVICE_UNKNOWN,
                                     FILE_DEVICE_SECURE_OPEN,
                                     FALSE,
                                     &DeviceObject);

    if (!NT_SUCCESS(status)) {
        DbgPrintLine("Failed to create device. Error: 0x%X", status);
        return status;
    }

    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrintLine("Failed to create symbolic link. Error: 0x%X", status);
        IoDeleteDevice(DeviceObject);
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = 
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DefaultDispatchHandler;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControlHandler;
    DriverObject->DriverUnload = UnloadDriver;
    
    DbgPrintLine("Driver loaded successfully");
    return STATUS_SUCCESS;
}

void UnloadDriver(PDRIVER_OBJECT DriverObject)
{
    DbgPrintLine("UnloadDriver called");

    UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMLINK_NAME);
    IoDeleteSymbolicLink(&symLink);
    IoDeleteDevice(DriverObject->DeviceObject);

    DbgPrintLine("Driver unloaded successfully");
}

NTSTATUS DefaultDispatchHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DbgPrintLine("DefaultDispatchHandler called");

    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DbgPrintLine("DefaultDispatchHandler completed");
    return STATUS_SUCCESS;
}

NTSTATUS DeviceControlHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DbgPrintLine("DeviceControlHandler called");
    
    UNREFERENCED_PARAMETER(DeviceObject);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID inputBuffer = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    PVOID outputBuffer = Irp->UserBuffer;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_GET_PROCESS_NAME:
            if (inputBufferLength >= sizeof(ULONG) && outputBufferLength >= 32) {
                __try {
                    ProbeForRead(inputBuffer, inputBufferLength, sizeof(ULONG));
                    ULONG pid = *(ULONG*)inputBuffer;

                    ProbeForWrite(outputBuffer, outputBufferLength, sizeof(CHAR));
                    UCHAR processName[16] = { 0 };
                    status = GetProcessNameByPid(pid, processName, sizeof(processName));
                    if (NT_SUCCESS(status)) {
                        RtlCopyMemory(outputBuffer, processName, sizeof(processName));
                        DbgPrintLine("Process name in outputBuffer for PID %u: %s", pid, (PCHAR)outputBuffer);
                    }
                    else {
                        DbgPrintLine("Failed to get process name for PID %u. Error: 0x%X", pid, status);
                    }
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    DbgPrintLine("Exception while processing IOCTL_GET_PROCESS_NAME");
                    status = GetExceptionCode();
                }
            } else {
                DbgPrintLine("Invalid buffer sizes. Input: %u, Output: %u", inputBufferLength, outputBufferLength);
                status = STATUS_INVALID_PARAMETER;
            }
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = (status == STATUS_SUCCESS) ? outputBufferLength : 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DbgPrintLine("DeviceControlHandler completed with status 0x%X", status);
    return status;
}

NTSTATUS GetProcessNameByPid(ULONG pid, UCHAR* outBuffer, ULONG outBufferSize)
{
    DbgPrintLine("GetProcessNameByPid called for PID %u", pid);
    
    PEPROCESS eProcess;
    
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)pid, &eProcess);
    if (!NT_SUCCESS(status)) {
        DbgPrintLine("Failed to lookup process by PID %u. Error: 0x%X", pid, status);
        return status;
    }

    //+0x5a8 ImageFileName : [15] UChar
    if (outBufferSize < 16) {
        DbgPrintLine("Buffer too small to store process name");
        ObDereferenceObject(eProcess);
        return STATUS_BUFFER_TOO_SMALL;
    }
    UCHAR* ImageFileName;
    ImageFileName = pPsGetProcessImageFileName(eProcess);
    DbgPrintLine("ImageFileName from PsGetProcessImageFileName: %s", ImageFileName);

    RtlCopyMemory(outBuffer, ImageFileName, 15);

    ObDereferenceObject(eProcess);

    DbgPrintLine("Process name for PID %u: %s", pid, outBuffer);
    return status;
}