#include "istorage.hpp"

#include <pqxx/pqxx>

#include <iostream>
#include <iomanip>
#include <sstream>

namespace chronos::core::storage {

class PostgresStorage : public IStorage {
public:

explicit PostgresStorage(const std::string& conn_str)
    : m_conn(std::make_unique<pqxx::connection>(conn_str)) {}

bool connect(const std::string& conn_str) {
    try {
        m_conn = std::make_unique<pqxx::connection>(conn_str);
        return m_conn->is_open();
    } catch (const std::exception& e) {
        std::cerr << "Postgres connection error: " << e.what() << std::endl;
        return false;
    }
}

// 1. Метод инициализации — создаем таблицу, если ее нет
bool init() override {
    try {
        pqxx::work tx(*m_conn);
        // 1. Таблица заметок
        tx.exec("CREATE TABLE IF NOT EXISTS notes ("
                "id UUID PRIMARY KEY, "
                "content TEXT NOT NULL, "
                "created_at TIMESTAMP NOT NULL, "
                "updated_at TIMESTAMP NOT NULL);");

        // 2. Таблица уникальных имен тегов
        tx.exec("CREATE TABLE IF NOT EXISTS tags ("
                "id SERIAL PRIMARY KEY, "
                "name TEXT UNIQUE NOT NULL);");

        // 3. Таблица связей (Many-to-Many)
        tx.exec("CREATE TABLE IF NOT EXISTS note_tags ("
                "note_id UUID REFERENCES notes(id) ON DELETE CASCADE, "
                "tag_id INTEGER REFERENCES tags(id) ON DELETE CASCADE, "
                "PRIMARY KEY (note_id, tag_id));");
        
        // 4. Таблица аттачментов (One-to-Many)
        // Здесь мы храним пути к файлам, привязанные к ID заметки
        tx.exec("CREATE TABLE IF NOT EXISTS attachments ("
                "id SERIAL PRIMARY KEY, "
                "note_id UUID REFERENCES notes(id) ON DELETE CASCADE, "
                "file_path TEXT NOT NULL);");

        tx.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Init DB error: " << e.what() << std::endl;
        return false;
    }
}

// 2. Метод получения списка заметок
std::vector<domain::Note> getNotes(const std::vector<std::string>& tags) override {
    std::vector<domain::Note> results;
    try {
        pqxx::nontransaction nt(*m_conn);

        // Основной запрос остается тем же
        std::string sql =
            "SELECT n.id, n.content, n.created_at, n.updated_at, "
            "ARRAY(SELECT t.name FROM tags t JOIN note_tags nt ON t.id = nt.tag_id WHERE nt.note_id = n.id) as tag_list, " // <-- БЫЛА ЗАПЯТАЯ
            "ARRAY(SELECT file_path FROM attachments WHERE note_id = n.id) as attach_list " // <-- И ТУТ ТЕПЕРЬ ЕСТЬ
            "FROM notes n ";

        pqxx::result res;

        if (tags.empty()) {
            // Если тегов нет, просто получаем все заметки
            res = nt.exec(sql + "ORDER BY n.created_at DESC");
        } else {
            // --- МАГИЯ НАЧИНАЕТСЯ ЗДЕСЬ ---
            // Нам нужно сгенерировать условие WHERE, которое проверит наличие всех тегов.

            // 1. Собираем плейсхолдеры для параметров ($1, $2, $3...)
            std::string placeholders;
            for (size_t i = 0; i < tags.size(); ++i) {
                placeholders += "$" + std::to_string(i + 1);
                if (i < tags.size() - 1) {
                    placeholders += ", ";
                }
            }

            // 2. Создаем подзапрос, который будет проверять, что количество
            // найденных тегов у заметки равно количеству тегов, которые мы ищем.
            sql += "WHERE (SELECT COUNT(DISTINCT t2.name) FROM tags t2 JOIN note_tags nt2 ON t2.id = nt2.tag_id "
                    "WHERE nt2.note_id = n.id AND t2.name IN (" + placeholders + ")) = " + std::to_string(tags.size()) + " ";

            sql += "ORDER BY n.created_at DESC";

            // 3. Выполняем параметризованный запрос с вектором тегов
            pqxx::params params;
            for(const auto& tag : tags) {
                params.append(tag);
            }
            res = nt.exec(sql, params);
        }

        for (const auto& row : res) {
            std::string id = row["id"].as<std::string>();
            std::string content = row["content"].as<std::string>();

            auto created = parseTime(row["created_at"].as<std::string>());
            auto updated = parseTime(row["updated_at"].as<std::string>());

            // libpqxx сам поймет, как превратить ARRAY в вектор
            // Если массив в базе пустой, as<std::vector>() вернет пустой вектор
            auto note_tags = parseArray(row["tag_list"].as<std::string>());
            auto note_attachments = parseArray(row["attach_list"].as<std::string>());

            results.emplace_back(
                id,
                content,
                created,
                updated,
                note_tags,
                note_attachments
            );
        }

    } catch (const std::exception& e) {
        std::cerr << "--- DATABASE CRASH ---" << std::endl;
        std::cerr << e.what() << std::endl;
    }
    return results;
}

// 3. Удаление по полному ID (просто и надежно)
bool deleteNote(const std::string& id) override {
    try {
        pqxx::work tx(*m_conn);
        tx.exec("DELETE FROM notes WHERE id = $1", pqxx::params{id});
        tx.commit();
        return true;
    } catch (...) {
        return false;
    }
}

// 4. Обычный поиск по ID (точный)
std::vector<domain::Note> search(const std::string& query) override {
    std::vector<domain::Note> results;
    try {
        pqxx::nontransaction nt(*m_conn);

        std::string sql = 
            "SELECT n.id, n.content, n.created_at, n.updated_at, "
            "ARRAY(SELECT t.name FROM tags t JOIN note_tags nt ON t.id = nt.tag_id WHERE nt.note_id = n.id) as tag_list, "
            "ARRAY(SELECT file_path FROM attachments WHERE note_id = n.id) as attach_list "
            "FROM notes n "
            "WHERE n.content ILIKE '%' || $1 || '%' "
            "ORDER BY n.created_at DESC";

        pqxx::result res = nt.exec(sql, pqxx::params{query});

        for (const auto& row : res) {
            results.emplace_back(
                row["id"].as<std::string>(),
                row["content"].as<std::string>(),
                parseTime(row["created_at"].as<std::string>()),
                parseTime(row["updated_at"].as<std::string>()),
                parseArray(row["tag_list"].as<std::string>()),
                parseArray(row["attach_list"].as<std::string>())
            );
        }
    } catch (const std::exception& e) {
        std::cerr << "--- SEARCH ERROR ---" << std::endl;
        std::cerr << e.what() << std::endl;
    }
    return results;
}

std::optional<domain::Note> getNoteById(const std::string& id) override {
    return getNoteByShortId(id); 
}

// 6. Сохранение
bool saveNote(const domain::Note& note) override {
try {
    pqxx::work tx(*m_conn);

    // 1. Сохраняем или обновляем основную запись заметки
    tx.exec("INSERT INTO notes (id, content, created_at, updated_at) "
            "VALUES ($1, $2, $3, CURRENT_TIMESTAMP) "
            "ON CONFLICT (id) DO UPDATE SET "
            "content = EXCLUDED.content, "
            "updated_at = CURRENT_TIMESTAMP",
            pqxx::params{note.getId(), note.getContent(), note.getCreatedAsString()});

    // 2. Работа с ТЕГАМИ (Many-to-Many)
    // Удаляем старые связи
    tx.exec("DELETE FROM note_tags WHERE note_id = $1", pqxx::params{note.getId()});

    for (const auto& tagName : note.getTags()) {
        // Вставляем тег, если его нет, и берем его ID
        pqxx::result res = tx.exec(
            "INSERT INTO tags (name) VALUES ($1) "
            "ON CONFLICT (name) DO UPDATE SET name = EXCLUDED.name RETURNING id",
            pqxx::params{tagName});
        
        int tagId = res[0][0].as<int>();

        // Создаем связь в промежуточной таблице
        tx.exec("INSERT INTO note_tags (note_id, tag_id) VALUES ($1, $2) ON CONFLICT DO NOTHING",
                pqxx::params{note.getId(), tagId});
    }

    // 3. Работа с АТТАЧМЕНТАМИ (One-to-Many)
    // Удаляем записи о старых файлах в БД (сами файлы на диске не трогаем здесь)
    tx.exec("DELETE FROM attachments WHERE note_id = $1", pqxx::params{note.getId()});

    for (const auto& filePath : note.getAttachments()) {
        tx.exec("INSERT INTO attachments (note_id, file_path) VALUES ($1, $2)",
                pqxx::params{note.getId(), filePath});
    }

        tx.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "--- SAVE ERROR ---" << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }
}

// 7. Получение по начальным символам UUID
std::optional<domain::Note> getNoteByShortId(const std::string& shortId) override {
    try {
        pqxx::nontransaction nt(*m_conn);
        
        std::string sql = 
            "SELECT n.id, n.content, n.created_at, n.updated_at, "
            "ARRAY(SELECT t.name FROM tags t JOIN note_tags nt ON t.id = nt.tag_id WHERE nt.note_id = n.id) as tag_list, "
            "ARRAY(SELECT file_path FROM attachments WHERE note_id = n.id) as attach_list "
            "FROM notes n WHERE n.id::text LIKE $1 || '%'";

        pqxx::result res = nt.exec(sql, pqxx::params{shortId});

        if (res.empty()) return std::nullopt;
        if (res.size() > 1) {
            std::cerr << "Found " << res.size() << " notes, please refine ID!" << std::endl;
            return std::nullopt;
        }

        const auto& row = res[0];

        return domain::Note(
            row["id"].as<std::string>(),
            row["content"].as<std::string>(),
            parseTime(row["created_at"].as<std::string>()),
            parseTime(row["updated_at"].as<std::string>()),
            parseArray(row["tag_list"].as<std::string>()),
            parseArray(row["attach_list"].as<std::string>())
        );

    } catch (const std::exception& e) {
        std::cerr << "GetNoteByShortId error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

// 8. Удаление по начальным символам UUID
bool deleteNoteByShortId(const std::string& shortId) override {
    try {
        pqxx::work tx(*m_conn);
        // ✅ exec вместо exec_params
        auto res = tx.exec(
            "SELECT id FROM notes WHERE id::text LIKE $1 || '%'",
            pqxx::params{shortId});

        if (res.size() != 1) return false;

        tx.exec("DELETE FROM notes WHERE id = $1",
                pqxx::params{res[0][0].as<std::string>()});
        tx.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "DeleteNoteByShortId error: " << e.what() << std::endl;
        return false;
    }
}

// 9. Получение заметок по аттачменту
std::vector<domain::Note> getNotesByAttachment(const std::string& filePath) override {
    std::vector<domain::Note> results;
    try {
        pqxx::nontransaction nt(*m_conn);

        std::string sql =
            "SELECT n.id, n.content, n.created_at, n.updated_at, "
            "ARRAY(SELECT t.name FROM tags t JOIN note_tags nt "
            "      ON t.id = nt.tag_id WHERE nt.note_id = n.id) as tag_list, "
            "ARRAY(SELECT file_path FROM attachments "
            "      WHERE note_id = n.id) as attach_list "
            "FROM notes n "
            "JOIN attachments a ON n.id = a.note_id "
            "WHERE a.file_path = $1 "
            "ORDER BY n.created_at DESC";

        pqxx::result res = nt.exec(sql, pqxx::params{filePath});

        for (const auto& row : res) {
            results.emplace_back(
                row["id"].as<std::string>(),
                row["content"].as<std::string>(),
                parseTime(row["created_at"].as<std::string>()),
                parseTime(row["updated_at"].as<std::string>()),
                parseArray(row["tag_list"].as<std::string>()),
                parseArray(row["attach_list"].as<std::string>())
            );
        }
    } catch (const std::exception& e) {
        std::cerr << "GetNotesByAttachment error: " << e.what() << std::endl;
    }
    return results;
}

// 10. Удаление аттачи у всех всех заметок
bool deleteAttachment(const std::string& filePath) override {
    try {
        pqxx::work tx(*m_conn);
        tx.exec("DELETE FROM attachments WHERE file_path = $1",
                pqxx::params{filePath});
        tx.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "DeleteAttachment error: " << e.what() << std::endl;
        return false;
    }
}

private:
    std::unique_ptr<pqxx::connection> m_conn;

    // Вспомогательная функция для парсинга времени из строки Postgres
    domain::TimePoint parseTime(const std::string& ts) override {
        std::tm tm = {};
        std::stringstream ss(ts);
        // Postgres по умолчанию отдает формат YYYY-MM-DD HH:MM:SS
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    // Парсит строку вида {"tag1","tag2","tag3"} в вектор
    std::vector<std::string> parseArray(const std::string& pgArray) override {
        std::vector<std::string> result;
        if (pgArray.size() <= 2) return result; // "{}" — пустой массив

        std::string inner = pgArray.substr(1, pgArray.size() - 2);
        std::stringstream ss(inner);
        std::string item;

        while (std::getline(ss, item, ',')) {
            if (!item.empty() && item.front() == '"') {
                item = item.substr(1, item.size() - 2);
            }
            result.push_back(item);
        }
        return result;
    }
};

std::unique_ptr<IStorage> createStorage(const std::string& connStr) {
    return std::make_unique<PostgresStorage>(connStr);
}

}
