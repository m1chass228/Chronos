#include <api/ichronos.hpp>
#include <api/dto/note_dto.hpp>
#include <api/dto/response.hpp>

#include "../formatter.hpp"

namespace chronos::cli::commands {

void executeRemove(api::IChronos* api, const std::vector<std::string>& ids) {
    if (ids.empty()) return;

    std::vector<std::string> fullIds;

    std::cout << "Resolving IDs..." << std::endl;

    for (const auto& shortId : ids) {
        // Используем твой метод поиска по короткому ID
        // (Возможно, стоит добавить метод getFullIdByShortId в API,
        // но пока можно просто вытащить всю заметку)
        auto note = api->getNoteById(shortId); // Твой метод в core/application.cpp

        if (note.has_value()) {
            fullIds.push_back(note->id);
        } else {
            std::cout << "!  Note with ID '" << shortId << "' not found or ambiguous." << std::endl;
        }
    }

    if (fullIds.empty()) return;

    // Выводим предупреждение с полными UUID
    std::cout << "\n!  YOU ARE ABOUT TO DELETE:" << std::endl;
    for (const auto& fid : fullIds) {
        std::cout << "   - " << fid << std::endl;
    }

    std::cout << "\nAre you sure you want to continue? [y/N]: ";

    std::string response;
    std::getline(std::cin, response);
    std::transform(response.begin(), response.end(), response.begin(), ::tolower);

    if (response == "y" || response == "yes") {
        // Теперь удаляем уже по списку найденных полных ID
        auto res = api->removeNote(fullIds);
        if (res.success) {
            std::cout << "✔ " << res.message << std::endl;
        } else {
            std::cout << "❌ " << res.message << std::endl;
        }
    } else {
        std::cout << "Operation cancelled." << std::endl;
    }
}

}
