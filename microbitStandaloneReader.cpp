#include <windows.h>
#include <iostream>
#include <vector>

bool InitializeSerialPort(HANDLE& hSerial, const char* portName) {
    hSerial = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening serial port" << std::endl;
        return false;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error getting serial port state" << std::endl;
        CloseHandle(hSerial);
        return false;
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error setting serial port state" << std::endl;
        CloseHandle(hSerial);
        return false;
    }

    return true;
}

bool ReadFromSerialPort(HANDLE hSerial) {
    DWORD dwEventMask;
    const DWORD responseSize = 9; 
    std::vector<char> receivedData; 
    DWORD dwRead;
    OVERLAPPED ov = { 0 };

 
    ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ov.hEvent == NULL) {
        std::cerr << "Error creating overlapped event" << std::endl;
        return false;
    }

    if (!SetCommMask(hSerial, EV_RXCHAR)) {
        std::cerr << "Error setting comm mask" << std::endl;
        CloseHandle(ov.hEvent);
        return false;
    }

    while (true) {
        receivedData.clear(); 
        while (receivedData.size() < responseSize) {
            char szBuf[responseSize];
            OVERLAPPED ovRead = { 0 };
            ovRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (!ReadFile(hSerial, szBuf, responseSize - receivedData.size(), &dwRead, &ovRead)) {
                if (GetLastError() != ERROR_IO_PENDING) {
                    std::cerr << "ReadFile failed immediately" << std::endl;
                    CloseHandle(ovRead.hEvent);
                    continue;
                }
             
                WaitForSingleObject(ovRead.hEvent, INFINITE);
            }

        
            if (!GetOverlappedResult(hSerial, &ovRead, &dwRead, FALSE)) {
                std::cerr << "GetOverlappedResult failed for read operation" << std::endl;
                break;
            }
            else if (dwRead > 0) {
                receivedData.insert(receivedData.end(), szBuf, szBuf + dwRead);
            }

            CloseHandle(ovRead.hEvent);
        }

        if (receivedData.size() == responseSize) {
            std::string receivedString(receivedData.begin(), receivedData.end());
            std::cout << receivedString;
        }
    }

    CloseHandle(ov.hEvent);
    return true;
}


int main() {
    HANDLE hSerial;
    const char* portName = "\\\\.\\COM9"; // Update this to match your COM port

    if (InitializeSerialPort(hSerial, portName)) {
        std::cout << "Serial port initialized. Waiting for data..." << std::endl;
        ReadFromSerialPort(hSerial);
        CloseHandle(hSerial);
    }
    else {
        std::cerr << "Failed to initialize serial port." << std::endl;
    }

    return 0;
}
