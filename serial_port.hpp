#pragma once

#if defined(WIN32)
#include <Windows.h>
typedef HANDLE MyPortHandle;
typedef DCB MyPortSettings;
#define MY_INVALID_HANDLE INVALID_HANDLE_VALUE
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstdint>
typedef int32_t MyPortHandle;
typedef termios MyPortSettings;
#define MY_INVALID_HANDLE -1
#endif

#include <string>
#include <cstring>
#include <cstdint>

#define MY_PORT_READ_BUF 1500
#define MY_PORT_WRITE_BUF 1500
#define SERIAL_PORT_DEFAULT_TIMEOUT 1.0

namespace splib {
    class SerialPort {
    public:
        enum BaudRate {
#if defined(WIN32)
            BAUDRATE_4800 = CBR_4800,
            BAUDRATE_9600 = CBR_9600,
            BAUDRATE_19200 = CBR_19200,
            BAUDRATE_38400 = CBR_38400,
            BAUDRATE_57600 = CBR_57600,
            BAUDRATE_115200 = CBR_115200,
#else
            BAUDRATE_4800 = B4800,
            BAUDRATE_9600 = B9600,
            BAUDRATE_19200 = B19200,
            BAUDRATE_38400 = B38400,
            BAUDRATE_57600 = B57600,
            BAUDRATE_115200 = B115200,
#endif
            BAUDRATE_INVALID = -1
        };

        // Четность
        enum Parity {
#if defined(WIN32)
            COM_PARITY_NONE = NOPARITY,
            COM_PARITY_ODD = ODDPARITY,
            COM_PARITY_EVEN = EVENPARITY,
#else
            COM_PARITY_NONE,
            COM_PARITY_ODD,
            COM_PARITY_EVEN,
#endif
        };

        // Стоповые биты
        enum StopBits {
#if defined(WIN32)
            STOPBIT_ONE = ONESTOPBIT,
            STOPBIT_TWO = TWOSTOPBITS
#else
            STOPBIT_ONE,
            STOPBIT_TWO
#endif
        };

        // Управление потоком
        enum FlowControl {
            CONTROL_NONE = 0,
#if defined(WIN32)
            CONTROL_HARDWARE_RTS_CTS = 0x01,
            CONTROL_HARDWARE_DSR_DTR = 0x02,
#endif
            CONTROL_SOFTWARE_XON_IN = 0x04,
            CONTROL_SOFTWARE_XON_OUT = 0x08
        };

        // Коды ошибок
        enum ErrorCodes {
            RE_OK = 0,
            RE_PORT_CONNECTED,
            RE_PORT_CONNECTION_FAILED,
            RE_PORT_INVALID_SETTINGS,
            RE_PORT_PARAMETERS_SET_FAILED,
            RE_PORT_PARAMETERS_GET_FAILED,
            RE_PORT_NOT_CONNECTED,
            RE_PORT_SYSTEM_ERROR,
            RE_PORT_WRITE_FAILED,
            RE_PORT_READ_FAILED
        };

        // Параметры серийного порта
        struct Parameters {
            BaudRate baud_rate;
            StopBits stop_bits;
            Parity parity;
            int controls;
            unsigned char data_bits;
            double timeout;
            size_t read_buffer_size;
            size_t write_buffer_size;
            unsigned char on_char;
            unsigned char off_char;
            int xon_lim;
            int xoff_lim;

            Parameters(BaudRate speed = BAUDRATE_INVALID) {
                Defaults();
                if (speed != BAUDRATE_INVALID)
                    baud_rate = speed;
            }

            void Defaults() {
                baud_rate = BAUDRATE_115200;
                stop_bits = STOPBIT_ONE;
                parity = COM_PARITY_NONE;
                controls = CONTROL_NONE;
                data_bits = 8;
                timeout = 0.0;
                read_buffer_size = MY_PORT_READ_BUF;
                write_buffer_size = MY_PORT_WRITE_BUF;
                on_char = 0;
                off_char = (unsigned char)0xFF;
                xon_lim = 128;
                xoff_lim = 128;
            }

            bool IsValid() const {
                return (baud_rate != BAUDRATE_INVALID);
            }
        };

    private:
        MyPortHandle _phandle;
        std::string _port_name;
        double _timeout;

        // Открыть порт
        int CreatePortHandle(const std::string& name) {
#if defined(WIN32)
            std::string sys_name = "\\\\.\\" + name;
            _phandle = CreateFileA(sys_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (_phandle == MY_INVALID_HANDLE)
                return RE_PORT_CONNECTION_FAILED;
#else
            _phandle = ::open(name.c_str(), O_RDWR | O_NOCTTY);
            if (_phandle < 0) {
                _phandle = MY_INVALID_HANDLE;
                return RE_PORT_CONNECTION_FAILED;
            }
#endif
            return RE_OK;
        }

        // Закрыть порт
        int ClosePortHandle() {
            int ret = RE_OK;
#if defined(WIN32)
            if (!::CloseHandle(_phandle))
                ret = RE_PORT_SYSTEM_ERROR;
#else
            tcflush(_phandle, TCIOFLUSH);
            if (::close(_phandle) < 0)
                ret = RE_PORT_SYSTEM_ERROR;
#endif
            _phandle = MY_INVALID_HANDLE;
            return ret;
        }

        // Установить параметры
        int SetParameters(const Parameters& inp_params) {
            if (!IsOpen())
                return RE_PORT_NOT_CONNECTED;

            MyPortSettings setts;
            memset(&setts, 0, sizeof(setts));

            if (!inp_params.IsValid())
                return RE_PORT_INVALID_SETTINGS;

#if defined(WIN32)
            setts.DCBlength = sizeof(DCB);
            GetCommState(_phandle, &setts);
            setts.fBinary = TRUE;
            setts.BaudRate = DWORD(inp_params.baud_rate);
            setts.ByteSize = BYTE(inp_params.data_bits);
            setts.Parity = BYTE(inp_params.parity);
            setts.StopBits = BYTE(inp_params.stop_bits);
            setts.fAbortOnError = FALSE;

            if (inp_params.controls & CONTROL_HARDWARE_RTS_CTS) {
                setts.fOutxCtsFlow = TRUE;
                setts.fRtsControl = RTS_CONTROL_ENABLE;
            } else {
                setts.fOutxCtsFlow = FALSE;
                setts.fRtsControl = RTS_CONTROL_DISABLE;
            }

            if (inp_params.controls & CONTROL_SOFTWARE_XON_IN)
                setts.fInX = TRUE;
            else
                setts.fInX = FALSE;

            if (inp_params.controls & CONTROL_SOFTWARE_XON_OUT)
                setts.fOutX = TRUE;
            else
                setts.fOutX = FALSE;

            setts.XonChar = inp_params.on_char;
            setts.XoffChar = inp_params.off_char;
            setts.XonLim = inp_params.xon_lim;
            setts.XoffLim = inp_params.xoff_lim;

            if (!SetCommState(_phandle, &setts))
                return RE_PORT_PARAMETERS_SET_FAILED;
#else
            if (tcgetattr(_phandle, &setts) != 0)
                return RE_PORT_PARAMETERS_SET_FAILED;

            cfsetispeed(&setts, inp_params.baud_rate);
            cfsetospeed(&setts, inp_params.baud_rate);
            setts.c_cflag &= ~CSIZE;
            setts.c_cflag |= (inp_params.data_bits == 5 ? CS5 :
                              inp_params.data_bits == 6 ? CS6 :
                              inp_params.data_bits == 7 ? CS7 : CS8);
            if (inp_params.parity == COM_PARITY_NONE)
                setts.c_cflag &= ~PARENB;
            else
                setts.c_cflag |= PARENB;

            if (inp_params.stop_bits == STOPBIT_TWO)
                setts.c_cflag |= CSTOPB;
            else
                setts.c_cflag &= ~CSTOPB;

            if (tcsetattr(_phandle, TCSANOW, &setts))
                return RE_PORT_PARAMETERS_SET_FAILED;
#endif

            _timeout = inp_params.timeout;
            return RE_OK;
        }

    public:
        // Конструктор
        SerialPort() : _phandle(MY_INVALID_HANDLE), _timeout(0.0) {}

        SerialPort(const std::string& name, BaudRate speed) : _phandle(MY_INVALID_HANDLE), _timeout(0.0) {
            Open(name, Parameters(speed));
        }

        // Деструктор
        virtual ~SerialPort() {
            if (IsOpen())
                Close();
        }

        // Открыть серийный порт
        int Open(const std::string& port_name, const Parameters& params) {
            if (IsOpen())
                return RE_PORT_CONNECTED;

            int ret = CreatePortHandle(port_name);
            if (ret != RE_OK)
                return ret;

            _port_name = port_name;
            ret = SetParameters(params);
            if (ret != RE_OK)
                Close();

            return ret;
        }

        // Закрыть серийный порт
        int Close() {
            if (!IsOpen())
                return RE_PORT_NOT_CONNECTED;

            int ret = ClosePortHandle();
            _timeout = 0.0;
            _port_name.clear();
            return ret;
        }

        // Проверить, открыт ли порт
        bool IsOpen() const {
            return (_phandle != MY_INVALID_HANDLE);
        }

        // Получить имя порта
        const std::string& GetPortName() {
            return _port_name;
        }

        // Получить таймаут
        double GetTimeout() {
            return _timeout;
        }

        // Установить таймаут
        int SetTimeout(double timeout) {
            if (!IsOpen())
                return RE_PORT_NOT_CONNECTED;

            int ret = RE_OK;
            int tmms = (int32_t)(timeout * 1e3);

#if defined(WIN32)
            COMMTIMEOUTS tmts;
            if (!GetCommTimeouts(_phandle, &tmts))
                return RE_PORT_PARAMETERS_GET_FAILED;

            tmts.ReadIntervalTimeout = (tmms > 0) ? 0 : MAXDWORD;
            tmts.ReadTotalTimeoutConstant = tmms;
            tmts.WriteTotalTimeoutConstant = tmms;

            if (!SetCommTimeouts(_phandle, &tmts))
                return RE_PORT_PARAMETERS_SET_FAILED;
#else
            MyPortSettings params;
            if (tcgetattr(_phandle, &params))
                return RE_PORT_PARAMETERS_GET_FAILED;

            params.c_cc[VMIN] = 0;
            params.c_cc[VTIME] = (tmms + 99) / 100;

            if (tcsetattr(_phandle, TCSANOW, &params))
                return RE_PORT_PARAMETERS_SET_FAILED;
#endif

            if (ret == RE_OK)
                _timeout = timeout;

            return ret;
        }

        // Запись в порт
        int Write(const void* buf, size_t buf_size, size_t* written = NULL) {
            if (!IsOpen())
                return RE_PORT_NOT_CONNECTED;

            if (written)
                *written = 0;

#if defined(WIN32)
            DWORD feedback = 0;
            if (!::WriteFile(_phandle, (LPCVOID)buf, (DWORD)buf_size, &feedback, NULL))
                return RE_PORT_WRITE_FAILED;

            if (written)
                *written = feedback;
#else
            ssize_t result = write(_phandle, buf, buf_size);
            if (result < 0)
                return RE_PORT_WRITE_FAILED;

            if (written)
                *written = result;
#endif

            return RE_OK;
        }

        // Запись строки в порт
        int Write(const std::string& data) {
            size_t wrt = 0;
            int ret = RE_OK;

            for (size_t i = 0; i < data.size(); i += wrt) {
                ret = Write(data.c_str() + i, data.size() - i, &wrt);
                if (ret != RE_OK)
                    break;
            }

            return ret;
        }

        // Чтение из порта
        int Read(void* buf, size_t max_size, size_t* readd) {
            if (!IsOpen())
                return RE_PORT_NOT_CONNECTED;

            *readd = 0;

#if defined(WIN32)
            DWORD feedback = 0;
            if (!::ReadFile(_phandle, buf, (DWORD)max_size, &feedback, NULL))
                return RE_PORT_READ_FAILED;

            *readd = (size_t)feedback;
#else
            ssize_t result = read(_phandle, buf, max_size);
            if (result < 0)
                return RE_PORT_READ_FAILED;

            *readd = (size_t)result;
#endif

            return RE_OK;
        }

        // Чтение строки из порта
        int Read(std::string& str, double timeout = SERIAL_PORT_DEFAULT_TIMEOUT) {
            str.clear();
            str.reserve(255);
            size_t rd = 0;

            int ret = Read((void*)str.c_str(), str.capacity(), &rd);
            if (ret != RE_OK)
                return ret;

            str.resize(rd);
            return ret;
        }

        // Очистка буферов
        int Flush() {
            if (!IsOpen())
                return RE_PORT_NOT_CONNECTED;

#if defined(WIN32)
            if (!PurgeComm(_phandle, PURGE_TXCLEAR | PURGE_RXCLEAR))
                return RE_PORT_SYSTEM_ERROR;
#else
            tcflush(_phandle, TCIOFLUSH);
#endif

            return RE_OK;
        }

        // Операторы для удобного чтения/записи строк
        SerialPort& operator<<(const std::string& data) {
            Write(data);
            return *this;
        }

        SerialPort& operator>>(std::string& data) {
            Read(data);
            return *this;
        }
    };
}
