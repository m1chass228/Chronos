#include "formatter.hpp"
#include <iostream>
#include <iomanip>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sstream>
#include <ctime>

namespace chronos::cli {

void Formatter::list(const std::vector<api::NoteDTO>& notes) {
    if (notes.empty()) {
        std::cout << GRAY << "  (no notes found)" << RESET << std::endl;
        return;
    }

    const int terminal_width = get_terminal_width();
    const int id_width = 10;
    const int date_width = 22;
    int remaining_width = terminal_width - id_width - date_width - 3;
    if (remaining_width < 20) remaining_width = 20;

    std::cout << BOLD << CYAN
              << std::left << std::setw(id_width) << " ID"
              << std::setw(date_width) << "| CREATED AT"
              << "| NOTE, TAGS & ATTACHMENTS" << RESET << std::endl;
    std::cout << GRAY << std::string(terminal_width, '-') << RESET << std::endl;

    for (const auto& note : notes) {
        std::string id_str = " " + note.id.substr(0, 8);
        std::string date_str = "| " + format_human_readable(note.created_at);
        std::vector<std::string> wrapped_lines = word_wrap(note.content, remaining_width - 2);

        std::cout << GREEN << std::left << std::setw(id_width) << id_str << RESET;
        std::cout << GRAY << std::left << std::setw(date_width) << date_str << RESET;
        std::cout << BOLD << "| " << (wrapped_lines.empty() ? "" : wrapped_lines[0]) << RESET << std::endl;

        std::string padding(id_width + date_width, ' ');
        for (size_t i = 1; i < wrapped_lines.size(); ++i) {
            std::cout << padding << BOLD << "| " << wrapped_lines[i] << RESET << std::endl;
        }

        if (!note.tags.empty()) {
            std::cout << padding << GRAY << "| " << CYAN;
            for (size_t i = 0; i < note.tags.size(); ++i) {
                std::cout << "#" << note.tags[i] << (i < note.tags.size() - 1 ? ", " : "");
            }
            std::cout << RESET << std::endl;
        }

        if (!note.attachments.empty()) {
            std::cout << padding << GRAY << "| " << MAGENTA << "📎 " ;
            for (size_t i = 0; i < note.attachments.size(); ++i) {
                std::cout << note.attachments[i] << (i < note.attachments.size() - 1 ? "; " : "");
            }
            std::cout << RESET << std::endl;
        }
        std::cout << GRAY << std::string(terminal_width, '-') << RESET << std::endl;
    }
}

std::chrono::system_clock::time_point Formatter::parse_timestamp(const std::string& timestamp_str) {
    std::tm tm = {};
    std::stringstream ss(timestamp_str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string Formatter::format_human_readable(const std::string& timestamp_str) {
    if (timestamp_str.length() > 19) return timestamp_str.substr(0, 19);
    return timestamp_str;
}

void Formatter::success(const std::string& msg) {
    std::cout << GREEN << "✔ " << RESET << msg << std::endl;
}

void Formatter::error(const std::string& msg) {
    std::cerr << RED << "✘ Error: " << RESET << msg << std::endl;
}

void Formatter::help() {
    std::cout << BOLD << "Chronos CLI v0.1" << RESET << std::endl;
    std::cout << "Usage: chronos <command> [arguments]" << std::endl;
}

int Formatter::get_terminal_width(int default_width) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) return w.ws_col;
    return default_width;
}

std::vector<std::string> Formatter::word_wrap(const std::string& text, size_t line_length) {
    std::vector<std::string> lines;
    if (line_length <= 0) return {text};
    for (size_t i = 0; i < text.length(); i += line_length) {
        lines.push_back(text.substr(i, line_length));
    }
    return lines;
}

}