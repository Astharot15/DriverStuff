#pragma once
#include <ntddk.h>

extern "C" {
    NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
    NTSTATUS DispatchPassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp);
    NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
    NTSTATUS OnReadCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);
    VOID UnloadWorkerRoutine(PDEVICE_OBJECT DeviceObject, PVOID Context);

    extern PDEVICE_OBJECT LowerDeviceObject;
}

typedef struct _KEYBOARD_INPUT_DATA {
    USHORT UnitId;
    USHORT MakeCode;
    USHORT Flags;
    USHORT Reserved;
    ULONG  ExtraInformation;
} KEYBOARD_INPUT_DATA, * PKEYBOARD_INPUT_DATA;

#define KEY_MAKE  0
#define KEY_BREAK 1

BOOLEAN g_Unloading = FALSE;