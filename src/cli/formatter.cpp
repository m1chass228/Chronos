#include "formatter.hpp"

// Подключаем все, что нужно для реализаций
#include <api/dto/note_dto.hpp> // Предполагаю, что файл с NoteDTO тут
#include <iostream>
#include <iomanip>
#include <format>
#include <sstream>

namespace chronos::cli {

std::chrono::system_clock::time_point Formatter::parse_timestamp(const std::string& timestamp_str) {
    std::string str_without_frac = timestamp_str.substr(0, 19);
    std::tm tm = {};
    std::stringstream ss(str_without_frac);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        return {};
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

// Реализация format_human_readable
std::string Formatter::format_human_readable(const std::string& timestamp_str) {
    auto event_time = parse_timestamp(timestamp_str);
    if (event_time == std::chrono::system_clock::time_point{}) {
        return timestamp_str;
    }

    const auto now = std::chrono::system_clock::now();
    const auto today_start     = std::chrono::floor<std::chrono::days>(now);
    const auto yesterday_start = today_start - std::chrono::days(1);
    const auto tomorrow_start  = today_start + std::chrono::days(1);

    if (event_time >= today_start && event_time < tomorrow_start) {
        return std::format("Сегодня, {:%H:%M}", event_time);
    }
    if (event_time >= yesterday_start && event_time < today_start) {
        return std::format("Вчера, {:%H:%M}", event_time);
    }

    const auto event_ymd = std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(event_time)};
    const auto now_ymd   = std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(now)};

    if (event_ymd.year() == now_ymd.year()) {
        return std::format("{:%d %b, %H:%M}", event_time);
    }

    return std::format("{:%d %b %Y, %H:%M}", event_time);
}

// Возвращает ширину терминала или значение по умолчанию, если определить не удалось
int Formatter::get_terminal_width(int default_width) {
    // Структура, в которую система запишет размеры окна
    struct winsize size;

    // ioctl - это системный вызов для управления устройствами.
    // Мы "спрашиваем" у стандартного вывода (STDOUT_FILENO) его размер (TIOCGWINSZ).
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0) {
        // Если вызов успешен, возвращаем количество колонок.
        // Проверяем, что оно не нулевое.
        if (size.ws_col > 0) {
            return size.ws_col;
        }
    }

    // Если ioctl не сработал (например, вывод перенаправлен в файл),
    // возвращаем значение по умолчанию.
    return default_width;
}

std::vector<std::string> Formatter::word_wrap(const std::string& text, size_t line_length) {
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string word;

    if (text.empty()) {
        return lines;
    }

    std::string current_line;
    ss >> current_line; // Берем первое слово

    while (ss >> word) {
        if (current_line.length() + word.length() + 1 > line_length) {
            lines.push_back(current_line);
            current_line = word;
        } else {
            current_line += " " + word;
        }
    }
    lines.push_back(current_line); // Добавляем последнюю строку
    return lines;
}

}
