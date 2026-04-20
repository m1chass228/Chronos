#include <api/ichronos.hpp>
#include <api/dto/note_dto.hpp>
#include <api/dto/response.hpp>

#include <set>
#include <algorithm> 

#include "../formatter.hpp"

namespace chronos::cli::commands {

void executeRemove(api::IChronos* api, const std::vector<std::string>& ids) {
    if (ids.empty()) return;

    std::vector<std::string> fullIds;
    std::set<std::string> allAttachments; // Используем set, чтобы пути не дублировались

    std::cout << "Resolving IDs..." << std::endl;

    for (const auto& shortId : ids) {
        auto note = api->getNoteById(shortId);
        if (note.has_value()) {
            fullIds.push_back(note->id);
            // Собираем все пути к файлам из этой заметки
            for (const auto& path : note->attachments) {
                allAttachments.insert(path);
            }
        } else {
            std::cout << "  Note with ID '" << shortId << "' not found." << std::endl;
        }
    }

    if (fullIds.empty()) return;

    // --- ИНСПЕКЦИЯ ВЛИЯНИЯ ---
    std::cout << "\n! YOU ARE ABOUT TO DELETE " << fullIds.size() << " NOTE(S)." << std::endl;
    
    bool hasSharedFiles = false;
    for (const auto& file : allAttachments) {
        auto usage = api->getNotesByAttachment(file);
        
        // Если файл используется где-то еще, кроме тех заметок, что мы и так удаляем
        // (Тут логика чуть сложнее, но для начала просто покажем список)
        if (usage.size() > 1) { 
            std::cout << "📎 Attachment " << file << " is also used in:" << std::endl;
            for (const auto& n : usage) {
                // Не показываем ту заметку, которую и так удаляем
                bool beingDeleted = std::find(fullIds.begin(), fullIds.end(), n.id) != fullIds.end();
                if (!beingDeleted) {
                    std::cout << "   -> Note [" << n.id.substr(0, 8) << "...]" << std::endl;
                    hasSharedFiles = true;
                }
            }
        }
    }

    std::cout << "\nAre you sure? [y/N]: ";
    std::string confirm;
    std::getline(std::cin, confirm);
    std::transform(confirm.begin(), confirm.end(), confirm.begin(), ::tolower);

    if (confirm == "y" || confirm == "yes") {
        // 1. Удаляем сами заметки (это удалит записи в БД и в note_tags)
        auto res = api->removeNote(fullIds);
        
        // 2. Пробуем почистить аттачменты
        for (const auto& file : allAttachments) {
            // Наш метод removeAttachment в Application уже умеет проверять RefCount!
            api->removeAttachment(file);
        }

        if (res.success) {
            std::cout << "✔ " << res.message << std::endl;
        } else {
            std::cout << "Error " << res.message << std::endl;
        }
    } else {
        std::cout << "Operation cancelled." << std::endl;
    }
}

}
