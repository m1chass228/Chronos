#pragma once

#include <api/ichronos.hpp>
#include <api/dto/note_dto.hpp>
#include <api/dto/response.hpp>

#include <sys/ioctl.h>
#include <unistd.h>

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <format>
#include <chrono>
#include <sstream>

namespace chronos::cli {

class Formatter {
public:
    // ANSI Color Codes
    static constexpr const char* RESET  = "\033[0m";
    static constexpr const char* BOLD   = "\033[1m";
    static constexpr const char* GREEN  = "\033[32m";
    static constexpr const char* CYAN   = "\033[36m";
    static constexpr const char* GRAY   = "\033[90m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* RED    = "\033[31m";

    static void list(const std::vector<api::NoteDTO>& notes) {
        if (notes.empty()) {
            std::cout << GRAY << "  (no notes found)" << RESET << std::endl;
            return;
        }

        const int terminal_width = get_terminal_width();

        // Определяем ширину колонок
        const int id_width = 10;
        const int date_width = 22;
        // Оставшееся место под контент и теги
        int remaining_width = terminal_width - id_width - date_width - 3; // 3 разделителя
        if (remaining_width < 20) remaining_width = 20;

        // Печатаем заголовок
        std::cout << BOLD << CYAN
                  << std::left << std::setw(id_width) << " ID"
                  << std::setw(date_width) << "| CREATED AT"
                  << "| NOTE (TAGS)" << RESET << std::endl;
        std::cout << GRAY << std::string(terminal_width, '-') << RESET << std::endl;

        for (const auto& note : notes) {
            // --- Подготовка данных для вывода ---

            // 1. Форматируем ID и дату
            std::string id_str = " " + note.id.substr(0, 8);
            std::string date_str = "| " + format_human_readable(note.created_at);

            // 2. Собираем контент и теги в одну большую строку
            std::string content_and_tags = note.content;
            if (!note.tags.empty()) {
                content_and_tags += " "; // Пробел перед тегами
                for (const auto& tag : note.tags) {
                    std::string clean_tag = tag;
                    // ... (вся твоя логика очистки тегов) ...
                    if (!clean_tag.empty()) {
                        content_and_tags += "#" + clean_tag + " ";
                    }
                }
            }

            // 3. Разбиваем эту большую строку на несколько строк с переносом
            std::vector<std::string> wrapped_lines = word_wrap(content_and_tags, remaining_width - 2); // -2 для "| "

            // --- Вывод ---

            // Печатаем первую строку со всеми колонками
            std::cout << GREEN << std::left << std::setw(id_width) << id_str << RESET;
            std::cout << GRAY << std::left << std::setw(date_width) << date_str << RESET;
            std::cout << BOLD << "| " << (wrapped_lines.empty() ? "" : wrapped_lines[0]) << RESET << std::endl;

            // Если есть еще строки контента, печатаем их с отступом
            if (wrapped_lines.size() > 1) {
                // Вычисляем отступ, равный ширине первых двух колонок
                std::string padding(id_width + date_width, ' ');
                for (size_t i = 1; i < wrapped_lines.size(); ++i) {
                    std::cout << padding << BOLD << "| " << wrapped_lines[i] << RESET << std::endl;
                }
            }

            std::cout << GRAY << std::string(terminal_width, '-') << RESET << std::endl;
        }
    }

    static void success(const std::string& msg) {
        std::cout << GREEN << "✔ " << RESET << msg << std::endl;
    }

    static void error(const std::string& msg) {
        std::cerr << RED << "✘ Error: " << RESET << msg << std::endl;
    }

    static void help() {
        std::cout << BOLD << "Chronos CLI v0.1" << RESET << std::endl;
        std::cout << "Usage: chronos <command> [arguments]" << std::endl << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  add \"content\" [tags...]   Add a new note" << std::endl;
        std::cout << "  ls [tag]                  List all notes (or filter by tag)" << std::endl;
        std::cout << "  rm <id>                   Remove note by ID or Short ID" << std::endl;
        std::cout << "  search <query>            Search notes by content" << std::endl;
    }

private:
    static std::chrono::system_clock::time_point parse_timestamp(const std::string& timestamp_str);
    static std::string format_human_readable(const std::string& timestamp_str);
    static int get_terminal_width(int default_width = 80);
    static std::vector<std::string> word_wrap(const std::string& text, size_t line_length);
};

}
