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
        if (!file.good()) {
            throw std::runtime_error("Failed to open: " + path.string());
        }

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
}


#endif //BOOST_CRC_TEST_AUXULLARY_H
