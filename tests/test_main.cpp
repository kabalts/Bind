#include <cstdio>
#include <cstring>
#include <cassert>
#include <iostream>

#include "common/String.hpp"
#include "common/ArrayList.hpp"
#include "common/LinkedList.hpp"
#include "common/Json.hpp"
#include "common/ResultSet.hpp"
#include "storage/BPlusTree.hpp"
#include "storage/FileEngine.hpp"
#include "storage/Table.hpp"
#include "storage/Database.hpp"
#include "parser/SQLParser.hpp"
#include "executor/Executor.hpp"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST(name) \
    std::cout << "  TEST: " << name << " ... ";

#define PASS() \
    do { std::cout << "PASSED" << std::endl; ++g_tests_passed; } while(0)

#define FAIL(msg) \
    do { std::cout << "FAILED: " << msg << std::endl; ++g_tests_failed; } while(0)

static void test_string() {
    std::cout << "[String Tests]" << std::endl;

    TEST("default construction") {
        String s;
        assert(s.length() == 0);
        assert(s.empty());
        assert(std::strcmp(s.c_str(), "") == 0);
        PASS();
    }

    TEST("c-string construction") {
        String s("hello");
        assert(s.length() == 5);
        assert(s == "hello");
        PASS();
    }

    TEST("copy construction") {
        String a("world");
        String b(a);
        assert(b == "world");
        assert(a == "world");
        PASS();
    }

    TEST("concatenation") {
        String a("hello");
        String b(" world");
        String c = a + b;
        assert(c == "hello world");
        assert(c.length() == 11);
        PASS();
    }

    TEST("operator+=") {
        String s("abc");
        s += "def";
        assert(s == "abcdef");
        s += 'g';
        assert(s == "abcdefg");
        PASS();
    }

    TEST("substr") {
        String s("hello world");
        String sub = s.substr(6, 5);
        assert(sub == "world");
        String sub2 = s.substr(0, 5);
        assert(sub2 == "hello");
        PASS();
    }

    TEST("find") {
        String s("hello world");
        assert(s.find("world") == 6);
        assert(s.find("hello") == 0);
        assert(s.find("xyz") == String::npos);
        assert(s.find('w') == 6);
        PASS();
    }

    TEST("comparison") {
        String a("abc");
        String b("abd");
        String c("abc");
        assert(a < b);
        assert(a <= b);
        assert(b > a);
        assert(a == c);
        assert(a != b);
        PASS();
    }
}

static void test_array_list() {
    std::cout << "[ArrayList Tests]" << std::endl;

    TEST("default construction") {
        ArrayList<int> list;
        assert(list.size() == 0);
        assert(list.empty());
        PASS();
    }

    TEST("push_back and access") {
        ArrayList<int> list;
        list.push_back(10);
        list.push_back(20);
        list.push_back(30);
        assert(list.size() == 3);
        assert(list[0] == 10);
        assert(list[1] == 20);
        assert(list[2] == 30);
        PASS();
    }

    TEST("pop_back") {
        ArrayList<int> list;
        list.push_back(1);
        list.push_back(2);
        list.pop_back();
        assert(list.size() == 1);
        assert(list[0] == 1);
        PASS();
    }

    TEST("copy construction") {
        ArrayList<int> a;
        a.push_back(5);
        a.push_back(10);
        ArrayList<int> b(a);
        assert(b.size() == 2);
        assert(b[0] == 5);
        assert(b[1] == 10);
        PASS();
    }

    TEST("iterator") {
        ArrayList<int> list;
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        int sum = 0;
        for (auto it = list.begin(); it != list.end(); ++it) {
            sum += *it;
        }
        assert(sum == 6);
        PASS();
    }

    TEST("clear") {
        ArrayList<int> list;
        list.push_back(1);
        list.push_back(2);
        list.clear();
        assert(list.size() == 0);
        assert(list.empty());
        PASS();
    }

    TEST("reserve") {
        ArrayList<int> list;
        list.reserve(100);
        assert(list.capacity() >= 100);
        assert(list.size() == 0);
        PASS();
    }

    TEST("front and back") {
        ArrayList<int> list;
        list.push_back(10);
        list.push_back(20);
        assert(list.front() == 10);
        assert(list.back() == 20);
        PASS();
    }

    TEST("insert") {
        ArrayList<int> list;
        list.push_back(1);
        list.push_back(3);
        list.insert(list.cbegin() + 1, 2);
        assert(list.size() == 3);
        assert(list[0] == 1);
        assert(list[1] == 2);
        assert(list[2] == 3);
        PASS();
    }

    TEST("erase") {
        ArrayList<int> list;
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        list.erase(list.cbegin() + 1);
        assert(list.size() == 2);
        assert(list[0] == 1);
        assert(list[1] == 3);
        PASS();
    }

    TEST("resize") {
        ArrayList<int> list;
        list.resize(5, 42);
        assert(list.size() == 5);
        assert(list[0] == 42);
        assert(list[4] == 42);
        PASS();
    }
}

static void test_linked_list() {
    std::cout << "[LinkedList Tests]" << std::endl;

    TEST("push_back and iteration") {
        LinkedList<int> list;
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        assert(list.size() == 3);
        int expected = 1;
        for (auto it = list.begin(); it != list.end(); ++it) {
            assert(*it == expected);
            ++expected;
        }
        PASS();
    }

    TEST("push_front") {
        LinkedList<int> list;
        list.push_front(3);
        list.push_front(2);
        list.push_front(1);
        assert(list.size() == 3);
        assert(list.front() == 1);
        assert(list.back() == 3);
        PASS();
    }

    TEST("pop_back and pop_front") {
        LinkedList<int> list;
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        list.pop_back();
        assert(list.back() == 2);
        list.pop_front();
        assert(list.front() == 2);
        PASS();
    }

    TEST("insert") {
        LinkedList<int> list;
        list.push_back(1);
        list.push_back(3);
        auto it = list.begin();
        ++it;
        list.insert(it, 2);
        assert(list.size() == 3);
        auto it2 = list.begin();
        assert(*it2 == 1); ++it2;
        assert(*it2 == 2); ++it2;
        assert(*it2 == 3);
        PASS();
    }

    TEST("erase") {
        LinkedList<int> list;
        list.push_back(1);
        list.push_back(2);
        list.push_back(3);
        auto it = list.begin();
        ++it;
        list.erase(it);
        assert(list.size() == 2);
        assert(list.front() == 1);
        assert(list.back() == 3);
        PASS();
    }

    TEST("clear") {
        LinkedList<int> list;
        list.push_back(1);
        list.push_back(2);
        list.clear();
        assert(list.size() == 0);
        assert(list.empty());
        PASS();
    }
}

static void test_json() {
    std::cout << "[Json Tests]" << std::endl;

    TEST("null") {
        Json j;
        assert(j.is_null());
        PASS();
    }

    TEST("bool") {
        Json j(true);
        assert(j.is_bool());
        assert(j.as_bool() == true);
        Json jf(false);
        assert(jf.as_bool() == false);
        PASS();
    }

    TEST("number") {
        Json j(42);
        assert(j.is_number());
        assert(j.as_number() == 42.0);
        Json jd(3.14);
        assert(jd.as_number() == 3.14);
        PASS();
    }

    TEST("string") {
        Json j("hello");
        assert(j.is_string());
        assert(j.as_string() == "hello");
        PASS();
    }

    TEST("array") {
        Json arr = Json::array();
        arr.push_back(Json(1));
        arr.push_back(Json(2));
        arr.push_back(Json(3));
        assert(arr.is_array());
        assert(arr.size() == 3);
        assert(arr[0].as_number() == 1);
        PASS();
    }

    TEST("object") {
        Json obj = Json::object();
        obj.insert("name", Json("Alice"));
        obj.insert("age", Json(30));
        assert(obj.is_object());
        assert(obj.size() == 2);
        assert(obj["name"].as_string() == "Alice");
        assert(obj["age"].as_number() == 30);
        PASS();
    }

    TEST("serialize null") {
        Json j;
        String s = j.serialize();
        assert(s == "null");
        PASS();
    }

    TEST("serialize object") {
        Json obj = Json::object();
        obj.insert("key", Json("value"));
        String s = obj.serialize();
        assert(s.find("key") != String::npos);
        assert(s.find("value") != String::npos);
        PASS();
    }

    TEST("parse simple") {
        Json j = Json::parse("42");
        assert(j.is_number());
        assert(j.as_number() == 42);
        PASS();
    }

    TEST("parse string") {
        Json j = Json::parse("\"hello\"");
        assert(j.is_string());
        assert(j.as_string() == "hello");
        PASS();
    }

    TEST("parse array") {
        Json j = Json::parse("[1, 2, 3]");
        assert(j.is_array());
        assert(j.size() == 3);
        PASS();
    }

    TEST("parse object") {
        Json j = Json::parse("{\"a\": 1, \"b\": \"hello\"}");
        assert(j.is_object());
        assert(j["a"].as_number() == 1);
        assert(j["b"].as_string() == "hello");
        PASS();
    }

    TEST("roundtrip") {
        Json obj = Json::object();
        obj.insert("id", Json(1));
        obj.insert("name", Json("test"));
        String s = obj.serialize();
        Json parsed = Json::parse(s);
        assert(parsed["id"].as_number() == 1);
        assert(parsed["name"].as_string() == "test");
        PASS();
    }
}

static void test_result_set() {
    std::cout << "[ResultSet Tests]" << std::endl;

    TEST("basic operations") {
        ResultSet rs;
        rs.add_column("id", ColumnType::Int);
        rs.add_column("name", ColumnType::String);
        assert(rs.column_count() == 2);
        assert(rs.column_name(0) == "id");
        assert(rs.column_name(1) == "name");
        assert(rs.row_count() == 0);
        PASS();
    }

    TEST("add and get rows") {
        ResultSet rs;
        rs.add_column("id", ColumnType::Int);
        rs.add_column("name", ColumnType::String);

        ArrayList<Json> row1;
        row1.push_back(Json(1));
        row1.push_back(Json("Alice"));
        rs.add_row(row1);

        ArrayList<Json> row2;
        row2.push_back(Json(2));
        row2.push_back(Json("Bob"));
        rs.add_row(std::move(row2));

        assert(rs.row_count() == 2);
        assert(rs.get_value(0, 0).as_number() == 1);
        assert(rs.get_value(0, 1).as_string() == "Alice");
        assert(rs.get_value(1, 0).as_number() == 2);
        PASS();
    }
}

static void test_file_engine() {
    std::cout << "[FileEngine Tests]" << std::endl;

    TEST("create and open") {
        FileEngine engine;
        assert(engine.open("test_fe.dat"));
        assert(engine.is_open());
        assert(engine.record_count() == 0);
        engine.close();
        PASS();
    }

    TEST("append and read") {
        FileEngine engine;
        engine.open("test_fe2.dat");

        FileEngine::Record rec;
        rec.id = 1;
        rec.deleted = false;
        std::strcpy(rec.data, "hello");

        int64_t idx = engine.append(rec);
        assert(idx == 0);
        assert(engine.record_count() == 1);

        FileEngine::Record rec2;
        assert(engine.read_record(0, rec2));
        assert(rec2.id == 1);
        assert(std::strcmp(rec2.data, "hello") == 0);
        engine.close();
        PASS();
    }

    TEST("delete mark") {
        FileEngine engine;
        engine.open("test_fe3.dat");

        FileEngine::Record rec;
        rec.id = 100;
        rec.deleted = false;
        engine.append(rec);

        int64_t offset = 0;
        assert(engine.delete_at(offset));

        FileEngine::Record rec2;
        engine.read_at(offset, rec2);
        assert(rec2.deleted == true);
        engine.close();
        PASS();
    }

    std::remove("test_fe.dat");
    std::remove("test_fe2.dat");
    std::remove("test_fe3.dat");
}

static void test_bplus_tree() {
    std::cout << "[BPlusTree Tests]" << std::endl;

    TEST("create and open") {
        BPlusTree<int32_t, int64_t> tree;
        assert(tree.open("test_btree.idx"));
        assert(tree.is_open());
        tree.close();
        PASS();
    }

    TEST("insert and search") {
        BPlusTree<int32_t, int64_t> tree;
        tree.open("test_btree2.idx");

        assert(tree.insert(1, 100));
        assert(tree.insert(2, 200));
        assert(tree.insert(3, 300));

        int64_t val;
        assert(tree.search(1, val)); assert(val == 100);
        assert(tree.search(2, val)); assert(val == 200);
        assert(tree.search(3, val)); assert(val == 300);
        assert(!tree.search(99, val));
        tree.close();
        PASS();
    }

    TEST("insert many and search") {
        BPlusTree<int32_t, int64_t> tree;
        tree.open("test_btree3.idx");

        for (int32_t i = 0; i < 100; ++i) {
            assert(tree.insert(i, i * 10));
        }

        int64_t val;
        for (int32_t i = 0; i < 100; ++i) {
            assert(tree.search(i, val));
            assert(val == i * 10);
        }
        tree.close();
        PASS();
    }

    TEST("remove") {
        BPlusTree<int32_t, int64_t> tree;
        tree.open("test_btree4.idx");

        for (int32_t i = 0; i < 50; ++i) {
            tree.insert(i, i * 100);
        }

        assert(tree.remove(25));
        int64_t val;
        assert(!tree.search(25, val));
        assert(tree.search(0, val));
        assert(tree.search(49, val));
        tree.close();
        PASS();
    }

    TEST("range search") {
        BPlusTree<int32_t, int64_t> tree;
        tree.open("test_btree5.idx");

        for (int32_t i = 0; i < 20; ++i) {
            tree.insert(i, i * 10);
        }

        int64_t results[20];
        int count = tree.range_search(5, 10, results, 20);
        assert(count == 6);
        for (int j = 0; j < count; ++j) {
            assert(results[j] == (5 + j) * 10);
        }
        tree.close();
        PASS();
    }

    std::remove("test_btree.idx");
    std::remove("test_btree2.idx");
    std::remove("test_btree3.idx");
    std::remove("test_btree4.idx");
    std::remove("test_btree5.idx");
}

static void test_sql_parser() {
    std::cout << "[SQL Parser Tests]" << std::endl;

    SQLParser parser;

    TEST("create database") {
        SQLStatement stmt = parser.parse("create database mydb");
        assert(stmt.type == StmtType::CREATE_DATABASE);
        assert(stmt.db_name == "mydb");
        PASS();
    }

    TEST("drop database") {
        SQLStatement stmt = parser.parse("drop database mydb");
        assert(stmt.type == StmtType::DROP_DATABASE);
        assert(stmt.db_name == "mydb");
        PASS();
    }

    TEST("use database") {
        SQLStatement stmt = parser.parse("use mydb");
        assert(stmt.type == StmtType::USE);
        assert(stmt.db_name == "mydb");
        PASS();
    }

    TEST("create table") {
        SQLStatement stmt = parser.parse("create table users (id int primary, name string)");
        assert(stmt.type == StmtType::CREATE_TABLE);
        assert(stmt.table_name == "users");
        assert(stmt.col_names.size() == 2);
        assert(stmt.col_names[0] == "id");
        assert(stmt.col_names[1] == "name");
        PASS();
    }

    TEST("drop table") {
        SQLStatement stmt = parser.parse("drop table users");
        assert(stmt.type == StmtType::DROP_TABLE);
        assert(stmt.table_name == "users");
        PASS();
    }

    TEST("select all") {
        SQLStatement stmt = parser.parse("select * from users");
        assert(stmt.type == StmtType::SELECT);
        assert(stmt.select_all == true);
        assert(stmt.table_name == "users");
        PASS();
    }

    TEST("select columns") {
        SQLStatement stmt = parser.parse("select id, name from users");
        assert(stmt.type == StmtType::SELECT);
        assert(stmt.select_all == false);
        assert(stmt.select_cols.size() == 2);
        assert(stmt.select_cols[0] == "id");
        assert(stmt.select_cols[1] == "name");
        PASS();
    }

    TEST("select with where") {
        SQLStatement stmt = parser.parse("select * from users where id = 1");
        assert(stmt.type == StmtType::SELECT);
        assert(stmt.has_where == true);
        assert(stmt.where_col == "id");
        assert(stmt.where_op == ConditionOp::EQ);
        PASS();
    }

    TEST("select with where string") {
        SQLStatement stmt = parser.parse("select * from users where name = 'Alice'");
        assert(stmt.has_where == true);
        assert(stmt.where_col == "name");
        assert(stmt.where_value.as_string() == "Alice");
        PASS();
    }

    TEST("insert") {
        SQLStatement stmt = parser.parse("insert into users values (1, 'Alice')");
        assert(stmt.type == StmtType::INSERT);
        assert(stmt.table_name == "users");
        assert(stmt.insert_values.size() == 2);
        PASS();
    }

    TEST("delete") {
        SQLStatement stmt = parser.parse("delete from users where id = 1");
        assert(stmt.type == StmtType::DELETE);
        assert(stmt.table_name == "users");
        assert(stmt.has_where == true);
        PASS();
    }

    TEST("update") {
        SQLStatement stmt = parser.parse("update users set name = 'Bob' where id = 1");
        assert(stmt.type == StmtType::UPDATE);
        assert(stmt.table_name == "users");
        assert(stmt.set_col == "name");
        assert(stmt.set_value.as_string() == "Bob");
        PASS();
    }

    TEST("exit") {
        SQLStatement stmt = parser.parse("exit");
        assert(stmt.type == StmtType::EXIT);
        PASS();
    }

    TEST("parse error") {
        SQLStatement stmt = parser.parse("invalid sql statement xyz");
        assert(!stmt.error_msg.empty() || stmt.type == StmtType::NONE);
        PASS();
    }
}

static void test_executor_integration() {
    std::cout << "[Executor Integration Tests]" << std::endl;

    Executor executor("test_data");

    TEST("create database and use") {
        ResultSet rs = executor.execute_sql("create database testdb");
        String err = executor.error_message();
        assert(err.empty());

        rs = executor.execute_sql("use testdb");
        err = executor.error_message();
        assert(err.empty());
        PASS();
    }

    TEST("create table") {
        ResultSet rs = executor.execute_sql("create table person (id int primary, name string)");
        String err = executor.error_message();
        assert(err.empty());
        PASS();
    }

    TEST("insert rows") {
        ResultSet rs = executor.execute_sql("insert into person values (1, 'Alice')");
        assert(executor.error_message().empty());

        rs = executor.execute_sql("insert into person values (2, 'Bob')");
        assert(executor.error_message().empty());

        rs = executor.execute_sql("insert into person values (3, 'Charlie')");
        assert(executor.error_message().empty());
        PASS();
    }

    TEST("insert duplicate PK") {
        ResultSet rs = executor.execute_sql("insert into person values (1, 'DupAlice')");
        assert(!executor.error_message().empty());

        rs = executor.execute_sql("insert into person values (2, 'DupBob')");
        assert(!executor.error_message().empty());

        rs = executor.execute_sql("select * from person");
        assert(executor.error_message().empty());
        assert(rs.row_count() == 3);
        PASS();
    }

    TEST("delete and reinsert same PK") {
        ResultSet rs = executor.execute_sql("delete from person where id = 1");
        assert(executor.error_message().empty());

        rs = executor.execute_sql("insert into person values (1, 'AliceNew')");
        assert(executor.error_message().empty());

        rs = executor.execute_sql("select * from person where id = 1");
        assert(executor.error_message().empty());
        assert(rs.row_count() == 1);
        assert(rs.get_value(0, 1).as_string() == "AliceNew");
        PASS();
    }

    TEST("select all") {
        ResultSet rs = executor.execute_sql("select * from person");
        assert(executor.error_message().empty());
        assert(rs.row_count() == 3);
        PASS();
    }

    TEST("select with where") {
        ResultSet rs = executor.execute_sql("select * from person where id = 1");
        assert(executor.error_message().empty());
        assert(rs.row_count() == 1);
        PASS();
    }

    TEST("update") {
        ResultSet rs = executor.execute_sql("update person set name = 'Alice2' where id = 1");
        assert(executor.error_message().empty());

        rs = executor.execute_sql("select name from person where id = 1");
        assert(executor.error_message().empty());
        if (rs.row_count() > 0) {
            assert(rs.get_value(0, 0).as_string() == "Alice2");
        }
        PASS();
    }

    TEST("delete") {
        ResultSet rs = executor.execute_sql("delete from person where id = 3");
        assert(executor.error_message().empty());

        rs = executor.execute_sql("select * from person");
        assert(executor.error_message().empty());
        assert(rs.row_count() == 2);
        PASS();
    }

    TEST("error: table not found") {
        ResultSet rs = executor.execute_sql("select * from nonexistent");
        assert(!executor.error_message().empty());
        PASS();
    }

    TEST("error: syntax error") {
        ResultSet rs = executor.execute_sql("invalid syntax xyz 123");
        assert(!executor.error_message().empty());
        PASS();
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Bind Integration Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    test_string();
    std::cout << std::endl;
    test_array_list();
    std::cout << std::endl;
    test_linked_list();
    std::cout << std::endl;
    test_json();
    std::cout << std::endl;
    test_result_set();
    std::cout << std::endl;
    test_file_engine();
    std::cout << std::endl;
    test_bplus_tree();
    std::cout << std::endl;
    test_sql_parser();
    std::cout << std::endl;
    test_executor_integration();
    std::cout << std::endl;

    std::cout << "========================================" << std::endl;
    std::cout << "  Results: " << g_tests_passed << " passed, "
              << g_tests_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return g_tests_failed > 0 ? 1 : 0;
}
