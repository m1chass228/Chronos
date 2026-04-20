#pragma once

#include "../domain/note.hpp"

#include <vector>
#include <string>
#include <optional>

namespace chronos::core::storage {
class IStorage {
public:
    virtual ~IStorage() = default;
    // Инициализация (создание таблиц, если их нет)
    virtual bool init() = 0;

    // Сохранение или обновление (Upsert)
    virtual bool saveNote(const domain::Note& note) = 0;

    // Поиск одной заметки (то самое "воскрешение")
    virtual std::optional<domain::Note> getNoteById(const std::string& id) = 0;

    // Получение списка по тегу (если пустой — все)
    virtual std::vector<domain::Note> getNotes(const std::vector<std::string>& tags) = 0;

    // Поиск по тексту (Full-text search в Postgres)
    virtual std::vector<domain::Note> search(const std::string& query) = 0;

    // Удаление по ID
    virtual bool deleteNote(const std::string& id) = 0;

    // Получение по ShortId
    virtual std::optional<domain::Note> getNoteByShortId(const std::string& shortId) = 0;

    // Удаление по ShortId
    virtual bool deleteNoteByShortId(const std::string& shortId) = 0;

    // Получить все заметки с аттачей
    virtual std::vector<domain::Note> getNotesByAttachment(const std::string& filePath) = 0;

    // Удаление аттачи у всех заметок 
    virtual bool deleteAttachment(const std::string& filePath) = 0;

private:
    // Вспомогательная функция для парсинга времени из строки Postgres
    virtual domain::TimePoint parseTime(const std::string& ts) = 0;

    virtual std::vector<std::string> parseArray(const std::string& pgArray) = 0;
};

// создание склада
std::unique_ptr<IStorage> createStorage(const std::string& connStr);

}
