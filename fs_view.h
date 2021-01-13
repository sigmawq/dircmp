//
// Created by swql on 1/10/21.
//

#ifndef BOOST_CRC_TEST_FS_VIEW_H
#define BOOST_CRC_TEST_FS_VIEW_H

#include "db_wrapper.h"
#include "auxullary.h"
#include "algorithm"

#define INT32_T std::type_index { typeid(int32_t) }
#define INT64_T std::type_index { typeid(int64_t) }
#define STR_T std::type_index { typeid(std::string) }
#define WSTR_T std::type_index { typeid(std::wstring) }

enum FSTATUS { SAME_NAME_SAME_CONTENT, SAME_NAME_CONTENT_CHANGED, NOT_FOUND };



struct dirh_record{
    int64_t parent;
    int64_t child;

    dirh_record() {};
    dirh_record(int64_t parent, int64_t child) : parent(parent), child(child) {}
};

struct fd_record {
    int64_t dir_ID;
    std::filesystem::path path;
    std::string fname;
    int32_t hash;

    bool is_dir() const { return fname.empty(); }

    fd_record() {}

    fd_record(int64_t dir_ID, const std::string path, const std::string fname, int32_t hash) :
            dir_ID(dir_ID), path(path), fname(fname), hash(hash) {};

    // Comparison engine should treat roots of both views as being one root folder.
    // It makes little sense to say that all files and directories from path ROOT were
    // moved to path ROOT2
    static void unify_root(std::vector<fd_record> &storage);
};

class fs_view{

    size_t global_fd_id = 0;

    // Get all files and/or directories at path. Sorted.
    static std::vector<std::filesystem::path> get_dirfd_sorted(const std::string& path);

    // Add file or directory. Return id of added fd_record. Completely controls fd ID assignment.
    void add_new_fd(const std::string& path, const std::string& fname, int32_t hash);

    void add_dirh_relation(size_t parent, size_t child);

    // Assumes id table is generated. Copies id to hash and path and sorts them.
    void copy_sort_every_table();

    // Internal form function. Returns hash of formed directory
    int32_t form_internal(const std::filesystem::path& path, size_t parent_id);
public:
    std::vector<fd_record> fd_list_id_sorted;
    std::vector<fd_record> fd_list_hash_sorted;
    std::vector<fd_record> fd_list_path_sorted;
    std::vector<dirh_record> dir_structure_table;
    std::filesystem::path root_path;

    // Load existing fs_view from disk to program
    void loadf(const std::string& path);

    // Load existing fs_view to file
    void loadf_out(const std::string& path);

    // Create fs_view of path
    void form(const std::filesystem::path& path);

    fd_record& get_fd_by_id(size_t dir_id);

    FSTATUS get_file_status(std::string& path, int32_t hash, bool if_dir) const;

    std::vector<std::filesystem::path> get_dirfd_sorted(const std::filesystem::path &path);

    // Returns IDs of children of given directory
    std::vector<size_t> get_directory_children(size_t dir_id);

    // Returns reference array to provided ids
    std::vector<std::reference_wrapper<fd_record>> get_fd_records_by_ids(std::vector<size_t> ids);

    static size_t get_fd_count(const fs_view &fd);

    fd_record& get_fd_by_path(const std::filesystem::path &path);

    size_t get_children_count(const size_t dir_id) const;

    };

struct dirh_cmp{
    bool operator()(dirh_record const& lhs, size_t rhs) const {
        return lhs.parent < rhs;
    }

    bool operator()(const size_t lhs, dirh_record const& rhs) const {
        return lhs < rhs.parent;
    }
};

#endif //BOOST_CRC_TEST_FS_VIEW_H
