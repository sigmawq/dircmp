//
// Created by swql on 1/10/21.
//

#include "fs_view.h"
#include "hashf.h"

void fs_view::loadf(const std::string &path) {
    db_wrapper dbw { path };

    const std::string q1 { "SELECT * from directory_structure_list" };
    const std::string q2 { "SELECT * from fd_table" };

    auto t1 = dbw.exec(q1, std::vector<std::type_index> { INT64_T, INT64_T } );
    dir_structure_table.reserve(t1.size());
    for (auto& el : t1){

        dir_structure_table.emplace_back(
                std::any_cast<long long>(el[0]),
                std::any_cast<long long>(el[1]) );
    }

    auto t2 = dbw.exec(q2, std::vector<std::type_index> { INT64_T, STR_T, STR_T, INT32_T });
    fd_list_id_sorted.reserve(t2.size());
    for (auto& el : t2){
        fd_list_id_sorted.emplace_back(
                std::any_cast<long long>(el[0]),
                std::any_cast<std::string>(el[1]),
                std::any_cast<std::string>(el[2]),
                std::any_cast<int32_t>(el[3]));
    }

    copy_sort_every_table();
}

void fs_view::loadf_out(const std::string& path) {
    create_file(path);
    db_wrapper dbw { path };

    const std::string fd_list_sql {
            R"(CREATE TABLE "directory_structure_list" (
	                "parent_dir_ID"	INTEGER,
                    "child_dir_ID"	INTEGER);
            )"
    };

    const std::string dirh_sql {
            R"(CREATE TABLE "fd_table" (
                "dir_ID"	INTEGER NOT NULL,
                "path"	TEXT,
	            "fname"	TEXT,
	            "hash"	INTEGER,
	            PRIMARY KEY("dir_ID")
                ))"
    };

    dbw.exec_noget(fd_list_sql);
    dbw.exec_noget(dirh_sql);
    dbw.flush();

    std::string buffer_str;
    for (int i = 0; i < fd_list_hash_sorted.size(); i++){
        str_compose(buffer_str,
                    "INSERT INTO fd_table VALUES (",
                    std::to_string(fd_list_hash_sorted[i].dir_ID), ",",
                    "'", fd_list_hash_sorted[i].path, "'", ",",
                    "'", fd_list_hash_sorted[i].fname, "'", ',',
                    std::to_string(fd_list_hash_sorted[i].hash), ')');
        dbw.exec_noget(buffer_str);
        buffer_str.clear();
    }

    for (int i = 0; i < dir_structure_table.size(); i++){
        str_compose(buffer_str,
                    "INSERT INTO directory_structure_list VALUES (",
                    std::to_string(dir_structure_table[i].parent), ",",
                    std::to_string(dir_structure_table[i].child), ")");
        dbw.exec_noget(buffer_str);
        buffer_str.clear();
    }
}

void fs_view::form(const std::filesystem::path &path) {
    this->root_path = path;

    // Save current path to restore it later
    auto retained_path = std::filesystem::current_path();
    auto abs = std::filesystem::absolute(path).parent_path();
    std::filesystem::current_path(abs);

    add_new_fd(path.filename(), "", 0);
    size_t current_dir_id = this->global_fd_id - 1;

    auto this_dir = get_dirfd_sorted(path.filename());
    std::vector<int32_t> hashes;
    hashes.reserve(this_dir.size());

    for (auto& fname : this_dir){
        if (std::filesystem::is_directory(fname)){
            // Recursive call to the same function
            hashes.push_back(form_internal(fname, current_dir_id));
        }
        else {
            auto hash = crc_32_file(fname);
            add_new_fd(fname, fname.filename(), hash);
            add_dirh_relation(current_dir_id, global_fd_id - 1);
            hashes.push_back(hash);
        }
    }

    int32_t dir_hash = crc_32(reinterpret_cast<const char *>(hashes.data()),
                              (hashes.size() * sizeof(decltype(hashes)::value_type)));
    get_fd_by_id(current_dir_id).hash = dir_hash;

    copy_sort_every_table();

    // Restore path
    std::filesystem::current_path(retained_path);
}

std::vector<std::filesystem::path> fs_view::get_dirfd_sorted(const std::filesystem::path &path) {
    std::vector<std::filesystem::path> directory_fd_list;
    for (auto& fd : std::filesystem::directory_iterator(path)){
        directory_fd_list.push_back(fd.path());
    }

    std::sort(directory_fd_list.begin(), directory_fd_list.end());
    return directory_fd_list;
}

void fs_view::add_new_fd(const std::string &path, const std::string &fname, int32_t hash) {
    fd_list_id_sorted.emplace_back(global_fd_id, path, fname, hash);
    this->global_fd_id++;
}

int32_t fs_view::form_internal(const std::filesystem::path &path, size_t parent) {
    add_new_fd(path, "", 0);
    size_t current_dir_id = this->global_fd_id - 1;
    add_dirh_relation(parent, current_dir_id);

    auto this_dir = get_dirfd_sorted(path);
    std::vector<int32_t> hashes;
    hashes.reserve(this_dir.size());

    for (auto& fname : this_dir){
        if (std::filesystem::is_directory(fname)){
            // Recursive call to the same function
            hashes.push_back(form_internal(fname, current_dir_id));
        }
        else {
            auto hash = crc_32_file(fname);
            add_new_fd(fname, fname.filename(), hash);
            add_dirh_relation(current_dir_id, global_fd_id - 1);
            hashes.push_back(hash);
        }
    }

    get_fd_by_id(current_dir_id).hash = crc_32(
            reinterpret_cast<const char *>(hashes.data()),
            (hashes.size() * sizeof(decltype(hashes)::value_type)));
}

void fs_view::add_dirh_relation(size_t parent, size_t child) {
    dir_structure_table.emplace_back(parent, child);
}

fd_record &fs_view::get_fd_by_id(size_t dir_id) {
    return fd_list_id_sorted[dir_id];
}

FSTATUS fs_view::get_file_status(std::string &path, int32_t hash, bool if_dir) const {
    struct cmp{
        bool operator()(fd_record const& lhs, std::string const& rhs) const {
            return lhs.path < rhs;
        }

        bool operator()(std::string const& lhs, fd_record const& rhs) const {
            return lhs < rhs.path;
        }
    };

    auto found_path_fd = std::equal_range(fd_list_path_sorted.begin(), fd_list_path_sorted.end(), path, cmp{}).first;
    if (found_path_fd == fd_list_path_sorted.end()){
        return FSTATUS::NOT_FOUND;
    }
    else if (found_path_fd->is_dir() == if_dir){
        if (found_path_fd->hash != hash){
            return FSTATUS::SAME_NAME_CONTENT_CHANGED;
        }
        else {
            return FSTATUS::SAME_NAME_SAME_CONTENT;
        }
    }
    else {
        return FSTATUS::NOT_FOUND;
    }
}

std::vector<size_t> fs_view::get_directory_children(size_t dir_id) {
    auto res = std::equal_range(dir_structure_table.begin(), dir_structure_table.end(), dir_id, dirh_cmp{});
    if (res.first == dir_structure_table.end()) return std::vector<size_t   > {};
    std::vector<size_t> children;

    size_t d = std::distance(res.first, res.second);
    children.resize(d);

   for (size_t i = 0; i < children.size(); i++) {
       children[i] = (res.first + i)->child;
   }
   return children;
}

std::vector<std::reference_wrapper<fd_record>> fs_view::get_fd_records_by_ids(std::vector<size_t> ids) {
    std::vector<std::reference_wrapper<fd_record>> result;
    result.reserve(ids.size());
    for (auto i : ids){
        result.emplace_back(get_fd_by_id(i));
    }
    return result;
}

size_t fs_view::get_fd_count(const fs_view &fd) {
    return fd.fd_list_id_sorted.size();
}

fd_record& fs_view::get_fd_by_path(const std::filesystem::path &path) {
    struct cmp{
        bool operator()(std::string const& lhs, fd_record const& rhs){
            return lhs < rhs.path;
        }
        bool operator()(fd_record const& lhs, std::string const& rhs){
            return lhs.path < rhs;
        }
    };

    return *std::equal_range(fd_list_path_sorted.begin(), fd_list_path_sorted.end(), path.string(), cmp{}).first;
}

size_t fs_view::get_children_count(const size_t dir_id) const {
    auto res = std::equal_range(dir_structure_table.begin(), dir_structure_table.end(), dir_id, dirh_cmp{});
    if (res.first == dir_structure_table.end()) return 0;
    return std::distance(res.first, res.second) + 1;
}

void fs_view::copy_sort_every_table() {
    // Sort id table
    std::sort(fd_list_id_sorted.begin(), fd_list_id_sorted.end(), [](const fd_record& fdr1, const fd_record& fdr2){
        return fdr1.dir_ID < fdr2.dir_ID;
    });

    // Create and sort hash array
    fd_list_hash_sorted = fd_list_id_sorted;
    std::sort(fd_list_hash_sorted.begin(), fd_list_hash_sorted.end(), [](const fd_record& fdr1, const fd_record& fdr2){
        return fdr1.hash < fdr2.hash;
    });
    fd_list_path_sorted = fd_list_hash_sorted;

    // Create and sort path array
    std::sort(fd_list_path_sorted.begin(), fd_list_path_sorted.end(), [](const fd_record& fdr1, const fd_record& fdr2){
        return fdr1.path < fdr2.path;
    });

    // Sort directory relation array
    std::sort(dir_structure_table.begin(), dir_structure_table.end(), [](const dirh_record &l, const dirh_record &r){
        return l.parent < r.parent;
    });
}

void fd_record::unify_root(std::vector<fd_record> &storage){
}