#pragma once

#include "common/String.hpp"
#include "common/ArrayList.hpp"
#include "common/Json.hpp"
#include "storage/Table.hpp"

enum class TokenType {
    KW_CREATE, KW_DROP, KW_USE, KW_SELECT, KW_INSERT, KW_UPDATE, KW_DELETE,
    KW_FROM, KW_WHERE, KW_SET, KW_VALUES, KW_PRIMARY, KW_KEY,
    KW_DATABASE, KW_TABLE, KW_INTO, KW_EXIT,
    KW_INT, KW_STRING, KW_BOOL, KW_DOUBLE,
    IDENTIFIER, NUMBER, STRING_LITERAL,
    LPAREN, RPAREN, COMMA, SEMICOLON, STAR, EQ, LT, GT, LE, GE, NE, DOT,
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    String    value;
    int       line;
    int       col;
};

enum class StmtType {
    CREATE_DATABASE,
    DROP_DATABASE,
    USE,
    CREATE_TABLE,
    DROP_TABLE,
    SELECT,
    INSERT,
    DELETE,
    UPDATE,
    EXIT,
    NONE
};

struct SQLStatement {
    StmtType          type;

    String            db_name;
    String            table_name;

    ArrayList<String> select_cols;
    bool              select_all;

    ArrayList<String> col_names;
    ArrayList<String> col_types_str;
    ArrayList<Column> col_defs;

    ArrayList<String> insert_col_names;
    ArrayList<Json>   insert_values;

    String            set_col;
    Json              set_value;

    bool              has_where;
    String            where_col;
    String            where_op_str;
    Json              where_value;
    ConditionOp       where_op;

    String            error_msg;

    SQLStatement() : type(StmtType::NONE), select_all(false),
                     has_where(false), set_col(), set_value(),
                     where_op(ConditionOp::EQ) {}
};

class SQLParser {
public:
    SQLParser();

    SQLStatement parse(const String& sql);

    const String& error_message() const { return m_error; }

private:
    String            m_sql;
    const char*       m_pos;
    int               m_line;
    int               m_col;
    String            m_error;
    ArrayList<Token>  m_tokens;
    size_t            m_token_idx;

    void set_error(const String& msg);

    bool lex();
    void skip_whitespace();
    Token lex_identifier_or_keyword();
    Token lex_number();
    Token lex_string();
    Token lex_symbol();

    Token peek() const;
    Token advance();
    Token expect(TokenType type);
    bool  match(TokenType type);
    bool  check(TokenType type) const;

    SQLStatement parse_statement();
    SQLStatement parse_create();
    SQLStatement parse_drop();
    SQLStatement parse_use();
    SQLStatement parse_select();
    SQLStatement parse_insert();
    SQLStatement parse_delete();
    SQLStatement parse_update();
    SQLStatement parse_exit();

    bool parse_where_clause(SQLStatement& stmt);
    ConditionOp token_to_condition_op(const String& op_str) const;

    static ColumnType string_to_column_type(const String& s);
    static String     column_type_to_string(ColumnType t);
};
