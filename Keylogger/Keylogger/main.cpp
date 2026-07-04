#include "driver.h"

#define SECRET_LENGTH 20
#define MAX_BUFFER_SIZE 64

// These are the pre-calculated FNV-1a hashes for "wh4th3fuck4k3yl0gger"
unsigned int TargetHashes[SECRET_LENGTH] = {
    0x140C74BC, 0x2E9B964D, 0xA6ED9358, 0x13FEE0A4, 0x013B7485,
    0x71986713, 0x04EA73B6, 0x5D1404E0, 0x5483904A, 0x7A1C5EBD,
    0xF2A91BA8, 0x8D365EF7, 0x3F977889, 0xB772DD94, 0x7BD2FF36,
    0x2A27CD07, 0x81A7F13F, 0x386090A7, 0x7503CCEF, 0x30FBB0B4
};

unsigned char KeyBuffer[MAX_BUFFER_SIZE] = { 0 };
int CurrentKeyIndex = 0;
PDEVICE_OBJECT LowerDeviceObject = NULL;

extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = NULL;
    PFILE_OBJECT fileObject = NULL;
    UNICODE_STRING kbdName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");

    for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = DispatchPassThrough;

    DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;

    status = IoCreateDevice(DriverObject, 0, NULL, FILE_DEVICE_KEYBOARD, 0, FALSE, &deviceObject);
    if (!NT_SUCCESS(status)) return status;

    deviceObject->Flags |= (DO_BUFFERED_IO | DO_POWER_PAGABLE);
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    status = IoGetDeviceObjectPointer(&kbdName, FILE_READ_DATA, &fileObject, &LowerDeviceObject);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        return status;
    }

    LowerDeviceObject = IoAttachDeviceToDeviceStack(deviceObject, LowerDeviceObject);
    ObDereferenceObject(fileObject);

    return STATUS_SUCCESS;
}

VOID UnloadWorkerRoutine(PDEVICE_OBJECT DeviceObject, PVOID Context) {
    PIO_WORKITEM workItem = (PIO_WORKITEM)Context;

    if (LowerDeviceObject) {
        IoDetachDevice(LowerDeviceObject);
        LowerDeviceObject = NULL;
    }

    IoFreeWorkItem(workItem);

    IoDeleteDevice(DeviceObject);
}

NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS DispatchPassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDeviceObject, Irp);
}

NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    if (g_Unloading) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(LowerDeviceObject, Irp);
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, OnReadCompletion, DeviceObject, TRUE, TRUE, TRUE);
    return IoCallDriver(LowerDeviceObject, Irp);
}


NTSTATUS OnReadCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context) {
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        PKEYBOARD_INPUT_DATA keys = (PKEYBOARD_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
        ULONG numKeys = (ULONG)(Irp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA));

        for (ULONG i = 0; i < numKeys; i++) {
            // Reset buffer on ESC key
            if (keys[i].MakeCode == 0x01 && !(keys[i].Flags & KEY_BREAK)) {
                CurrentKeyIndex = 0;
                RtlZeroMemory(KeyBuffer, MAX_BUFFER_SIZE);
                continue;
            }

            if (!(keys[i].Flags & KEY_BREAK)) {
                if (CurrentKeyIndex < SECRET_LENGTH) {
                    KeyBuffer[CurrentKeyIndex++] = (unsigned char)keys[i].MakeCode;
                }
                else {
                    RtlMoveMemory(KeyBuffer, KeyBuffer + 1, SECRET_LENGTH - 1);
                    KeyBuffer[SECRET_LENGTH - 1] = (unsigned char)keys[i].MakeCode;
                }

                bool isCorrect = true;
                for (int len = 1; len <= CurrentKeyIndex; len++) {
                    unsigned int currentLenHash = 0x811C9DC5;
                    for (int j = 0; j < len; j++) {
                        currentLenHash = (currentLenHash ^ KeyBuffer[j]) * 16777619;
                    }

                    if (currentLenHash != TargetHashes[len - 1]) {
                        isCorrect = false;
                        break;
                    }
                }

                if (isCorrect && CurrentKeyIndex == SECRET_LENGTH) {
                    DbgPrint("Sequence matched! Initiating self-removal...\n");
                    g_Unloading = TRUE;

                    // Allocate and queue work item
                    PIO_WORKITEM workItem = IoAllocateWorkItem(DeviceObject);
                    if (workItem) {
                        IoQueueWorkItem(workItem, UnloadWorkerRoutine, DelayedWorkQueue, workItem);
                    }
                }
            }
        }
    }

    if (Irp->PendingReturned) IoMarkIrpPending(Irp);
    return Irp->IoStatus.Status;
}