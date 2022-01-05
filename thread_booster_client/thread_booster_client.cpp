// thread_booster_client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <iostream>
#include "..\thread_booster\common.h"
#include <string>

int main(int argc, const char* argv[])
{
    if (argc < 3) {
        std::cout << "Usage:\nthread_booster_client.exe <ThreadId> <Priority>\n";
        return 1;
    }

    // open a handle to the driver.
    auto hDevice = CreateFile(THREAD_BOOSTER_CLIENT_SYMBOLIC_LINK, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (INVALID_HANDLE_VALUE == hDevice) {
        std::cout << "Failure " << GetLastError() << " to open driver symbolic link : " << THREAD_BOOSTER_CLIENT_SYMBOLIC_LINK << "\n";
        return 1;
    }
    
    // create data to send to driver
    ThreadBooster::ThreadData data;
    data.ThreadId = std::stoi(argv[1]);
    data.Priority = std::stoi(argv[2]);

    DWORD returned = 0;
    // make call to driver
    BOOL success = DeviceIoControl(hDevice, ThreadBooster::IOCTL_THREAD_BOOSTER_SET_PRIORITY, &data, sizeof(data), nullptr, 0, &returned, nullptr);
    if (!success) {
        std::cout << "Failure " << GetLastError() << " while setting the thread priority for threadid: " << data.ThreadId << " and priority: " << data.Priority << "\n";
        return 1;
    }

    std::cout << "Successfully set thread priority!!!\n";
    return 0;
}
