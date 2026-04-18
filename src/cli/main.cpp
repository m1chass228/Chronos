#include "../core/application.hpp"
#include <api/ichronos.hpp>
#include <api/dto/note_dto.hpp>
#include <api/dto/response.hpp>

#include "formatter.hpp"

#include <iostream>
#include <string>
#include <vector>

// Прототипы функций из commands (чтобы не плодить хедеры для каждой команды)
namespace chronos::cli::commands {
    void executeAdd(api::IChronos* api, const std::string& content, const std::vector<std::string>& tags);
    void executeList(api::IChronos* api, const std::vector<std::string>& tags);
    void executeRemove(api::IChronos* api, const std::string& id);
    void executeSearch(api::IChronos* api, const std::string& query);
}

int main(int argc, char** argv) {
    using namespace chronos::cli;

    // Инициализируем Ядро
    auto app = std::make_unique<chronos::core::Application>();

    if (argc < 2) {
        Formatter::help();
        return 0;
    }

    std::string command = argv[1];

    try {
        if (command == "add" && argc >= 3) {
            std::string content = argv[2];
            std::vector<std::string> tags;
            for (int i = 3; i < argc; ++i) tags.push_back(argv[i]);
            commands::executeAdd(app.get(), content, tags);
        }
        else if (command == "ls") {
            std::vector<std::string> tags;
            for (int i = 2; i < argc; ++i) { // Начинаем с argv[2]
                tags.push_back(argv[i]);
            }
            // Вызываем executeList с вектором тегов
            commands::executeList(app.get(), tags);
        }
        else if (command == "rm" && argc >= 3) {
            commands::executeRemove(app.get(), argv[2]);
        }
        else if (command == "search" && argc >= 3) {
            commands::executeSearch(app.get(), argv[2]);
        }
        else {
            Formatter::error("Unknown command or missing arguments.");
            Formatter::help();
        }
    } catch (const std::exception& e) {
        Formatter::error(e.what());
        return 1;
    }

    return 0;
}
