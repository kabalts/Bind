#include "storage/Database.hpp"
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <direct.h>
#include <windows.h>
#define mkdir(path, mode) _mkdir(path)

static bool mkdir_p(const char* path) {
    String s(path);
    size_t len = s.length();
    char* buf = new char[len + 1];
    std::strcpy(buf, path);

    for (size_t i = 1; i < len; ++i) {
        if (buf[i] == '/') {
            buf[i] = '\0';
            mkdir(buf, 0755);
            buf[i] = '/';
        }
    }
    int rc = mkdir(path, 0755);
    delete[] buf;
    return (rc == 0 || errno == EEXIST);
}

Database::Database() : m_name(), m_base_dir(), m_tables(), m_open(false) {}

Database::~Database() {
    close();
}

bool Database::create(const String& name, const String& base_dir) {
    close();
    m_name = name;
    m_base_dir = base_dir;

    String data_path = data_dir();
    if (!mkdir_p(data_path.c_str())) {
        return false;
    }

    m_open = true;
    return true;
}

bool Database::open(const String& name, const String& base_dir) {
    close();
    m_name = name;
    m_base_dir = base_dir;

    String dp = data_dir();

    String search_path = dp + "\\*";
    WIN32_FIND_DATAA find_data;
    HANDLE h_find = FindFirstFileA(search_path.c_str(), &find_data);
    if (h_find == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (find_data.cFileName[0] == '.') continue;
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;

        String table_name(find_data.cFileName);
        String meta_path = dp + "/" + table_name + "/" + table_name + ".meta";
        FILE* test = std::fopen(meta_path.c_str(), "r");
        if (test) {
            std::fclose(test);
            Table table;
            if (table.open_existing(table_name, dp)) {
                m_tables.push_back(std::move(table));
            }
        }
    } while (FindNextFileA(h_find, &find_data));

    FindClose(h_find);

    m_open = true;
    return true;
}

void Database::close() {
    for (size_t i = 0; i < m_tables.size(); ++i) {
        m_tables[i].close();
    }
    m_tables.clear();
    m_open = false;
}

bool Database::is_open() const {
    return m_open;
}

String Database::data_dir() const {
    return m_base_dir + "/" + m_name;
}

Table* Database::create_table(const String& table_name,
                               const ArrayList<Column>& columns) {
    if (!m_open) return nullptr;

    for (size_t i = 0; i < m_tables.size(); ++i) {
        if (m_tables[i].name() == table_name) {
            return nullptr;
        }
    }

    Table table;
    if (!table.create(table_name, columns, data_dir())) {
        return nullptr;
    }

    m_tables.push_back(std::move(table));
    return &m_tables.back();
}

bool Database::drop_table(const String& table_name) {
    for (size_t i = 0; i < m_tables.size(); ++i) {
        if (m_tables[i].name() == table_name) {
            m_tables[i].close();
            String base = data_dir() + "/" + table_name;
            String dat = base + "/" + table_name + ".dat";
            String idx = base + "/" + table_name + ".idx";
            String meta = base + "/" + table_name + ".meta";
            std::remove(dat.c_str());
            std::remove(idx.c_str());
            std::remove(meta.c_str());
            std::remove(base.c_str());
            m_tables.erase(m_tables.cbegin() + static_cast<long>(i));
            return true;
        }
    }
    return false;
}

Table* Database::get_table(const String& table_name) {
    for (size_t i = 0; i < m_tables.size(); ++i) {
        if (m_tables[i].name() == table_name) {
            return &m_tables[i];
        }
    }
    return nullptr;
}
