#define LOGGER_TO_COUT

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <filesystem>
#include "boost/crc.hpp"
#include "fs_view.h"
#include "fs_comparator.h"
#include "pbar.h"
#include "loggerpp/logger.h"
#include "file_h.h"

int main(int argc, char** argv) {
    std::cout << "Note: running dircmp as root is recommended" << std::endl;

    if (argc < 2){
        std::cout << "dircmp [command]" << std::endl;
        std::cout << "dircmp create [folder] [view-name] - create filesystem view from provided folder" << std::endl;
        std::cout << "dircmp cmp [path-to-original-view] [path-to-new-view] - compare 2 views" << std::endl;
    }
    else if (strcmp(argv[1], "create") == 0){
        if (argc < 4){
            std::cout << "dircmp create [folder] [name] - create filesystem view from provided folder using" <<
            " custom name"
            << std::endl;
            return 0;
        }

        std::filesystem::path log_path {"dircmp-log"};
        std::string fname { "last-create.txt"};

        logger logger {log_path, fname };

        std::cout << "Path: " << argv[2] << std::endl;
        std::cout << "FS view name: " << argv[3] << std::endl;

        auto time_now = std::chrono::high_resolution_clock::now();
        std::cout << "Forming directory structure..." << std::endl;
        fs_view fv;
        fv.form(argv[2], logger);

        typedef std::chrono::seconds sec;
        auto time_then = std::chrono::high_resolution_clock::now();
        std::cout << "Directory structure formed in " << std::chrono::duration_cast<sec>((time_then - time_now)).count()
        << " sec" << std::endl;
        std::cout << "A total of " << fv.get_fd_count() << " files and directories" << std::endl;

        time_now = std::chrono::high_resolution_clock::now();
        std::cout << "Writing view to the disk..." << std::endl;

        fv.loadf_out(argv[3], logger);
        time_then = std::chrono::high_resolution_clock::now();
        std::cout << "View written to disk in " << std::chrono::duration_cast<sec>((time_then - time_now)).count()
                  << " sec" << std::endl;

        std::cout << "Log written to " << log_path.string() << '/' << fname << std::endl;
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

        std::cout << "Loading done" << std::endl;
        fs_comparator cmp {original, modified};

        cmp.do_comparison();

        auto cs = cmp.changes_summary();
        std::cout << cs << std::endl;
        std::cout << "Comparison done" << std::endl;

        std::string cmp_log_path { "dircmp-log/last-comparison.txt" };
        auto f = new_file_t(cmp_log_path);
        write_nt(f, cs.data(), cs.size());
    }
    else{
        std::cout << "Wrong arguments" << std::endl;
    }
    return 0;
}