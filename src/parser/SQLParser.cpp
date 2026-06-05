#include "parser/SQLParser.hpp"
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <string>

static bool is_keyword(const String& s) {
    const char* keywords[] = {
        "create", "drop", "use", "select", "insert", "update", "delete",
        "from", "where", "set", "values", "primary", "key",
        "database", "table", "into", "exit",
        "int", "string", "bool", "double",
        nullptr
    };
    for (int i = 0; keywords[i] != nullptr; ++i) {
        if (s == keywords[i]) return true;
    }
    return false;
}

static TokType keyword_to_type(const String& kw) {
    if (kw == "create")   return TokType::KW_CREATE;
    if (kw == "drop")     return TokType::KW_DROP;
    if (kw == "use")      return TokType::KW_USE;
    if (kw == "select")   return TokType::KW_SELECT;
    if (kw == "insert")   return TokType::KW_INSERT;
    if (kw == "update")   return TokType::KW_UPDATE;
    if (kw == "delete")   return TokType::KW_DELETE;
    if (kw == "from")     return TokType::KW_FROM;
    if (kw == "where")    return TokType::KW_WHERE;
    if (kw == "set")      return TokType::KW_SET;
    if (kw == "values")   return TokType::KW_VALUES;
    if (kw == "primary")  return TokType::KW_PRIMARY;
    if (kw == "key")      return TokType::KW_KEY;
    if (kw == "database") return TokType::KW_DATABASE;
    if (kw == "table")    return TokType::KW_TABLE;
    if (kw == "into")     return TokType::KW_INTO;
    if (kw == "exit")     return TokType::KW_EXIT;
    if (kw == "int")      return TokType::KW_INT;
    if (kw == "string")   return TokType::KW_STRING;
    if (kw == "bool")     return TokType::KW_BOOL;
    if (kw == "double")   return TokType::KW_DOUBLE;
    return TokType::IDENTIFIER;
}

SQLParser::SQLParser()
    : m_sql(), m_pos(nullptr), m_line(1), m_col(1),
      m_error(), m_tokens(), m_token_idx(0) {}

void SQLParser::set_error(const String& msg) {
    if (m_error.empty()) {
        m_error = msg;
    }
}

SQLStatement SQLParser::parse(const String& sql) {
    m_sql = sql;
    m_pos = sql.c_str();
    m_line = 1;
    m_col = 1;
    m_error = String();
    m_tokens.clear();
    m_token_idx = 0;

    SQLStatement stmt;
    stmt.type = StmtType::NONE;

    if (!lex()) {
        stmt.error_msg = m_error;
        return stmt;
    }

    m_token_idx = 0;
    stmt = parse_statement();

    if (stmt.error_msg.empty() && m_error.empty()) {
        if (m_token_idx < m_tokens.size()) {
            Token t = peek();
            if (t.type != TokType::SEMICOLON && t.type != TokType::END_OF_FILE) {
                stmt.error_msg = String("Unexpected token: ") + t.value +
                                 String(" at line ") + String(std::to_string(t.line).c_str());
            }
        }
    }

    if (!stmt.error_msg.empty()) {
        return stmt;
    }
    if (!m_error.empty()) {
        stmt.error_msg = m_error;
        return stmt;
    }

    return stmt;
}

bool SQLParser::lex() {
    m_tokens.clear();
    while (*m_pos != '\0') {
        skip_whitespace();
        if (*m_pos == '\0') break;

        if (std::isalpha(static_cast<unsigned char>(*m_pos)) || *m_pos == '_') {
            Token t = lex_identifier_or_keyword();
            m_tokens.push_back(t);
        } else if (std::isdigit(static_cast<unsigned char>(*m_pos)) ||
                   (*m_pos == '-' && std::isdigit(static_cast<unsigned char>(*(m_pos + 1))))) {
            Token t = lex_number();
            m_tokens.push_back(t);
        } else if (*m_pos == '\'' || *m_pos == '"') {
            Token t = lex_string();
            if (t.type == TokType::UNKNOWN) {
                set_error(t.value);
                return false;
            }
            m_tokens.push_back(t);
        } else {
            Token t = lex_symbol();
            if (t.type == TokType::UNKNOWN) {
                set_error(t.value);
                return false;
            }
            m_tokens.push_back(t);
        }
    }

    Token eof;
    eof.type = TokType::END_OF_FILE;
    eof.value = "";
    eof.line = m_line;
    eof.col = m_col;
    m_tokens.push_back(eof);

    return true;
}

void SQLParser::skip_whitespace() {
    while (*m_pos == ' ' || *m_pos == '\t' || *m_pos == '\n' || *m_pos == '\r') {
        if (*m_pos == '\n') {
            ++m_line;
            m_col = 1;
        } else {
            ++m_col;
        }
        ++m_pos;
    }
}

Token SQLParser::lex_identifier_or_keyword() {
    Token t;
    t.line = m_line;
    t.col = m_col;

    const char* start = m_pos;
    while (std::isalnum(static_cast<unsigned char>(*m_pos)) || *m_pos == '_') {
        ++m_pos;
        ++m_col;
    }

    size_t len = static_cast<size_t>(m_pos - start);
    String ident(start, len);

    for (size_t i = 0; i < len; ++i) {
        if (std::isupper(static_cast<unsigned char>(start[i]))) {
            char lower_char = static_cast<char>(
                std::tolower(static_cast<unsigned char>(start[i])));
            ident[i] = lower_char;
        }
    }

    if (is_keyword(ident)) {
        t.type = keyword_to_type(ident);
    } else {
        t.type = TokType::IDENTIFIER;
    }
    t.value = ident;
    return t;
}

Token SQLParser::lex_number() {
    Token t;
    t.line = m_line;
    t.col = m_col;
    t.type = TokType::NUMBER;

    const char* start = m_pos;
    if (*m_pos == '-') {
        ++m_pos;
        ++m_col;
    }
    while (std::isdigit(static_cast<unsigned char>(*m_pos))) {
        ++m_pos;
        ++m_col;
    }
    if (*m_pos == '.') {
        ++m_pos;
        ++m_col;
        while (std::isdigit(static_cast<unsigned char>(*m_pos))) {
            ++m_pos;
            ++m_col;
        }
    }

    size_t len = static_cast<size_t>(m_pos - start);
    t.value = String(start, len);
    return t;
}

Token SQLParser::lex_string() {
    Token t;
    t.line = m_line;
    t.col = m_col;
    t.type = TokType::STRING_LITERAL;

    char quote = *m_pos;
    ++m_pos;
    ++m_col;

    String val;
    while (*m_pos != '\0' && *m_pos != quote) {
        if (*m_pos == '\\') {
            ++m_pos;
            ++m_col;
            if (*m_pos == '\0') {
                t.type = TokType::UNKNOWN;
                t.value = "Unterminated string literal";
                return t;
            }
            switch (*m_pos) {
            case 'n':  val += '\n'; break;
            case 't':  val += '\t'; break;
            case 'r':  val += '\r'; break;
            case '\\': val += '\\'; break;
            case '\'': val += '\''; break;
            case '"':  val += '"';  break;
            default:   val += *m_pos; break;
            }
            ++m_pos;
            ++m_col;
        } else {
            if (*m_pos == '\n') {
                ++m_line;
                m_col = 1;
            } else {
                ++m_col;
            }
            val += *m_pos;
            ++m_pos;
        }
    }

    if (*m_pos != quote) {
        t.type = TokType::UNKNOWN;
        t.value = "Unterminated string literal";
        return t;
    }

    ++m_pos;
    ++m_col;
    t.value = val;
    return t;
}

Token SQLParser::lex_symbol() {
    Token t;
    t.line = m_line;
    t.col = m_col;

    char c = *m_pos;
    ++m_pos;
    ++m_col;

    switch (c) {
    case '(': t.type = TokType::LPAREN; t.value = "("; break;
    case ')': t.type = TokType::RPAREN; t.value = ")"; break;
    case ',': t.type = TokType::COMMA;  t.value = ","; break;
    case ';': t.type = TokType::SEMICOLON; t.value = ";"; break;
    case '*': t.type = TokType::STAR;   t.value = "*"; break;
    case '.': t.type = TokType::DOT;    t.value = "."; break;
    case '=': t.type = TokType::EQ;     t.value = "="; break;
    case '<':
        if (*m_pos == '=') {
            t.type = TokType::LE;
            t.value = "<=";
            ++m_pos; ++m_col;
        } else if (*m_pos == '>') {
            t.type = TokType::NE;
            t.value = "<>";
            ++m_pos; ++m_col;
        } else {
            t.type = TokType::LT;
            t.value = "<";
        }
        break;
    case '>':
        if (*m_pos == '=') {
            t.type = TokType::GE;
            t.value = ">=";
            ++m_pos; ++m_col;
        } else {
            t.type = TokType::GT;
            t.value = ">";
        }
        break;
    case '!':
        if (*m_pos == '=') {
            t.type = TokType::NE;
            t.value = "!=";
            ++m_pos; ++m_col;
        } else {
            t.type = TokType::UNKNOWN;
            t.value = String("Unexpected character: ") + String(&c, 1);
        }
        break;
    default:
        t.type = TokType::UNKNOWN;
        t.value = String("Unexpected character: ") + String(&c, 1);
        break;
    }
    return t;
}

Token SQLParser::peek() const {
    if (m_token_idx < m_tokens.size()) {
        return m_tokens[m_token_idx];
    }
    Token eof;
    eof.type = TokType::END_OF_FILE;
    eof.value = "";
    eof.line = 0;
    eof.col = 0;
    return eof;
}

Token SQLParser::advance() {
    if (m_token_idx < m_tokens.size()) {
        return m_tokens[m_token_idx++];
    }
    Token eof;
    eof.type = TokType::END_OF_FILE;
    eof.value = "";
    eof.line = 0;
    eof.col = 0;
    return eof;
}

Token SQLParser::expect(TokType type) {
    Token t = peek();
    if (t.type != type) {
        String msg = String("Expected token type ") +
                     String(std::to_string(static_cast<int>(type)).c_str()) +
                     String(" but got: ") + t.value +
                     String(" at line ") + String(std::to_string(t.line).c_str());
        set_error(msg);
        SQLStatement err_stmt;
        err_stmt.type = StmtType::NONE;
        err_stmt.error_msg = msg;
        return t;
    }
    return advance();
}

bool SQLParser::match(TokType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool SQLParser::check(TokType type) const {
    return peek().type == type;
}

SQLStatement SQLParser::parse_statement() {
    Token t = peek();

    switch (t.type) {
    case TokType::KW_CREATE:  return parse_create();
    case TokType::KW_DROP:    return parse_drop();
    case TokType::KW_USE:     return parse_use();
    case TokType::KW_SELECT:  return parse_select();
    case TokType::KW_INSERT:  return parse_insert();
    case TokType::KW_DELETE:  return parse_delete();
    case TokType::KW_UPDATE:  return parse_update();
    case TokType::KW_EXIT:    return parse_exit();
    case TokType::END_OF_FILE: {
        SQLStatement stmt;
        stmt.type = StmtType::NONE;
        return stmt;
    }
    default: {
        SQLStatement stmt;
        stmt.type = StmtType::NONE;
        stmt.error_msg = String("Unexpected token: ") + t.value +
                         String(" at line ") + String(std::to_string(t.line).c_str());
        return stmt;
    }
    }
}

SQLStatement SQLParser::parse_create() {
    SQLStatement stmt;
    advance(); // CREATE

    Token t = peek();
    if (t.type == TokType::KW_DATABASE) {
        advance();
        t = expect(TokType::IDENTIFIER);
        if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
        stmt.type = StmtType::CREATE_DATABASE;
        stmt.db_name = t.value;
        return stmt;
    }

    if (t.type == TokType::KW_TABLE) {
        advance();
        t = expect(TokType::IDENTIFIER);
        if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
        stmt.type = StmtType::CREATE_TABLE;
        stmt.table_name = t.value;

        expect(TokType::LPAREN);
        if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }

        while (true) {
            Token col_name = expect(TokType::IDENTIFIER);
            if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }

            Token col_type_tok = peek();
            if (col_type_tok.type != TokType::KW_INT &&
                col_type_tok.type != TokType::KW_STRING &&
                col_type_tok.type != TokType::KW_BOOL &&
                col_type_tok.type != TokType::KW_DOUBLE) {
                stmt.error_msg = String("Expected column type but got: ") + col_type_tok.value;
                return stmt;
            }
            advance();

            stmt.col_names.push_back(col_name.value);
            stmt.col_types_str.push_back(col_type_tok.value);

            Column col(col_name.value, string_to_column_type(col_type_tok.value));
            stmt.col_defs.push_back(col);

            if (match(TokType::KW_PRIMARY)) {
                match(TokType::KW_KEY);
            }

            if (match(TokType::COMMA)) {
                continue;
            } else {
                break;
            }
        }

        expect(TokType::RPAREN);
        if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
        return stmt;
    }

    stmt.error_msg = String("Expected DATABASE or TABLE after CREATE, got: ") + t.value;
    return stmt;
}

SQLStatement SQLParser::parse_drop() {
    SQLStatement stmt;
    advance(); // DROP

    Token t = peek();
    if (t.type == TokType::KW_DATABASE) {
        advance();
        t = expect(TokType::IDENTIFIER);
        if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
        stmt.type = StmtType::DROP_DATABASE;
        stmt.db_name = t.value;
        return stmt;
    }

    if (t.type == TokType::KW_TABLE) {
        advance();
        t = expect(TokType::IDENTIFIER);
        if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
        stmt.type = StmtType::DROP_TABLE;
        stmt.table_name = t.value;
        return stmt;
    }

    stmt.error_msg = String("Expected DATABASE or TABLE after DROP, got: ") + t.value;
    return stmt;
}

SQLStatement SQLParser::parse_use() {
    SQLStatement stmt;
    advance(); // USE

    Token t = expect(TokType::IDENTIFIER);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
    stmt.type = StmtType::USE;
    stmt.db_name = t.value;
    return stmt;
}

SQLStatement SQLParser::parse_select() {
    SQLStatement stmt;
    stmt.type = StmtType::SELECT;
    advance(); // SELECT

    if (match(TokType::STAR)) {
        stmt.select_all = true;
    } else {
        stmt.select_all = false;
        while (true) {
            Token col = expect(TokType::IDENTIFIER);
            if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
            stmt.select_cols.push_back(col.value);
            if (!match(TokType::COMMA)) break;
        }
    }

    expect(TokType::KW_FROM);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }

    Token table = expect(TokType::IDENTIFIER);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
    stmt.table_name = table.value;

    parse_where_clause(stmt);

    return stmt;
}

SQLStatement SQLParser::parse_insert() {
    SQLStatement stmt;
    stmt.type = StmtType::INSERT;
    advance(); // INSERT

    expect(TokType::KW_INTO);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }

    Token table = expect(TokType::IDENTIFIER);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
    stmt.table_name = table.value;

    if (match(TokType::LPAREN)) {
        while (true) {
            Token col = expect(TokType::IDENTIFIER);
            if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
            stmt.insert_col_names.push_back(col.value);
            if (!match(TokType::COMMA)) break;
        }
        expect(TokType::RPAREN);
        if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
    }

    expect(TokType::KW_VALUES);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }

    expect(TokType::LPAREN);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }

    while (true) {
        Token val = peek();
        if (val.type == TokType::NUMBER) {
            advance();
            double d = std::strtod(val.value.c_str(), nullptr);
            stmt.insert_values.push_back(Json(d));
        } else if (val.type == TokType::STRING_LITERAL) {
            advance();
            stmt.insert_values.push_back(Json(val.value));
        } else if (val.type == TokType::IDENTIFIER) {
            String lower = val.value;
            for (size_t i = 0; i < lower.length(); ++i) {
                if (lower[i] >= 'A' && lower[i] <= 'Z') {
                    lower[i] = static_cast<char>(lower[i] + ('a' - 'A'));
                }
            }
            if (lower == "true") {
                advance();
                stmt.insert_values.push_back(Json(true));
            } else if (lower == "false") {
                advance();
                stmt.insert_values.push_back(Json(false));
            } else if (lower == "null") {
                advance();
                stmt.insert_values.push_back(Json());
            } else {
                advance();
                stmt.insert_values.push_back(Json(val.value));
            }
        } else {
            stmt.error_msg = String("Expected value but got: ") + val.value;
            return stmt;
        }
        if (!match(TokType::COMMA)) break;
    }

    expect(TokType::RPAREN);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }

    return stmt;
}

SQLStatement SQLParser::parse_delete() {
    SQLStatement stmt;
    stmt.type = StmtType::DEL;
    advance(); // DELETE

    expect(TokType::KW_FROM);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }

    Token table = expect(TokType::IDENTIFIER);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
    stmt.table_name = table.value;

    parse_where_clause(stmt);

    return stmt;
}

SQLStatement SQLParser::parse_update() {
    SQLStatement stmt;
    stmt.type = StmtType::UPDATE;
    advance(); // UPDATE

    Token table = expect(TokType::IDENTIFIER);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
    stmt.table_name = table.value;

    expect(TokType::KW_SET);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }

    Token col = expect(TokType::IDENTIFIER);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }
    stmt.set_col = col.value;

    expect(TokType::EQ);
    if (!m_error.empty()) { stmt.error_msg = m_error; return stmt; }

    Token val = peek();
    if (val.type == TokType::NUMBER) {
        advance();
        stmt.set_value = Json(std::strtod(val.value.c_str(), nullptr));
    } else if (val.type == TokType::STRING_LITERAL) {
        advance();
        stmt.set_value = Json(val.value);
    } else if (val.type == TokType::IDENTIFIER) {
        String lower = val.value;
        for (size_t i = 0; i < lower.length(); ++i) {
            if (lower[i] >= 'A' && lower[i] <= 'Z')
                lower[i] = static_cast<char>(lower[i] + ('a' - 'A'));
        }
        if (lower == "true") {
            advance();
            stmt.set_value = Json(true);
        } else if (lower == "false") {
            advance();
            stmt.set_value = Json(false);
        } else {
            advance();
            stmt.set_value = Json(val.value);
        }
    } else {
        stmt.error_msg = String("Expected value after =, got: ") + val.value;
        return stmt;
    }

    parse_where_clause(stmt);

    return stmt;
}

SQLStatement SQLParser::parse_exit() {
    SQLStatement stmt;
    stmt.type = StmtType::EXIT;
    advance();
    return stmt;
}

bool SQLParser::parse_where_clause(SQLStatement& stmt) {
    if (match(TokType::KW_WHERE)) {
        stmt.has_where = true;

        Token col = expect(TokType::IDENTIFIER);
        if (!m_error.empty()) { stmt.error_msg = m_error; return false; }
        stmt.where_col = col.value;

        Token op = peek();
        if (op.type == TokType::EQ || op.type == TokType::LT ||
            op.type == TokType::GT || op.type == TokType::LE ||
            op.type == TokType::GE || op.type == TokType::NE) {
            advance();
            stmt.where_op_str = op.value;
            stmt.where_op = token_to_condition_op(op.value);
        } else {
            stmt.error_msg = String("Expected comparison operator, got: ") + op.value;
            return false;
        }

        Token val = peek();
        if (val.type == TokType::NUMBER) {
            advance();
            stmt.where_value = Json(std::strtod(val.value.c_str(), nullptr));
        } else if (val.type == TokType::STRING_LITERAL) {
            advance();
            stmt.where_value = Json(val.value);
        } else if (val.type == TokType::IDENTIFIER) {
            String lower = val.value;
            for (size_t i = 0; i < lower.length(); ++i) {
                if (lower[i] >= 'A' && lower[i] <= 'Z')
                    lower[i] = static_cast<char>(lower[i] + ('a' - 'A'));
            }
            if (lower == "true") {
                advance();
                stmt.where_value = Json(true);
            } else if (lower == "false") {
                advance();
                stmt.where_value = Json(false);
            } else {
                advance();
                stmt.where_value = Json(val.value);
            }
        } else {
            stmt.error_msg = String("Expected value in WHERE clause, got: ") + val.value;
            return false;
        }
    }
    return true;
}

ConditionOp SQLParser::token_to_condition_op(const String& op_str) const {
    if (op_str == "=")  return ConditionOp::EQ;
    if (op_str == "!=" || op_str == "<>") return ConditionOp::NE;
    if (op_str == "<")  return ConditionOp::LT;
    if (op_str == "<=") return ConditionOp::LE;
    if (op_str == ">")  return ConditionOp::GT;
    if (op_str == ">=") return ConditionOp::GE;
    return ConditionOp::EQ;
}

ColumnType SQLParser::string_to_column_type(const String& s) {
    if (s == "int")    return ColumnType::Int;
    if (s == "string") return ColumnType::String;
    if (s == "bool")   return ColumnType::Bool;
    if (s == "double") return ColumnType::Double;
    return ColumnType::Null;
}

String SQLParser::column_type_to_string(ColumnType t) {
    switch (t) {
    case ColumnType::Int:    return "int";
    case ColumnType::String: return "string";
    case ColumnType::Bool:   return "bool";
    case ColumnType::Double: return "double";
    default:                 return "null";
    }
}
