#pragma once

// Common file for clients and kernel driver
namespace ThreadBooster {
	struct ThreadData {
		ULONG ThreadId;
		int Priority;
	};

	const int DeviceCode = 0x8000;

	#define THREAD_BOOSTER_DEVICE_NAME L"\\Device\\ThreadBoosterDevice"
	#define THREAD_BOOSTER_SYMBOLIC_LINK L"\\??\\ThreadBoosterSymLink"
	#define THREAD_BOOSTER_CLIENT_SYMBOLIC_LINK L"\\\\.\\ThreadBoosterSymLink"
	const auto IOCTL_THREAD_BOOSTER_SET_PRIORITY = CTL_CODE(ThreadBooster::DeviceCode, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS);
}
