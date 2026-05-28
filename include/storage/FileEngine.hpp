#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "common/String.hpp"

class FileEngine {
public:
    static constexpr size_t STRING_FIELD_SIZE = 256;
    static constexpr size_t RECORD_HEADER_SIZE = 5;
    static constexpr size_t RECORD_SIZE = RECORD_HEADER_SIZE + STRING_FIELD_SIZE;

    struct Record {
        int32_t id;
        bool    deleted;
        char    data[STRING_FIELD_SIZE];

        Record() : id(0), deleted(false) {
            data[0] = '\0';
        }
    };

    FileEngine();
    ~FileEngine();

    FileEngine(const FileEngine&) = delete;
    FileEngine& operator=(const FileEngine&) = delete;
    FileEngine(FileEngine&& other) noexcept;
    FileEngine& operator=(FileEngine&& other) noexcept;

    bool open(const char* filepath);
    void close();
    bool is_open() const;

    int64_t append(const Record& record);
    bool    read_at(int64_t offset, Record& record) const;
    bool    read_record(int64_t record_index, Record& record) const;
    bool    update_at(int64_t offset, const Record& record);
    bool    delete_at(int64_t offset);

    int64_t record_count() const;
    int64_t file_size_bytes() const;

    const String& filepath() const { return m_filepath; }

    static bool read_record_from(const char* filepath, int64_t offset, Record& record);

private:
    FILE*   m_file;
    String  m_filepath;
    bool    m_open;

    int64_t m_record_count;
    int64_t m_file_size;

    void refresh_stats();
    long rec_offset(int64_t rec_index) const;
};
