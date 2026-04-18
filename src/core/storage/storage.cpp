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
                "ARRAY(SELECT t.name FROM tags t JOIN note_tags nt ON t.id = nt.tag_id WHERE nt.note_id = n.id) as tag_list "
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
                res = nt.exec_params(sql, params);
            }

            // ... остальная часть функции (цикл for и парсинг) остается без изменений ...
            for (const auto& row : res) {
                // 1. Извлекаем основные данные
                std::string id = row["id"].as<std::string>();
                std::string content = row["content"].as<std::string>();

                // 2. Парсим таймстампы через вспомогательный метод
                auto created = parseTime(row["created_at"].as<std::string>());
                auto updated = parseTime(row["updated_at"].as<std::string>());

                // 3. НОРМАЛЬНЫЙ ПАРСИНГ МАССИВА ТЕГОВ
                std::vector<std::string> note_tags;
                std::string raw_tags = row["tag_list"].as<std::string>();

                pqxx::array_parser parser(raw_tags);

                while (true) {
                    auto [j, val] = parser.get_next();

                    if (j == pqxx::array_parser::juncture::done) {
                        break;
                    }

                    // ВОТ ЗДЕСЬ: используем string_value
                    if (j == pqxx::array_parser::juncture::string_value) {
                        note_tags.push_back(std::string(val));
                    }
                }

                // 4. Создаем объект Note и добавляем в результат
                results.emplace_back(
                    id,
                    content,
                    created,
                    updated,
                    note_tags,
                    std::vector<std::string>{} // Метаданные, если нужны
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
            tx.exec_params("DELETE FROM notes WHERE id = $1", id);
            tx.commit();
            return true;
        } catch (...) {
            return false;
        }
    }

    // 4. Обычный поиск по ID (точный)
    std::optional<domain::Note> getNoteById(const std::string& id) override {
        // Мы уже написали логику в getNoteByShortId
        return getNoteByShortId(id);
    }

    // 5. Поиск по тексту
    std::vector<domain::Note> search(const std::string& query) override {
        std::vector<domain::Note> results;
        try {
            pqxx::nontransaction nt(*m_conn);

            // Используем ILIKE для регистронезависимого поиска
            // %query% ищет подстроку в любом месте текста
            pqxx::result res = nt.exec_params(
                "SELECT id, content, created_at, updated_at FROM notes "
                "WHERE content ILIKE '%' || $1 || '%' "
                "ORDER BY created_at DESC",
                query
            );

            for (const auto& row : res) {
                results.emplace_back(
                    row["id"].as<std::string>(),
                    row["content"].as<std::string>(),
                    parseTime(row["created_at"].as<std::string>()),
                    parseTime(row["updated_at"].as<std::string>()),
                    std::vector<std::string>{},
                    std::vector<std::string>{}
                );
            }
        } catch (const std::exception& e) {
            std::cerr << "Search error: " << e.what() << std::endl;
        }
        return results;
    }

    // 6. Сохранение
    bool saveNote(const domain::Note& note) override {
        try {
            pqxx::work tx(*m_conn);

            // Сохраняем/обновляем основное тело заметки
            tx.exec_params(
                "INSERT INTO notes (id, content, created_at, updated_at) "
                // Используем SQL функцию CURRENT_TIMESTAMP напрямую, плейсхолдер не нужен
                "VALUES ($1, $2, $3, CURRENT_TIMESTAMP) " // <-- Теперь 3 плейсхолдера
                "ON CONFLICT (id) DO UPDATE SET content = EXCLUDED.content, updated_at = CURRENT_TIMESTAMP", // И здесь тоже
                note.getId(),
                note.getContent(),
                note.getCreatedAsString()
            );

            // Чистим старые связи тегов (если это обновление), чтобы перезаписать новыми
            tx.exec_params("DELETE FROM note_tags WHERE note_id = $1", note.getId());

            // Сохраняем каждый тег
            for (const auto& tagName : note.getTags()) {
                // Вставляем тег, если его нет, и получаем его ID
                pqxx::result res = tx.exec_params(
                    "INSERT INTO tags (name) VALUES ($1) "
                    "ON CONFLICT (name) DO UPDATE SET name = EXCLUDED.name RETURNING id",
                    tagName
                );
                int tagId = res[0][0].as<int>();

                // Связываем заметку с тегом
                tx.exec_params(
                    "INSERT INTO note_tags (note_id, tag_id) VALUES ($1, $2) ON CONFLICT DO NOTHING",
                    note.getId(), tagId
                );
            }

            tx.commit();
            return true;
        } catch (const std::exception& e) {
            std::cerr << "SaveNote error: " << e.what() << std::endl;
            return false;
        }
    }

    // 7. Получение по начальным символам UUID
    std::optional<domain::Note> getNoteByShortId(const std::string& shortId) override {
        try {
            pqxx::nontransaction nt(*m_conn);
            auto res = nt.exec_params(
                "SELECT id, content, created_at, updated_at FROM notes WHERE id::text LIKE $1 || '%'",
                shortId
            );

            if (res.empty()) return std::nullopt;
            if (res.size() > 1) {
                // Лучше не бросать исключение, а просто считать, что ничего не найдено
                std::cerr << "Найдено несколько заметок, уточните ID!" << std::endl;
                return std::nullopt;
            }

            auto row = res[0];
            return domain::Note(
                row[0].as<std::string>(),
                row[1].as<std::string>(),
                parseTime(row[2].as<std::string>()),
                parseTime(row[3].as<std::string>()),
                {}, {}
            );
        } catch (const std::exception& e) {
            std::cerr << "GetNoteByShortId error: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    // 8. Удаление по начальным символам UUID
    bool deleteNoteByShortId(const std::string& shortId) override {
        pqxx::work tx(*m_conn);
        // Сначала проверяем, не удалим ли мы лишнего
        auto res = tx.exec_params("SELECT id FROM notes WHERE id::text LIKE $1 || '%'", shortId);

        if (res.size() != 1) return false; // Либо 0, либо слишком много

        tx.exec_params("DELETE FROM notes WHERE id = $1", res[0][0].as<std::string>());
        tx.commit();
        return true;
    }

private:
    std::unique_ptr<pqxx::connection> m_conn;

    // Вспомогательная функция для парсинга времени из строки Postgres
    domain::TimePoint parseTime(const std::string& ts) {
        std::tm tm = {};
        std::stringstream ss(ts);
        // Postgres по умолчанию отдает формат YYYY-MM-DD HH:MM:SS
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
};

std::unique_ptr<IStorage> createStorage(const std::string& connStr) {
    return std::make_unique<PostgresStorage>(connStr);
}

}
