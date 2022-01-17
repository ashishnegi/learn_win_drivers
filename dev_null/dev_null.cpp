#include <ntddk.h>

namespace DevNull {
	void DriverUnload(_In_ PDRIVER_OBJECT);
	NTSTATUS DeviceCreateOrClose(_In_ PDEVICE_OBJECT, _Inout_ PIRP);
	NTSTATUS DeviceRead(_In_ PDEVICE_OBJECT, _Inout_ PIRP);
	NTSTATUS DeviceWrite(_In_ PDEVICE_OBJECT, _Inout_ PIRP);
}

extern "C" {
	NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
		UNREFERENCED_PARAMETER(RegistryPath);

		DriverObject->DriverUnload = DevNull::DriverUnload;
		DriverObject->MajorFunction[IRP_MJ_CREATE] = DevNull::DeviceCreateOrClose;
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = DevNull::DeviceCreateOrClose;
		DriverObject->MajorFunction[IRP_MJ_READ] = DevNull::DeviceRead;
		DriverObject->MajorFunction[IRP_MJ_WRITE] = DevNull::DeviceWrite;

		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\DevNullDevice");
		
		PDEVICE_OBJECT DeviceObject;
		NTSTATUS status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint(("DevNull: Failure (0x%08X): while creating device with name %wZ.", status, devName));
			return status;
		}

		UNICODE_STRING symName = RTL_CONSTANT_STRING(L".\\??\\DevNullSymLink");
		status = IoCreateSymbolicLink(&symName, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("DevNull: Failure (0x%08X): while creating symbolic name with name %wZ.", status, symName));
			IoDeleteDevice(DeviceObject);
			return status;
		}
		
		KdPrint(("DevNull: Succssfully created driver with device name : %wZ, symbolic name : %wZ", devName, symName));
		return status;
	}
}

namespace DevNull {
	void DriverUnload(_In_ PDRIVER_OBJECT Driver) {
		KdPrint(("DevNull: Unloading Dev Null Driver"));
		
		UNICODE_STRING symName = RTL_CONSTANT_STRING(L".\\??\\DevNullSymLink");
		NTSTATUS status = IoDeleteSymbolicLink(&symName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("DevNull: Failure (0x%08X) while deleting symbolic link %wZ", status, symName));
			// continue cleanup.
		}

		IoDeleteDevice(Driver->DeviceObject);
		KdPrint(("DevNull: Unload finished"));
	}

	NTSTATUS DeviceCreateOrClose(_In_ PDEVICE_OBJECT, _Inout_ PIRP Irp) {
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		// don't touch Irp now.

		KdPrint(("DevNull: CreateOrClose finished"));
		return STATUS_SUCCESS; 
	}

	NTSTATUS DeviceRead(_In_ PDEVICE_OBJECT, _Inout_ PIRP Irp) {
		// auto stack = IoGetCurrentIrpStackLocation(Irp);
		auto status = STATUS_SUCCESS;

		// Get the output buffer.
		// Write zeroes on it.

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return status;
	}

	NTSTATUS DeviceWrite(_In_ PDEVICE_OBJECT, _Inout_ PIRP Irp) { 
		// don't do anything - just /dev/null :)
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return STATUS_SUCCESS;
	}
}