#include <ntddk.h>

void ThreadBoosterUnload(_In_ PDRIVER_OBJECT DriverObject);

extern "C" {
	NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
		UNREFERENCED_PARAMETER(RegistryPath);

		DriverObject->DriverUnload = ThreadBoosterUnload;

		KdPrint(("Thread booster driver entry called with driver object.\n"));
		return STATUS_SUCCESS;
	}

}

void ThreadBoosterUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);

	KdPrint(("Thread booster driver unload called with driver object.\n"));
}

