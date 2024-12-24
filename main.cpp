#include "http_server.hpp"
#include "serial_port.hpp"
#include "general_utils.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <thread>
#include <atomic>

namespace fs = std::filesystem;

struct ParsedData {
    double temp = 0;
    bool is_error = false;
};

constexpr char LOG_DIR[] = "logs";
constexpr char LOG_ALL_NAME[] = "logs/temperature_log_all.log";
constexpr char LOG_HOUR_NAME[] = "logs/temperature_log_hourly.log";
constexpr char LOG_DAY_NAME[] = "logs/temperature_log_daily.log";


void CreateFileIfNotExists(const std::string& name) {
    if (!fs::exists(name)) {
        std::ofstream(name).close();
    }
}

constexpr int64_t HOUR_SEC = 3600;
constexpr int64_t DAY_SEC = HOUR_SEC * 24;
constexpr int64_t MONTH_SEC = DAY_SEC * 30;
constexpr int64_t YEAR_SEC = DAY_SEC * 365;

double GetMeanTemp(const std::string& file_name, int64_t now, int64_t diff_sec) {
    std::ifstream log(file_name);
    if (!log.is_open()) return 0.0;

    double mean = 0;
    uint32_t count = 0;
    std::string line;
    while (std::getline(log, line)) {
        auto split = utillib::Split(line, " ");
        if (split.size() < 2) continue;

        int64_t time = std::stoll(split[0]);
        if (now - time >= diff_sec) break;

        mean += std::stod(split[1]);
        ++count;
    }
    return count > 0 ? mean / count : 0.0;
}

void WriteTempToFile(const std::string& file_name, double temp, int64_t now, int64_t lim_sec) {
    std::string temp_name = LOG_DIR + std::string("/temp.log");
    fs::rename(file_name, temp_name);

    std::ofstream of_log(file_name);
    std::ifstream temp_log(temp_name);
    if (!of_log.is_open() || !temp_log.is_open()) return;

    std::string line;
    while (std::getline(temp_log, line)) {
        auto split = utillib::Split(line, " ");
        if (split.size() < 2) continue;

        int64_t time = std::stoll(split[0]);
        if (now - time < lim_sec) {
            of_log << line << '\n';
        }
    }
    of_log << now << " " << temp << std::endl;

    fs::remove(temp_name);
}

ParsedData ParseTemperature(const std::string& str) {
    auto comp_dollar = [](int ch) -> int { return (std::isspace(ch) || ch == '$') ? 1 : 0; };
    auto str_temp = utillib::Trim(str, comp_dollar);

    try {
        return {std::stod(str_temp), false};
    } catch (...) {
        return {0, true};
    }
}


constexpr char DEFAULT_SERIAL_PORT_NAME[] = "COM4";
constexpr char DEFAULT_SERVER_HOST[] = "127.0.0.1";
constexpr short DEFAULT_SERVER_PORT = 8080;

std::atomic<int64_t> last_hour_gather(0);
std::atomic<int64_t> last_day_gather(0);

void SerialThread(const std::string& serial_port_name) {
    splib::SerialPort serial_port(serial_port_name, splib::SerialPort::BAUDRATE_115200);

    if (!serial_port.IsOpen()) {
        std::cerr << "Failed to open port: " << serial_port_name << std::endl;
        return;
    }

    fs::create_directories(LOG_DIR);
    CreateFileIfNotExists(LOG_ALL_NAME);
    CreateFileIfNotExists(LOG_HOUR_NAME);
    CreateFileIfNotExists(LOG_DAY_NAME);

    last_hour_gather = utillib::GetUNIXTimeNow();
    last_day_gather = utillib::GetUNIXTimeNow();

    std::string input;
    serial_port.SetTimeout(1.0);

    while (true) {
        serial_port >> input;
        auto parsed = ParseTemperature(input);
        if (parsed.is_error) continue;

        int64_t now_time = utillib::GetUNIXTimeNow();
        

        if (now_time - last_hour_gather >= HOUR_SEC) {
            double hour_mean = GetMeanTemp(LOG_ALL_NAME, now_time, HOUR_SEC);
            WriteTempToFile(LOG_HOUR_NAME, hour_mean, now_time, MONTH_SEC);
            last_hour_gather = now_time;
        }

        if (now_time - last_day_gather >= DAY_SEC) {
            double day_mean = GetMeanTemp(LOG_HOUR_NAME, now_time, DAY_SEC);
            WriteTempToFile(LOG_DAY_NAME, day_mean, now_time, YEAR_SEC);
            last_day_gather = now_time;
        }
    }
}

void ServerThread(const std::string& host_ip, short port) {
    srvlib::HTTPServer server(host_ip, port);
    if (!server.IsValid()) {
        std::cerr << "Failed to start server at: http://" << host_ip << ":" << port << std::endl;
        return;
    }

    std::cout << "Server started! Open in your browser: http://" << host_ip << ":" << port << "/" << std::endl;

    std::vector<srvlib::SpecialResponse> resps = {
        {"GET", "/all", []() { return utillib::ReadFile(LOG_ALL_NAME); }},
        {"GET", "/hour", []() { return utillib::ReadFile(LOG_HOUR_NAME); }},
        {"GET", "/day", []() { return utillib::ReadFile(LOG_DAY_NAME); }}
    };
    server.RegisterResponses(resps);

    while (server.IsValid()) {
        server.ProcessClient();
    }
}


int main(int argc, char** argv) {
    std::string serial_port_name = (argc > 1) ? argv[1] : DEFAULT_SERIAL_PORT_NAME;
    std::string host_ip = (argc > 2) ? argv[2] : DEFAULT_SERVER_HOST;
    short server_port = (argc > 3) ? std::stoi(argv[3]) : DEFAULT_SERVER_PORT;

    std::cout << "Starting server at: http://" << host_ip << ":" << server_port << "/" << std::endl;

    std::thread serial_thread(SerialThread, serial_port_name);
    std::thread server_thread(ServerThread, host_ip, server_port);

    serial_thread.join();
    server_thread.join();

    return 0;
}

