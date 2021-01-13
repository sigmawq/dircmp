#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <filesystem>
#include "boost/crc.hpp"
#include "fs_view.h"
#include "fs_comparator.h"


int main(int argc, char** argv) {

    if (argc < 2){
        std::cout << "dircmp [command]" << std::endl;
        std::cout << "dircmp create [folder] - create filesystem view from provided folder" << std::endl;
        std::cout << "dircmp cmp [path-to-original-view] [path-to-new-view] - compare 2 views" << std::endl;
    }
    else if (strcmp(argv[1], "create") == 0){
        if (argc < 4){
            std::cout << "dircmp create [folder] [name] - create filesystem view from provided folder using" <<
            " custom name"
            << std::endl;
            return 0;
        }
        std::cout << "Path: " << argv[2] << std::endl;
        std::cout << "FS view name: " << argv[3] << std::endl;

        auto time_now = std::chrono::high_resolution_clock::now();
        std::cout << "Forming directory structure..." << std::endl;
        fs_view fv;
        fv.form(argv[2]);

        typedef std::chrono::seconds sec;
        auto time_then = std::chrono::high_resolution_clock::now();
        std::cout << "Directory structure formed in " << std::chrono::duration_cast<sec>((time_then - time_now)).count()
        << " sec" << std::endl;
        std::cout << "A total of " << fv.get_fd_count() << " files and directories" << " and " << fv.get_dirh_rel_count()
        << " directory relations" << std::endl;

        time_now = std::chrono::high_resolution_clock::now();
        std::cout << "Writing view to the disk..." << std::endl;

        fv.loadf_out(argv[3]);
        time_then = std::chrono::high_resolution_clock::now();
        std::cout << "View written to disk in " << std::chrono::duration_cast<sec>((time_then - time_now)).count()
                  << " sec" << std::endl;

        std::cout << "Done." << std::endl;
    }
    else if (strcmp(argv[1], "cmp") == 0){
        if (argc < 4){
            std::cout << "dircmp cmp [path-to-original-view] [path-to-new-view]" << std::endl;
            return 0;
        }
        std::cout << "Compare views" << std::endl;
        std::cout << "Original view: " << argv[2] << std::endl;
        std::cout << "Modified view: " << argv[3] << std::endl;

        std::cout << "Loading original view from disk..." << std::endl;
        fs_view original;
        original.loadf(argv[2]);

        std::cout << "Loading modified view from disk..." << std::endl;
        fs_view modified;
        modified.loadf(argv[3]);

        std::cout << "Load done." << std::endl;
        fs_comparator cmp {original, modified};
        cmp.do_comparison();

        cmp.changes_summary_console();
        std::cout << "Comparing done" << std::endl;
    }
    else{
        std::cout << "Wrong arguments" << std::endl;
    }
    return 0;
}