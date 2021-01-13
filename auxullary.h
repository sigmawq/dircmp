//
// Created by swql on 1/10/21.
//

#ifndef BOOST_CRC_TEST_AUXULLARY_H
#define BOOST_CRC_TEST_AUXULLARY_H

#include <filesystem>
#include <fstream>

namespace{
    void get_file_content(const std::filesystem::path& path, std::string& buffer){
        std::ifstream file;
        file.open(path);
        if (!file.good()) throw std::runtime_error("Failed to open the file");

        size_t fsize = std::filesystem::file_size(path);
        buffer.resize(fsize);
        file.read(buffer.data(), buffer.size());
    }

    void create_file(const std::string& path){
        std::ofstream ofs;
        ofs.open(path, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }

// Base case
    void _str_compose(std::string& str) { }

    template<typename Head, typename ...Tail>
    void _str_compose(std::string& str, Head h, Tail... t){
        str += h;
        _str_compose(str, t...);
    }

    template<typename ...Args>
    void str_compose(std::string& str, Args... args){
        _str_compose(str, args...);
    }

    sqlite3* db_open(std::string path){
        sqlite3 *db;
        auto status = sqlite3_open("kal", &db);
        if (status != SQLITE_OK){
            std::runtime_error("Failed to open DB");
        }
    }

    sqlite3_stmt* db_exec(sqlite3 *db, std::string& sql_statement){
        sqlite3_stmt *st;
        auto status = sqlite3_prepare_v2(db, sql_statement.c_str(), sql_statement.size(), &st, nullptr);
        if (status != SQLITE_OK) throw std::runtime_error("Error while preparing sqlite statement");
        return st;
    }
}


#endif //BOOST_CRC_TEST_AUXULLARY_H
