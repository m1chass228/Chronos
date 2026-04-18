#include <CLI/CLI.hpp>

#include "../core/application.hpp"

#include "formatter.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace chronos::cli::commands {
    void executeAdd(api::IChronos* api, const std::string& content, const std::vector<std::string>& tags);
    void executeList(api::IChronos* api, const std::vector<std::string>& tags);
    void executeRemove(api::IChronos* api, const std::vector<std::string>& ids);
    void executeSearch(api::IChronos* api, const std::string& query);
}

int main(int argc, char** argv) {
    using namespace chronos::cli;

    CLI::App app{"Chronos — A powerful CLI note manager for geeks"};
    app.require_subcommand(1);

    // Делаем вывод help более детальным
    app.get_formatter()->column_width(30);

    // Переменные
    std::string content, query;
    std::vector<std::string> tags, ids;
    int limit = 10;

    // --- ADD COMMAND ---
    auto* add_cmd = app.add_subcommand("add", "Create a new note");
    add_cmd->add_option("content", content, "Note content")
           ->required()
           ->each([](const std::string& s) {}) // Костыль для именования в help
           ->type_name("TEXT");
    add_cmd->add_option("-t,--tag", tags, "Tags associated with the note")
           ->type_name("TAG");

    // --- LIST COMMAND ---
    auto* ls_cmd = app.add_subcommand("ls", "List your notes chronologically");
    ls_cmd->add_option("-t,--tag", tags, "Filter notes by tags")
          ->type_name("TAG");
    ls_cmd->add_option("-n,--limit", limit, "Number of notes to display")
          ->capture_default_str() // Покажет [10] в help
          ->type_name("INT");

    // --- SEARCH COMMAND ---
    auto* search_cmd = app.add_subcommand("search", "Full-text search through notes");
    search_cmd->add_option("query", query, "Search phrase or word")
              ->required()
              ->type_name("QUERY");

    // --- REMOVE COMMAND ---
    auto* rm_cmd = app.add_subcommand("rm", "Remove notes by their IDs");
    rm_cmd->add_option("ids", ids, "List of note Short-IDs")
          ->required()
          ->type_name("ID");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    auto chronosCore = std::make_unique<chronos::core::Application>();

    try {
        if (add_cmd->parsed()) {
            commands::executeAdd(chronosCore.get(), content, tags);
        }
        else if (ls_cmd->parsed()) {
            // Не забываем передать limit в executeList!
            commands::executeList(chronosCore.get(), tags);
        }
        else if (search_cmd->parsed()) {
            commands::executeSearch(chronosCore.get(), query);
        }
        else if (rm_cmd->parsed()) {
            commands::executeRemove(chronosCore.get(), ids);
        }
    } catch (const std::exception& e) {
        Formatter::error(e.what());
        return 1;
    }

    return 0;
}
