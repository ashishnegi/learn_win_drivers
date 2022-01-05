#pragma once
#include <ntddk.h>

namespace ThreadBooster {
	void ThreadBoosterUnload(_In_ PDRIVER_OBJECT DriverObject);
	NTSTATUS ThreadBoosterCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
	NTSTATUS ThreadBoosterDeviceControl(_In_ PDEVICE_OBJECT, _In_ PIRP Irp);
}