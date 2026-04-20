#pragma once

#include <api/ichronos.hpp>
#include <api/dto/note_dto.hpp>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>

namespace chronos::cli {

class Formatter {
public:
    static constexpr const char* RESET  = "\033[0m";
    static constexpr const char* BOLD   = "\033[1m";
    static constexpr const char* GREEN  = "\033[32m";
    static constexpr const char* CYAN   = "\033[36m";
    static constexpr const char* GRAY   = "\033[90m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* RED    = "\033[31m";
    static constexpr const char* MAGENTA = "\033[35m";

    static void list(const std::vector<api::NoteDTO>& notes);
    static void success(const std::string& msg);
    static void error(const std::string& msg);
    static void help();
    
    // Добавлена декларация для парсинга времени
    static std::chrono::system_clock::time_point parse_timestamp(const std::string& timestamp_str);

private:
    static int get_terminal_width(int default_width = 80);
    static std::string format_human_readable(const std::string& timestamp_str);
    static std::vector<std::string> word_wrap(const std::string& text, size_t line_length);
};

}