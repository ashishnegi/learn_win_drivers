#include <ntddk.h>

namespace DevNull {
	void DriverUnload(_In_ PDRIVER_OBJECT);
	NTSTATUS DeviceCreateOrClose(_In_ PDEVICE_OBJECT, _Inout_ PIRP);
	NTSTATUS DevNullRead(_In_ PDEVICE_OBJECT, _Inout_ PIRP);
	NTSTATUS DevNullWrite(_In_ PDEVICE_OBJECT, _Inout_ PIRP);

	NTSTATUS CompleteIrp(PIRP, NTSTATUS Status = STATUS_SUCCESS, ULONG_PTR Information = 0);
}

extern "C" {
	NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
		UNREFERENCED_PARAMETER(RegistryPath);

		DriverObject->DriverUnload = DevNull::DriverUnload;
		DriverObject->MajorFunction[IRP_MJ_CREATE] = DevNull::DeviceCreateOrClose;
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = DevNull::DeviceCreateOrClose;
		DriverObject->MajorFunction[IRP_MJ_READ] = DevNull::DevNullRead;
		DriverObject->MajorFunction[IRP_MJ_WRITE] = DevNull::DevNullWrite;

		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\DevNullDevice");
		
		PDEVICE_OBJECT DeviceObject;
		NTSTATUS status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint(("DevNull: Failure (0x%08X): while creating device with name %wZ.\n", status, devName));
			return status;
		}

		// Users can have arbitrary large size of buffers. Use Direct IO to reduce copy overhead.
		DeviceObject->Flags |= DO_DIRECT_IO;

		UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\DevNullSymLink");
		// delete leaked symbolic link -- happened during testing
		IoDeleteSymbolicLink(&symName);

		status = IoCreateSymbolicLink(&symName, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("DevNull: Failure (0x%08X): while creating symbolic name with name %wZ.\n", status, symName));
			IoDeleteDevice(DeviceObject);
			return status;
		}
		
		KdPrint(("DevNull: Successfully created driver with device name : %wZ, symbolic name : %wZ.\n", devName, symName));
		return status;
	}
}

namespace DevNull {
	void DriverUnload(_In_ PDRIVER_OBJECT Driver) {
		KdPrint(("DevNull: Unloading Dev Null Driver.\n"));
		
		UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\DevNullSymLink");
		NTSTATUS status = IoDeleteSymbolicLink(&symName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("DevNull: Failure (0x%08X) while deleting symbolic link %wZ.\n", status, symName));
			// continue cleanup.
		}

		IoDeleteDevice(Driver->DeviceObject);
		KdPrint(("DevNull: Unload finished.\n"));
	}

	NTSTATUS DeviceCreateOrClose(_In_ PDEVICE_OBJECT, _Inout_ PIRP Irp) {
		KdPrint(("DevNull: CreateOrClose finished.\n"));
		return CompleteIrp(Irp);
	}

	NTSTATUS DevNullRead(_In_ PDEVICE_OBJECT, _Inout_ PIRP Irp) {
		auto stack = IoGetCurrentIrpStackLocation(Irp);
		const auto len = stack->Parameters.Read.Length;
		// Get the output buffer.
		if (len == 0) {
			return CompleteIrp(Irp, STATUS_INVALID_BUFFER_SIZE);
		}

		// Write zeroes on it.
		auto buf = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
		if (nullptr == buf) {
			return CompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES);
		}

		memset(buf, 0, len);
		return CompleteIrp(Irp, STATUS_SUCCESS, len);
	}

	NTSTATUS DevNullWrite(_In_ PDEVICE_OBJECT, _Inout_ PIRP Irp) { 
		// don't do anything - just /dev/null :)
		auto stack = IoGetCurrentIrpStackLocation(Irp);
		return CompleteIrp(Irp, STATUS_SUCCESS, stack->Parameters.Write.Length);
	}

	NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS Status, ULONG_PTR Information) {
		Irp->IoStatus.Status = Status;
		Irp->IoStatus.Information = Information;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return Status;
	}
}