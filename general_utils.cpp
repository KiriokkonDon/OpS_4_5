#include "general_utils.hpp"

namespace utillib
{
    bool ends_with(const std::string &str, const std::string &suffix) {
        if (str.length() >= suffix.length()) {
            return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
        }
        return false;
    }

    std::vector<std::string> Split(const std::string &str, const std::string &delim)
    {
        std::vector<std::string> split;
        size_t start = 0;
        size_t end = str.find(delim);
        while (end != std::string::npos) {
            split.push_back(str.substr(start, end - start));
            start = end + delim.length();
            end = str.find(delim, start);
        }
        split.push_back(str.substr(start)); // Добавляем последний элемент
        return split;
    }

    std::string Trim(const std::string &in, int (*compare_func)(int))
    {
        size_t start = 0;
        while (start < in.length() && compare_func(in[start])) {
            ++start;
        }
        size_t end = in.length();
        while (end > start && compare_func(in[end - 1])) {
            --end;
        }
        return in.substr(start, end - start);
    }

    std::string ReadFile(const std::string &file_path, std::ios::openmode open_mode)
    {
        std::ifstream file(file_path, open_mode);
        if (!file) {
            throw std::runtime_error("Could not open file: " + file_path);
        }
        std::stringstream str;
        str << file.rdbuf();
        return str.str();
    }

    std::string GetTime()
    {
        auto now = std::chrono::system_clock::now();
        time_t t = std::chrono::system_clock::to_time_t(now);
        tm *st = localtime(&t);

        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << (st->tm_year + 1900) << "-"
            << std::setw(2) << std::setfill('0') << (st->tm_mon + 1) << "-"
            << std::setw(2) << std::setfill('0') << st->tm_mday << " "
            << std::setw(2) << std::setfill('0') << st->tm_hour << ":"
            << std::setw(2) << std::setfill('0') << st->tm_min << ":"
            << std::setw(2) << std::setfill('0') << st->tm_sec;

        return oss.str();
    }

    std::string GetTimeFromSec(int64_t unix_sec)
    {
        time_t t = time_t(unix_sec);
        tm *st = localtime(&t);

        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << (st->tm_year + 1900) << "-"
            << std::setw(2) << std::setfill('0') << (st->tm_mon + 1) << "-"
            << std::setw(2) << std::setfill('0') << st->tm_mday << " "
            << std::setw(2) << std::setfill('0') << st->tm_hour << ":"
            << std::setw(2) << std::setfill('0') << st->tm_min << ":"
            << std::setw(2) << std::setfill('0') << st->tm_sec;

        return oss.str();
    }

    int64_t GetUNIXTimeNow()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    std::string Exec(const char* cmd) {
        std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
        if (!pipe) {
            return "ERROR";
        }
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
            result += buffer;
        }
        return result;
    }
}
