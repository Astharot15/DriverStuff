#pragma once
#include <ntifs.h> 
#include <ntstrsafe.h>

constexpr auto TAG = '1GAT';

//typedef struct _EPROCESS* PEPROCESS;

constexpr auto PS_PROTECTION_OFFSET = 0x5fa;

typedef struct _PS_PROTECTION {
    UCHAR Level;
    UCHAR Type : 3;
    UCHAR Audit : 1;
    UCHAR Signer : 4;
} PS_PROTECTION, * PPS_PROTECTION;

UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\TestDriver");
UNICODE_STRING symlink = RTL_CONSTANT_STRING(L"\\??\\TestDriver");

NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
void CleanUp(PDRIVER_OBJECT DriverObject);