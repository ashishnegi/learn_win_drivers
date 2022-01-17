#include <ntifs.h>
#include <ntddk.h>
#include "thread_booster.h"
#include "common.h"

extern "C" {
	// DriverEntry is called when driver is started using
	// > sc start <threadbooster>
	// Before this you need to register driver with
	// > sc create threadbooster type= kernel binPath= <path_to_driver_file>\thread_booster.sys
	NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
		UNREFERENCED_PARAMETER(RegistryPath);

		// Set functions so that kernel can call us for specific tasks
		DriverObject->DriverUnload = ThreadBooster::ThreadBoosterUnload;
		DriverObject->MajorFunction[IRP_MJ_CREATE] = ThreadBooster::ThreadBoosterCreateClose;
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = ThreadBooster::ThreadBoosterCreateClose;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ThreadBooster::ThreadBoosterDeviceControl;

		// Create a Device Object so that clients can access us.
		UNICODE_STRING deviceName = RTL_CONSTANT_STRING(THREAD_BOOSTER_DEVICE_NAME);
		PDEVICE_OBJECT DeviceObject;

		NTSTATUS status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failure (0x%08X): while creating device with name : %wZ.\n", status, deviceName));
			return status;
		}

		// Create symbolic link so that user mode callers can access us.
		UNICODE_STRING symbolicLinkName = RTL_CONSTANT_STRING(THREAD_BOOSTER_SYMBOLIC_LINK);
		status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failure (0x%08X): while creating symbolic link %wZ for device name %wZ.\n", status, symbolicLinkName, deviceName));
			
			// cleanup in case of failures.
			IoDeleteDevice(DeviceObject);
			return status;
		}

		KdPrint(("Thread booster driver entry called with driver object 0x%08X.\n", DriverObject));
		return STATUS_SUCCESS;
	}

}

namespace ThreadBooster {
	// Unload is called when Driver is stopped using
	// sc stop <threadbooster>
	void ThreadBoosterUnload(_In_ PDRIVER_OBJECT DriverObject) {
		UNREFERENCED_PARAMETER(DriverObject);
		
		// delete the symbolic link we exposed for user mode customers.
		UNICODE_STRING symbolicLinkName = RTL_CONSTANT_STRING(THREAD_BOOSTER_SYMBOLIC_LINK);
		NTSTATUS status = IoDeleteSymbolicLink(&symbolicLinkName);
		if (!NT_SUCCESS(status)) {
			KdPrint(("Failure (0x%08X): while deleting symbolic link %wZ.\n", status, &symbolicLinkName));
			// continue further cleanup.
		}

		// delete the device object for complete cleanup.
		IoDeleteDevice(DriverObject->DeviceObject);
		KdPrint(("Thread booster driver unload called with driver object 0x%08X.\n", DriverObject));
	}

	NTSTATUS ThreadBoosterCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
		UNREFERENCED_PARAMETER(DeviceObject);
		
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;

		// complete the request -- with no temporary priority boost to the caller thread.
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	// Main handler that is called for IoCtl's from user mode.
	NTSTATUS ThreadBoosterDeviceControl(_In_ PDEVICE_OBJECT, _In_ PIRP Irp) {
		auto stack = IoGetCurrentIrpStackLocation(Irp);
		auto status = STATUS_SUCCESS;

		switch (stack->Parameters.DeviceIoControl.IoControlCode) {
		case ThreadBooster::IOCTL_THREAD_BOOSTER_SET_PRIORITY:
		{
			auto inputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
			if (sizeof(ThreadData) != inputLen) {
				status = (inputLen < sizeof(ThreadData)) ? STATUS_BUFFER_TOO_SMALL : STATUS_INVALID_BUFFER_SIZE;
				KdPrint(("Failure (0x%08X) on received request.\n", status));
				break;
			}

			auto data = reinterpret_cast<ThreadData*>(stack->Parameters.DeviceIoControl.Type3InputBuffer);
			if (nullptr == data) {
				status = STATUS_INVALID_PARAMETER;
				KdPrint(("Failure (0x%08X) on received request.\n", status));
				break;
			}

			if (data->Priority < 1 || data->Priority > 31) {
				status = STATUS_INVALID_PARAMETER;
				KdPrint(("Failure (0x%08X) on received request. Priority value should be between 1 to 31.\n", status));
				break;
			}


			PETHREAD thread;
			status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &thread);
			if (!NT_SUCCESS(status)) {
				KdPrint(("Failure (0x%08X) when looking up thread for thread id: 0x%08X.\n", status, data->ThreadId));
				break;
			}

			auto oldPriority = KeSetPriorityThread(thread, data->Priority);
			ObDereferenceObject(thread);

			KdPrint(("Successfully changed priority of Thread : 0x%08X from 0x%08X to 0x%08X.\n", data->ThreadId, oldPriority, data->Priority));
			break;
		}
		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}
}
