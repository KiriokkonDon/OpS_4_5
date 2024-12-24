#include "serial_port.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <numeric>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

constexpr char LOG_ALL_FILE[] = "temperature_log_all.txt";
constexpr char LOG_HOURLY_FILE[] = "temperature_log_hourly.txt";
constexpr char LOG_DAILY_FILE[] = "temperature_log_daily.txt";

constexpr int SECONDS_IN_HOUR = 3600;
constexpr int SECONDS_IN_DAY = 86400;

// Append a temperature value to a log file
void append_to_log(const std::string &filename, const std::string &entry) {
    std::ofstream file(filename, std::ios::app);
    if (file) {
        file << entry << std::endl;
    }
}

// Prune log file entries older than a certain threshold
void prune_log(const std::string &filename, std::chrono::hours retention_period) {
    std::ifstream file(filename);
    if (!file) return;

    std::vector<std::string> lines;
    std::string line;
    auto now = std::chrono::system_clock::now();

    while (std::getline(file, line)) {
        auto timestamp_pos = line.find_first_of(' ');
        if (timestamp_pos != std::string::npos) {
            auto timestamp = std::stoll(line.substr(0, timestamp_pos));
            auto log_time = std::chrono::system_clock::time_point{std::chrono::seconds{timestamp}};

            if (now - log_time <= retention_period) {
                lines.push_back(line);
            }
        }
    }

    file.close();
    std::ofstream out_file(filename, std::ios::trunc);
    for (const auto &entry : lines) {
        out_file << entry << std::endl;
    }
}

// Calculate average temperature from a vector of values
std::string calculate_average(const std::vector<double> &temperatures) {
    if (temperatures.empty()) return "N/A";

    double sum = std::accumulate(temperatures.begin(), temperatures.end(), 0.0);
    return std::to_string(sum / temperatures.size());
}

void process_temperature_data(const std::string &port) {
    splib::SerialPort serial_port(port, splib::SerialPort::BAUDRATE_115200);

    if (!serial_port.IsOpen()) {
        std::cerr << "Failed to open port: " << port << std::endl;
        return;
    }

    std::vector<double> hourly_temperatures;
    std::vector<double> daily_temperatures;

    auto last_hour = std::chrono::system_clock::now();
    auto last_day = std::chrono::system_clock::now();

    while (true) {
        std::string temperature_data;
        serial_port >> temperature_data;

        try {
            double temperature = std::stod(temperature_data);
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::system_clock::to_time_t(now);

            append_to_log(LOG_ALL_FILE, std::to_string(timestamp) + " " + std::to_string(temperature));
            hourly_temperatures.push_back(temperature);
            daily_temperatures.push_back(temperature);

            if (now - last_hour >= std::chrono::hours(1)) {
                std::string hourly_avg = calculate_average(hourly_temperatures);
                append_to_log(LOG_HOURLY_FILE, std::to_string(timestamp) + " " + hourly_avg);
                hourly_temperatures.clear();
                last_hour = now;
                prune_log(LOG_ALL_FILE, std::chrono::hours(24));
                prune_log(LOG_HOURLY_FILE, std::chrono::hours(720));
            }

            if (now - last_day >= std::chrono::hours(24)) {
                std::string daily_avg = calculate_average(daily_temperatures);
                append_to_log(LOG_DAILY_FILE, std::to_string(timestamp) + " " + daily_avg);
                daily_temperatures.clear();
                last_day = now;
            }

        } catch (const std::exception &e) {
            std::cerr << "Error processing data: " << e.what() << std::endl;
        }

#ifdef _WIN32
        Sleep(1000);
#else
        usleep(1000000);
#endif
    }
}

int main(int argc, char **argv) {
    std::string port = argc > 1 ? argv[1] : "COM3";

    try {
        process_temperature_data(port);
    } catch (const std::exception &e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
