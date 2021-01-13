//
// Created by swql on 1/10/21.
//

#ifndef BOOST_CRC_TEST_FS_COMPARATOR_H
#define BOOST_CRC_TEST_FS_COMPARATOR_H

#include "fs_view.h"
#include <list>

typedef std::pair<std::vector<std::reference_wrapper<fd_record>>, std::vector<std::reference_wrapper<fd_record>>> df_container;

struct moved_record {
    std::filesystem::path path_old;
    std::filesystem::path path_new;

    moved_record(const std::string &old, const std::string &new_) : path_old(old), path_new(new_) {}
};

class fs_comparator {
    fs_view& first;
    fs_view& second;

    std::list<std::reference_wrapper<fd_record>> potentially_removed_files;
    std::list<std::reference_wrapper<fd_record>> potentially_removed_directories;
    std::list<std::reference_wrapper<fd_record>> potentially_added_files;
    std::list<std::reference_wrapper<fd_record>> potentially_added_directories;
    std::list<std::reference_wrapper<fd_record>> changed_files;
    std::list<std::reference_wrapper<fd_record>> changed_directories;

    std::list<moved_record> moved_and_renamed_files;
    std::list<moved_record> moved_and_renamed_directories;
    std::list<moved_record> moved_files;
    std::list<moved_record> moved_disrectories;
    std::list<moved_record> renamed_files;
    std::list<moved_record> renamed_disrectories;


    // Separates directory and files into 2 different containers, sorts them and returns.
    static df_container perform_directory_vs_file_separation(std::vector<std::reference_wrapper<fd_record>> &vec);

    // Does file to file and directory to directory intersection by path.
    static df_container get_df_intersection(df_container& first, df_container& second);

    // Does file to file and dir to dir difference so that C = B/A. Writes results to provided vectors
    static void get_df_difference(df_container &B, df_container &A,
                                  std::list<std::reference_wrapper<fd_record>> &files,
                                  std::list<std::reference_wrapper<fd_record>> &directories);

    // Perform recursive addition of every file and directory to desired storages
    void directory_recursive_ops(const fd_record &dir,
                                 std::list<std::reference_wrapper<fd_record>> &storage_f,
                                 std::list<std::reference_wrapper<fd_record>> &storage_d,
                                 fs_view &pick_from);

    // Perform initial comparison between two fs_view objects.
    void initial_compare(size_t dir_id_l, size_t dir_id_r);

    // All files and subdirectories inside removed directories are considered also removed
    void resolve_removed_directories();

    // All files and subdirectories inside added directories are considered also added
    void resolve_added_directories();

    // Link between generated potential_* files and/or directories
    void link_between_potential_files();
public:
    fs_comparator(fs_view& first, fs_view& second) :
        first(first), second(second)
    { }

    void do_comparison();

    void changes_summary_console();
};


#endif //BOOST_CRC_TEST_FS_COMPARATOR_H
