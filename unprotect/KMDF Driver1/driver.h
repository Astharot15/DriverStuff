#pragma once
#include <ntifs.h> 
#include <ntstrsafe.h>

constexpr auto TAG = '1GAT';

//typedef struct _EPROCESS* PEPROCESS;

constexpr auto PS_PROTECTION_OFFSET = 0x5fa;
constexpr auto PRIVILEGE_TOKEN_OFFSET = 0x40;

typedef struct _PS_PROTECTION {
    UCHAR Level;
    UCHAR Type : 3;
    UCHAR Audit : 1;
    UCHAR Signer : 4;
} PS_PROTECTION, * PPS_PROTECTION;

typedef struct _SEP_TOKEN_PRIVILEGES
{
    UCHAR Present[8];
    UCHAR Enabled[8];
    UCHAR EnabledByDefault[8];
} SEP_TOKEN_PRIVILEGES, * PSEP_TOKEN_PRIVILEGES;

UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\TestDriver");
UNICODE_STRING symlink = RTL_CONSTANT_STRING(L"\\??\\TestDriver");

NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
void CleanUp(PDRIVER_OBJECT DriverObject);