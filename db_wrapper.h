//
// Created by swql on 1/10/21.
//

#ifndef BOOST_CRC_TEST_DB_WRAPPER_H
#define BOOST_CRC_TEST_DB_WRAPPER_H

#include "sqlite3.h"
#include <string>
#include <any>
#include <vector>
#include <type_traits>
#include <typeindex>
#include <cassert>
#include <stdexcept>
#include <iostream>

class db_wrapper {
    sqlite3 *db;
    std::string db_path;
    sqlite3_stmt *current_stmt;

    std::vector<std::any> exec_get_row(const std::vector<std::type_index>& layout);
public:
    db_wrapper(const std::string& path) : db_path(path) {
        auto status = sqlite3_open(path.c_str(), &db);
        if (status != SQLITE_OK){
            std::runtime_error("Failed to open DB");
        }
    }

    ~db_wrapper() {
        sqlite3_close(db);
    }

    // Execute SQl. No return statement
    void exec_noget(const std::string& sql_statement);

    void begin_transaction();

    void end_transaction();

    // Execute SQL. Returned relation is determined by layout vector
    std::vector<std::vector<std::any>> exec(const std::string& sql_statement, const std::vector<std::type_index>& layout);

    // Flush to disk
    void flush();
};


#endif //BOOST_CRC_TEST_DB_WRAPPER_H
