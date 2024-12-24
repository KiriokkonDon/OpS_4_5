#pragma once

#if defined(WIN32)
#include <Windows.h>
#define PORT_HANDLE HANDLE
#define INVALID_HANDLE INVALID_HANDLE_VALUE
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define PORT_HANDLE int
#define INVALID_HANDLE -1
#endif

#include <string>
#include <stdexcept>
#include <iostream>

namespace splib {
    class SerialPort {
    public:
        enum BaudRate {
            BAUDRATE_4800 = 4800,
            BAUDRATE_9600 = 9600,
            BAUDRATE_19200 = 19200,
            BAUDRATE_38400 = 38400,
            BAUDRATE_57600 = 57600,
            BAUDRATE_115200 = 115200
        };

        SerialPort() : handle(INVALID_HANDLE) {}

        SerialPort(const std::string& portName, BaudRate baudRate) {
            Open(portName, baudRate);
        }

        int Open(const std::string& portName, BaudRate baudRate) {
            #if defined(WIN32)
            handle = CreateFile(portName.c_str(),
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);
            if (handle == INVALID_HANDLE) {
                throw std::runtime_error("Error opening port: " + std::to_string(GetLastError()));
            }

            DCB dcb;
            GetCommState(handle, &dcb);
            dcb.BaudRate = baudRate;
            dcb.ByteSize = 8;
            dcb.StopBits = ONESTOPBIT;
            dcb.Parity = NOPARITY;
            SetCommState(handle, &dcb);

            // Установка таймаутов
            COMMTIMEOUTS timeouts;
            timeouts.ReadIntervalTimeout = 50;
            timeouts.ReadTotalTimeoutConstant = 50;
            timeouts.ReadTotalTimeoutMultiplier = 10;
            timeouts.WriteTotalTimeoutConstant = 50;
            timeouts.WriteTotalTimeoutMultiplier = 10;
            SetCommTimeouts(handle, &timeouts);

            #else
            handle = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
            if (handle == INVALID_HANDLE) {
                throw std::runtime_error("Error opening port: " + std::string(strerror(errno)));
            }

            struct termios options;
            tcgetattr(handle, &options);
            cfsetispeed(&options, baudRate);
            cfsetospeed(&options, baudRate);
            options.c_cflag |= (CLOCAL | CREAD); // Включить прием
            options.c_cflag &= ~PARENB; // Без четности
            options.c_cflag &= ~CSTOPB; // Один стоп-бит
            options.c_cflag &= ~CSIZE; // Очистить маску битов
            options.c_cflag |= CS8; // 8 бит данных
            tcsetattr(handle, TCSANOW, &options);
            #endif

            return 0; // Успех
        }

        void Close() {
            #if defined(WIN32)
            if (handle != INVALID_HANDLE) {
                CloseHandle(handle);
                handle = INVALID_HANDLE;
            }
            #else
            if (handle != INVALID_HANDLE) {
                close(handle);
                handle = INVALID_HANDLE;
            }
            #endif
        }

        bool IsOpen() const {
            return handle != INVALID_HANDLE;
        }

        int SetTimeout(double timeout) {
            #if defined(WIN32)
            COMMTIMEOUTS timeouts;
            GetCommTimeouts(handle, &timeouts);
            timeouts.ReadTotalTimeoutConstant = static_cast<DWORD>(timeout);
            timeouts.WriteTotalTimeoutConstant = static_cast<DWORD>(timeout);
            SetCommTimeouts(handle, &timeouts);
            #else

            #endif
            return 0; // Успех
        }

        SerialPort& operator>>(std::string& data) {
            if (!IsOpen()) {
                throw std::runtime_error("Port is not open");
            }

            char buffer[256]; // Буфер для чтения данных
            #if defined(WIN32)
            DWORD bytesRead;
            if (ReadFile(handle, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
                buffer[bytesRead] = '\0'; // Нуль-терминируем строку
                data = buffer;
            } else {
                throw std::runtime_error("Error reading from port: " + std::to_string(GetLastError()));
            }
            #else
            ssize_t bytesRead = read(handle, buffer, sizeof(buffer) - 1);
            if (bytesRead >= 0) {
                buffer[bytesRead] = '\0'; // Нуль-терминируем строку
                data = buffer;
            } else {
                throw std::runtime_error("Error reading from port: " + std::string(strerror(errno)));
            }
            #endif

            return *this;
        }

    private:
        PORT_HANDLE handle;
    };
}
