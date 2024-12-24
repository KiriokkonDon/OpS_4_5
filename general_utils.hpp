#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <memory>
#include <iostream>
#include <stdexcept>

namespace utillib
{
    // Функция ends_with
    bool ends_with(const std::string &str, const std::string &suffix);

    // Разделяет строку str разделителем delim
    std::vector<std::string> Split(const std::string &str, const std::string &delim);

    // Убирает по краям строки любые символы, определённые функцией сравнения. По умолчанию isspace (пробельные, в т.ч. \n)
    std::string Trim(const std::string &in, int (*compare_func)(int) = isspace);

    // Возвращает строку, содержащую текст файла. По умолчанию режим открытия binary
    std::string ReadFile(const std::string &file_path, std::ios::openmode open_mode = std::ios::binary);

    // Строка с текущей датой и временем
    std::string GetTime();

    // Возвращает строку с датой и временем по UNIX времени
    std::string GetTimeFromSec(int64_t unix_sec);

    // Возвращает текущее время в секундах с начала эпохи UNIX
    int64_t GetUNIXTimeNow();

    // Выполняет команду в командной строке и возвращает ответ команды либо "ERROR"
    std::string Exec(const char* cmd);
}

#endif // UTILITY_HPP
