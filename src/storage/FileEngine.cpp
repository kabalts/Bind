#include "storage/FileEngine.hpp"
#include <cstring>

FileEngine::FileEngine()
    : m_file(nullptr), m_filepath(), m_open(false),
      m_record_count(0), m_file_size(0) {}

FileEngine::~FileEngine() {
    close();
}

FileEngine::FileEngine(FileEngine&& other) noexcept
    : m_file(other.m_file), m_filepath(std::move(other.m_filepath)),
      m_open(other.m_open), m_record_count(other.m_record_count),
      m_file_size(other.m_file_size) {
    other.m_file = nullptr;
    other.m_open = false;
    other.m_record_count = 0;
    other.m_file_size = 0;
}

FileEngine& FileEngine::operator=(FileEngine&& other) noexcept {
    if (this != &other) {
        close();
        m_file = other.m_file;
        m_filepath = std::move(other.m_filepath);
        m_open = other.m_open;
        m_record_count = other.m_record_count;
        m_file_size = other.m_file_size;
        other.m_file = nullptr;
        other.m_open = false;
        other.m_record_count = 0;
        other.m_file_size = 0;
    }
    return *this;
}

bool FileEngine::open(const char* filepath) {
    close();
    m_filepath = filepath;

    m_file = std::fopen(filepath, "r+b");
    if (!m_file) {
        m_file = std::fopen(filepath, "w+b");
        if (!m_file) return false;
    }

    m_open = true;
    refresh_stats();
    return true;
}

void FileEngine::close() {
    if (m_file) {
        std::fflush(m_file);
        std::fclose(m_file);
        m_file = nullptr;
    }
    m_open = false;
    m_record_count = 0;
    m_file_size = 0;
}

bool FileEngine::is_open() const {
    return m_open;
}

long FileEngine::rec_offset(int64_t rec_index) const {
    return static_cast<long>(rec_index * RECORD_SIZE);
}

int64_t FileEngine::append(const Record& record) {
    if (!m_open) return -1;

    if (std::fseek(m_file, 0, SEEK_END) != 0) return -1;

    unsigned char buf[RECORD_SIZE];
    std::memset(buf, 0, RECORD_SIZE);

    std::memcpy(buf, &record.id, sizeof(int32_t));
    buf[4] = record.deleted ? 1 : 0;
    std::memcpy(buf + RECORD_HEADER_SIZE, record.data, STRING_FIELD_SIZE);

    size_t written = std::fwrite(buf, 1, RECORD_SIZE, m_file);
    if (written != RECORD_SIZE) return -1;

    std::fflush(m_file);

    int64_t idx = m_record_count;
    ++m_record_count;
    m_file_size += RECORD_SIZE;
    return idx;
}

bool FileEngine::read_at(int64_t offset, Record& record) const {
    if (!m_open || offset < 0) return false;

    if (std::fseek(m_file, offset, SEEK_SET) != 0) return false;

    unsigned char buf[RECORD_SIZE];
    size_t n = std::fread(buf, 1, RECORD_SIZE, m_file);
    if (n != RECORD_SIZE) return false;

    std::memcpy(&record.id, buf, sizeof(int32_t));
    record.deleted = (buf[4] != 0);
    std::memcpy(record.data, buf + RECORD_HEADER_SIZE, STRING_FIELD_SIZE);
    return true;
}

bool FileEngine::read_record(int64_t record_index, Record& record) const {
    return read_at(rec_offset(record_index), record);
}

bool FileEngine::update_at(int64_t offset, const Record& record) {
    if (!m_open || offset < 0) return false;

    if (std::fseek(m_file, offset, SEEK_SET) != 0) return false;

    unsigned char buf[RECORD_SIZE];
    std::memset(buf, 0, RECORD_SIZE);

    std::memcpy(buf, &record.id, sizeof(int32_t));
    buf[4] = record.deleted ? 1 : 0;
    std::memcpy(buf + RECORD_HEADER_SIZE, record.data, STRING_FIELD_SIZE);

    size_t written = std::fwrite(buf, 1, RECORD_SIZE, m_file);
    if (written != RECORD_SIZE) return false;

    std::fflush(m_file);
    return true;
}

bool FileEngine::delete_at(int64_t offset) {
    if (!m_open || offset < 0) return false;

    if (std::fseek(m_file, offset + 4, SEEK_SET) != 0) return false;

    unsigned char deleted_flag = 1;
    size_t written = std::fwrite(&deleted_flag, 1, 1, m_file);
    if (written != 1) return false;

    std::fflush(m_file);
    return true;
}

int64_t FileEngine::record_count() const {
    return m_record_count;
}

int64_t FileEngine::file_size_bytes() const {
    return m_file_size;
}

void FileEngine::refresh_stats() {
    if (!m_file) return;
    if (std::fseek(m_file, 0, SEEK_END) != 0) return;
    long size = std::ftell(m_file);
    if (size < 0) return;
    m_file_size = static_cast<int64_t>(size);
    m_record_count = m_file_size / RECORD_SIZE;
}

bool FileEngine::read_record_from(const char* filepath, int64_t offset, Record& record) {
    FILE* f = std::fopen(filepath, "rb");
    if (!f) return false;
    if (std::fseek(f, static_cast<long>(offset), SEEK_SET) != 0) {
        std::fclose(f);
        return false;
    }
    unsigned char buf[RECORD_SIZE];
    size_t n = std::fread(buf, 1, RECORD_SIZE, f);
    std::fclose(f);
    if (n != RECORD_SIZE) return false;
    std::memcpy(&record.id, buf, sizeof(int32_t));
    record.deleted = (buf[4] != 0);
    std::memcpy(record.data, buf + RECORD_HEADER_SIZE, STRING_FIELD_SIZE);
    return true;
}
