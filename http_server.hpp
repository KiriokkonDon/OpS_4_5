#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <unordered_map>

#include "general_utils.hpp"

#ifdef WIN32
#include <winsock2.h> 
#include <ws2tcpip.h> 
#else
#include <sys/socket.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <unistd.h>
#include <netdb.h> 
#include <poll.h>  
#include <signal.h>
#include <string.h> 
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

namespace srvlib
{
#define READ_WAIT_MS 50
#define DEFAULT_HTTP_VERSION "HTTP/1.1"
    namespace fs = std::filesystem;

    std::string GetMimeType(const std::string &filename)
    {
        std::string mimeType = "application/unknown";
        std::string extension = filename.substr(filename.find_last_of('.'));
        
        #ifdef WIN32
        // Получение типа контента из реестра Windows
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, extension.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            char buffer[256];
            DWORD bufferSize = sizeof(buffer);
            if (RegQueryValueEx(hKey, "Content Type", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS)
            {
                mimeType = buffer;
            }
            RegCloseKey(hKey);
        }
        #else
        // Использование команды file для получения типа MIME в Linux
        auto result = utillib::Exec(("file --mime-type -b " + filename).c_str());
        if (result.find("cannot open") == std::string::npos && result.find("ERROR") == std::string::npos)
        {
            mimeType = utillib::Trim(result);
        }
        #endif

        return mimeType;
    }

    std::string FindFile(const std::string &file_path)
    {
        std::string path = file_path;
        if (path.find(".") == std::string::npos)
        {
            path += path.ends_with("/") ? "temperature.html" : "/temperature.html";
        }

        std::vector<std::string> directories = {"../html", "./html", "..", "."};
        if (path.ends_with(".js")) directories = {"../js", "./js", "..", "."};

        for (const auto &dir : directories)
        {
            auto fullPath = dir + path;
            if (fs::exists(fullPath))
            {
                return fullPath;
            }
        }
        return "";
    }

    class SpecialResponse;


    class Request
{
    std::string method;
    std::string url;
    std::string fileUrl;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> urlArgs;

public:
    Request(const std::string &data)
    {
        std::istringstream recv(data);
        std::string line;
        std::getline(recv, line);

        std::vector<std::string> split;
        std::string word;
        std::istringstream iss(line);
        while (iss >> word) {
            split.push_back(word);
        }

        std::string urlPart = split[1];
        size_t pos = urlPart.find('?');
        if (pos != std::string::npos) {
            url = urlPart.substr(0, pos);
            std::string args = urlPart.substr(pos + 1);
            size_t start = 0, end;
            while ((end = args.find('&', start)) != std::string::npos) {
                std::string arg = args.substr(start, end - start);
                size_t equalPos = arg.find('=');
                if (equalPos != std::string::npos) {
                    urlArgs[arg.substr(0, equalPos)] = arg.substr(equalPos + 1);
                }
                start = end + 1;
            }

            std::string arg = args.substr(start);
            size_t equalPos = arg.find('=');
            if (equalPos != std::string::npos) {
                urlArgs[arg.substr(0, equalPos)] = arg.substr(equalPos + 1);
            }
        } else {
            url = urlPart;
        }

        method = split[0];
        version = split[2];


        fileUrl = FindFile(url);


        while (std::getline(recv, line)) {
            if (line == "\r" || line == "\n") {
                break;
            }
            size_t colonPos = line.find(": ");
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 2);
                headers[key] = value;
            }
        }
    }

    bool CheckResponse(SpecialResponse response) const;

    std::string GetMethod() const { return method; }
    std::string GetURL() const { return url; }
    std::string GetFileURL() const { return fileUrl; }
    std::string GetVersion() const { return version; }
    std::string GetHeader(const std::string &key) const { return headers.at(key); }
};

    class Response
{
protected:
    std::string responseType;
    std::string contentType;
    std::string version;

public:
    // Конструктор с тремя параметрами
    Response(const std::string &type, const std::string &content, const std::string &ver)
    {
        responseType = type;
        contentType = content;
        version = ver;
    }

    // Конструктор с двумя параметрами
    Response(const std::string &type, const std::string &content)
        : Response(type, content, "HTTP/1.1") // Используем стандартную версию
    {
    }

    // Конструктор с одним параметром
    Response(const std::string &type)
        : Response(type, "text/plain") // По умолчанию
    {
    }

    // Конструктор по умолчанию
    Response() : Response("200 OK") {}

    // Конструктор на основе запроса
    Response(const Request &request) : Response("200 OK") // Вызываем конструктор по умолчанию
    {
        version = request.GetVersion();
        contentType = GetMimeType(request.GetFileURL()); // Устанавливаем тип контента
    }

    // Установка версии
    void SetVersion(const std::string &ver)
    {
        version = ver;
    }

    // Установка типа ответа
    void SetResponseType(const std::string &type)
    {
        responseType = type;
    }

    // Установка типа контента
    void SetContentType(const std::string &content)
    {
        contentType = content;
    }

    // Формирование ответа
    std::string GetAnswer(const std::string &body) const
    {
        std::stringstream answer; // Создаем строковый поток
        answer << version << " " << responseType << "\r\n"
               << "Content-Type: " << contentType << "\r\n"
               << "Content-Length: " << body.length() << "\r\n\r\n"
               << body; // Добавляем тело ответа
        return answer.str(); // Возвращаем строку
    }
};

    class SpecialResponse : public Response
    {
    protected:
        std::string method;
        std::string url;
        std::string (*bodyFunc)(void);
        bool isRaw;

    public:
        SpecialResponse(const std::string &meth, const std::string &url, std::string (*func)(void), bool raw = false)
            : Response()
        {
            method = meth;
            this->url = url;
            bodyFunc = func;
            isRaw = raw;
        }

        std::string GetBody()
        {
            if (bodyFunc == NULL)
            {
                return "";
            }
            return bodyFunc(); // Получаем тело
        }

        std::string GetAnswer()
        {
            return Response::GetAnswer(GetBody());
        }

        std::string GetMethod() { return method; }
        std::string GetURL() { return url; }
        bool IsRaw() { return isRaw; }
    };

    bool Request::CheckResponse(SpecialResponse response) const
    {
        return response.GetMethod() == GetMethod() && response.GetURL() == GetURL();
    }

    class ErrorResponse : public Response
    {
    public:
        ErrorResponse() : Response("404 Not Found") {}

        std::string GetAnswer() const
        {
            return Response::GetAnswer("<html><body>404 Not Found</body></html>");
        }
    };

    class SocketBase
{
public:
    // Конструктор, инициализирующий сокет
    SocketBase() : m_socket(INVALID_SOCKET)
    {
#ifdef WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
        signal(SIGPIPE, SIG_IGN);
#endif
    }

    // Деструктор, закрывающий сокет
    ~SocketBase()
    {
        CloseSocket();
#if defined(WIN32)
        WSACleanup();
#endif
    }

    // Получаем код последней ошибки
    static int GetErrorCode()
    {
#ifdef WIN32
        return WSAGetLastError();
#else
        return errno;
#endif
    }

    // Проверяем, валиден ли сокет
    bool IsValid() const
    {
        return m_socket != INVALID_SOCKET;
    }

protected:

    void CloseSocket()
    {
        CloseSocket(m_socket);
        m_socket = INVALID_SOCKET; // Устанавливаем в недействительный
    }

    // Статический метод для закрытия сокета
    static void CloseSocket(SOCKET sock)
    {
        if (sock == INVALID_SOCKET) return; // Если недействителен, ничего не делаем

#ifdef WIN32
        shutdown(sock, SD_SEND); // Закрываем отправку для Windows
        closesocket(sock);
#else
        shutdown(sock, SHUT_WR); // Закрываем отправку для Unix
        close(sock);
#endif
    }

    // Проверяем активность сокета
    static int Poll(const SOCKET &socket, int timeout_ms = READ_WAIT_MS)
    {
        struct pollfd pollStruct = {}; // Обнуляем структуру
        pollStruct.fd = socket; // Устанавливаем сокет
        pollStruct.events = POLLIN; // Проверяем на доступные данные

#ifdef WIN32
        return WSAPoll(&pollStruct, 1, timeout_ms);
#else
        return poll(&pollStruct, 1, timeout_ms);
#endif
    }

    SOCKET m_socket;
};


    class HTTPServer : public SocketBase
{
public:
    // Конструктор, который запускает сервер
    HTTPServer(const std::string &ip, const short int port)
    {
        Listen(ip, port);
    }

    // Метод для начала прослушивания
    void Listen(const std::string &ip, const short int port)
    {
        if (m_socket != INVALID_SOCKET)
        {
            CloseSocket(); // Закрываем старый сокет
        }

        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints)); // Обнуляем структуру
        hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_STREAM; // TCP
        hints.ai_protocol = IPPROTO_TCP; // Протокол TCP
        hints.ai_flags = AI_PASSIVE; // Для привязки к адресу

        struct addrinfo *addr = NULL;
        int res = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &addr);

        if (res != 0)
        {
            std::cerr << "Address error: " << res << std::endl;
            freeaddrinfo(addr);
            return;
        }

        m_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (m_socket == INVALID_SOCKET)
        {
            std::cerr << "Socket error: " << GetErrorCode() << std::endl;
            freeaddrinfo(addr);
            return;
        }

        if (bind(m_socket, addr->ai_addr, addr->ai_addrlen) == SOCKET_ERROR)
        {
            std::cerr << "Binding error: " << GetErrorCode() << std::endl;
            freeaddrinfo(addr);
            CloseSocket();
            return;
        }
        freeaddrinfo(addr); // Освобождаем адрес

        if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR)
        {
            std::cerr << "Listening error: " << GetErrorCode() << std::endl;
            CloseSocket();
        }
    }

    // Метод для обработки клиентов
    void ProcessClient()
    {
        if (!IsValid())
        {
            std::cerr << "The socket is not valid." << std::endl;
            return;
        }

        SOCKET client_socket = accept(m_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET)
        {
            std::cerr << "Client error: " << GetErrorCode() << std::endl;
            CloseSocket(client_socket);
            return;
        }

        if (Poll(client_socket) <= 0)
        {
            CloseSocket(client_socket);
            return;
        }

        std::stringstream recv_str;
        int buf_size = sizeof(m_input_buf) - 1;
        int result = -1;

        do
        {
            result = recv(client_socket, m_input_buf, buf_size, 0);
            if (result < 0)
            {
                break; // Если ошибка, выходим
            }
            m_input_buf[result] = '\0'; // Завершаем строку
            recv_str << m_input_buf; // Добавляем в строку
        } while (result >= buf_size);

        if (result <= 0)
        {
            std::cerr << "Error retrieving data: " << GetErrorCode() << std::endl;
            CloseSocket(client_socket);
            return;
        }

        auto request = Request(recv_str.str());
        int index_r = -1;
        for (uint64_t i = 0; i < m_sp_responses.size(); ++i)
        {
            if (request.CheckResponse(m_sp_responses[i]))
            {
                index_r = i; // Находим ответ
                break;
            }
        }

        std::string response;
        if (index_r != -1)
        {
            response = m_sp_responses[index_r].IsRaw() ? 
                m_sp_responses[index_r].GetBody() : 
                m_sp_responses[index_r].GetAnswer();
        }
        else
        {
            std::string path = request.GetFileURL();
            if (!path.empty())
            {
                response = Response(request).GetAnswer(utillib::ReadFile(path));
            }
            else
            {
                response = error_response.GetAnswer();
            }
        }


        result = send(client_socket, response.c_str(), (int)response.length(), 0);
        if (result == SOCKET_ERROR)
        {
            std::cerr << "Sending error: " << GetErrorCode() << std::endl;
        }
        CloseSocket(client_socket);
        std::cout << "Reply sent <3" << std::endl;
    }

    // Метод для регистрации ответов
    void RegisterResponses(std::vector<SpecialResponse> responses)
    {
        m_sp_responses = responses; // Сохраняем ответы
    }

private:
    char m_input_buf[1024]; // Буфер для данных
    std::vector<SpecialResponse> m_sp_responses; // Ответы
    ErrorResponse error_response; // Ответ об ошибке
};

}
