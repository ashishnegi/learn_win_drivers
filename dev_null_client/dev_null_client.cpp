// dev_null_client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <Windows.h>

int main()
{
	const wchar_t * devNullSymName = L"\\??\\DevNullSymLink";
	auto devNull = CreateFile(devNullSymName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (INVALID_HANDLE_VALUE == devNull) {
		std::cout << "Failed to open handle to " << devNullSymName << ". Error (" << GetLastError() << ").\n";
		return 1;
	}

	DWORD numRead = 0;
	const DWORD size = rand() % 10000 + 1;
	std::cout << "Trying to read and write " << size << " bytes from /dev/null driver.\n";

	std::vector<char> buf(size, '1');
	
	// Read test
	{
		if (!ReadFile(devNull, buf.data(), size, &numRead, nullptr)) {
			std::cout << "Failed to read handle to " << devNullSymName
				<< ". Bytes to read: " << size << ". Bytes read: " << numRead << " Error(" << GetLastError() << ").\n";
			return 1;
		}

		if (numRead != size) {
			std::cout << "Not able to read " << size << " bytes. Only read " << numRead << " bytes.\n";
			return 1;
		}

		std::vector<char> zeroes(size, 0);
		if (zeroes != buf) {
			std::cout << "Read Buffer is not all zeroes. :(\n";
			return 1;
		}
		else {
			std::cout << "Read " << size << " bytes. All zeroes :)\n";
		}
	}

	// Write test
	{
		DWORD numWritten = 0;
		if (!WriteFile(devNull, buf.data(), size, &numWritten, nullptr)) {
			std::cout << "Failed to write to handle " << devNullSymName
				<< ". Bytes to write: " << size << ". Bytes written: " << numWritten << " Error(" << GetLastError() << ").\n";
			return 1;
		}

		if (numWritten != size) {
			std::cout << "Not able to write " << size << " bytes. Only written" << numRead << " bytes.\n";
			return 1;
		}
		else {
			std::cout << "Written " << size << " bytes.\n";
		}
	}

	std::cout << "Sucess!!!\n";

	return 0;
}
