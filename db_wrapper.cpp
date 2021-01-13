//
// Created by swql on 1/10/21.
//

#include "db_wrapper.h"

std::vector<std::any> db_wrapper::exec_get_row(const std::vector<std::type_index> &layout) {
        std::vector<std::any> row;
        row.reserve(layout.size());
        size_t col = 0; // TODO OPTIMIZE
        for (auto& l : layout){
            if (l == std::type_index { typeid(int32_t) }){
                row.push_back(sqlite3_column_int(current_stmt, col));
            }
            else if (l == std::type_index { typeid(int64_t) }){
                row.push_back(sqlite3_column_int64(current_stmt, col));
            }
            else if (l == std::type_index { typeid(std::string) }){
                auto res = sqlite3_column_text(current_stmt, col);
                if (res == nullptr) row.push_back(std::string {"NULL"});
                else {
                    row.push_back(std::string (reinterpret_cast<const char*>(res),
                                               sqlite3_column_bytes(current_stmt, col)));
                }
            }
            else if (l == std::type_index { typeid(std::wstring) }){
                const void* res = sqlite3_column_text16(current_stmt, col);
                if (res == nullptr) row.push_back(std::wstring {L""});
                else row.push_back(std::wstring {reinterpret_cast<const wchar_t*>(res)});
            }
            else if (l == std::type_index { typeid(float) }){
                row.push_back(sqlite3_column_double(current_stmt, col));
            }
            else {
                assert(false);
            }
            col++;
        }

        return row;
    }

void db_wrapper::exec_noget(const std::string &sql_statement) {
    auto status = sqlite3_prepare(db, sql_statement.c_str(), sql_statement.size(), &current_stmt, nullptr);
    if (status != SQLITE_OK) {
        std::cout << sqlite3_errmsg(db) << std::endl;
        std::cout << "SQL statement: " << sql_statement << std::endl;
        throw std::runtime_error("Error while preparing sqlite statement");
    }

    status = sqlite3_step(current_stmt);
    bool done = false;
    while (!done){
        switch (status) {
            case SQLITE_DONE:
                done = true;
                break;
            case SQLITE_BUSY:
                break;
            default:
                std::cout << sqlite3_errmsg(db) << std::endl;
                throw std::runtime_error("Error while query");
        }
    }


    sqlite3_finalize(current_stmt);
}

std::vector<std::vector<std::any>>
db_wrapper::exec(const std::string &sql_statement, const std::vector<std::type_index> &layout) {
    auto status = sqlite3_prepare_v2(db, sql_statement.c_str(), -1, &current_stmt, nullptr);
    if (status != SQLITE_OK){
        std::cout << sqlite3_errmsg(db) << std::endl;
        std::cout << "SQL statement: " << sql_statement << std::endl;
        throw std::runtime_error("Error while preparing sqlite statement");
    }

    std::vector<std::vector<std::any>> table;
    bool run = true;
    while (run){
        status = sqlite3_step(current_stmt);
        switch (status) {
            case SQLITE_DONE:
                run = false;
                break;
            case SQLITE_ROW:
                table.push_back(exec_get_row(layout));
                break;
            case SQLITE_BUSY:
                break;
            default:
                throw std::runtime_error("Error while query");
        }
    }

    sqlite3_finalize(current_stmt);
    return table;
}

void db_wrapper::flush() {
    sqlite3_close(db);
    sqlite3_open(db_path.c_str(), &db);
}

void db_wrapper::begin_transaction() {
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
}

void db_wrapper::end_transaction() {
    sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
}
