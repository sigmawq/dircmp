//
// Created by swql on 1/10/21.
//

#ifndef BOOST_CRC_TEST_HASHF_H
#define BOOST_CRC_TEST_HASHF_H

#include <boost/crc.hpp>
#include <cstring>


namespace{
    // Calculate CRC32 of a buffer
    int32_t crc_32(const char* buffer, size_t length){
        boost::crc_32_type result;
        result.process_bytes(buffer, length);
        int32_t hash = result.checksum();
        return hash;
    }

    // Calculate CRC32 of a buffer
    int32_t crc_32(std::string& buffer){
        boost::crc_32_type result;
        result.process_bytes(buffer.c_str(), buffer.size());
        return result.checksum();
    }

    // Calculate CRC32 of a file
    int32_t crc_32_file(const std::filesystem::path& file_path){
        std::string buffer;
        get_file_content(file_path, buffer);
        boost::crc_32_type result;
        result.process_bytes(buffer.c_str(), buffer.size());
        return result.checksum();
    }
}

#endif //BOOST_CRC_TEST_HASHF_H
