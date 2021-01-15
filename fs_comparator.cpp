//
// Created by swql on 1/10/21.
//

#include "fs_comparator.h"

void fs_comparator::initial_compare(size_t l_dir_id, size_t r_dir_id) {
    auto file_equals = [](const fd_record &l, const fd_record &r){
        return (l.hash == r.hash);
    };

    // Compare two fd_records that are assumed to be directories for equality ONLY by hash and children count
    auto directory_equals = [](const fd_record &l, const fs_view &l_view,
                                const fd_record &r, const fs_view &r_view){
        return (l_view.get_children_count(l.dir_ID) == r_view.get_children_count(r.dir_ID) &&
                r.hash == l.hash);
    };

    // Initial compare
    auto &root1 = first.get_fd_by_id(l_dir_id);
    auto &root2 = second.get_fd_by_id(r_dir_id);

    df_container children_sep_first;
    df_container children_sep_second;
    {
        // Get root children
        auto root_children_first = first.get_fd_records_by_ids(first.get_directory_children(l_dir_id));
        auto root_children_second = second.get_fd_records_by_ids(second.get_directory_children(r_dir_id));

        if (directory_equals(root1, first, root2, second)) return;

        // Separate children by file and directory
        children_sep_first = perform_directory_vs_file_separation(root_children_first);
        children_sep_second = perform_directory_vs_file_separation(root_children_second);
    }

    df_container intersection = get_df_intersection(children_sep_first, children_sep_second);
    get_df_difference(children_sep_first, intersection, potentially_removed_directories, potentially_removed_files);
    get_df_difference(children_sep_second, intersection, potentially_added_directories, potentially_added_files);

    // Resolve intersection files/dirs
    {
        // Dirs
        std::filesystem::path path;
        for (auto &dir : intersection.first) {
            path = dir.get().path;
            fd_record &second_found = second.get_fd_by_path(path);
            if (!directory_equals(dir.get(), first, second_found, second)) {
                // Directories not equal hence we need to add it to list and recusrively search it
                this->changed_directories.push_back(dir);
                initial_compare(dir.get().dir_ID, second_found.dir_ID);
            }
        }

        // Files
        for (auto &f : intersection.second) {
            path = f.get().path;
            fd_record &second_found = second.get_fd_by_path(path);
            if (!directory_equals(f.get(), first, second_found, second)) {
                // File changed, just add to list
                this->changed_files.push_back(f);
            }
        }
    }
}

void fs_comparator::do_comparison() {
    initial_compare(0, 0);
    resolve_removed_directories();
    resolve_added_directories();
    link_between_potential_files();
}

df_container fs_comparator::perform_directory_vs_file_separation(std::vector<std::reference_wrapper<fd_record>> &vec) {
    static auto sort_fd_by_path = [](std::vector<std::reference_wrapper<fd_record>> &vec){
        std::sort(vec.begin(), vec.end(), [](const fd_record &l, const fd_record &r){
            return l.path < r.path;
        });
    };

    std::pair<std::vector<std::reference_wrapper<fd_record>>, std::vector<std::reference_wrapper<fd_record>>> ret;
    for (auto i = 0; i < vec.size(); i++){
        if (vec[i].get().is_dir()) ret.first.emplace_back(vec[i].get());
        else ret.second.emplace_back(vec[i].get());
    }

    sort_fd_by_path(ret.first);
    sort_fd_by_path(ret.second);
    return ret;
}

df_container fs_comparator::get_df_intersection(df_container &l, df_container &r) {
    df_container res;
    auto cmp = [](const fd_record &l, const fd_record &r){
        return l.path < r.path;
    };

    // Get intersection for directories
    std::set_intersection(l.first.begin(), l.first.end(),
                          r.first.begin(), r.first.end(),
                          std::back_inserter(res.first), cmp);

    // Get intersection for files
    std::set_intersection(l.second.begin(), l.second.end(),
                          r.second.begin(), r.second.end(),
                          std::back_inserter(res.second), cmp);

    return res;
}

void fs_comparator::get_df_difference(df_container &B, df_container &A,
                                      std::list<std::reference_wrapper<fd_record>> &files,
                                      std::list<std::reference_wrapper<fd_record>> &directories
                                      ) {

    // Get files
    std::set_difference(B.first.begin(), B.first.end(),
                        A.first.begin(), A.first.end(),
                        std::back_inserter(files),
                        [](const fd_record &l, const fd_record &r){
                            return l.path < r.path;
                        });

    // Get directories
    std::set_difference(B.second.begin(), B.second.end(),
                        A.second.begin(), A.second.end(),
                        std::back_inserter(directories),
                        [](const fd_record &l, const fd_record &r){
                            return l.path < r.path;
                        });
}

void fs_comparator::link_between_potential_files() {
    typedef std::list<std::reference_wrapper<fd_record>>::const_iterator iterator_t;

    auto find_fd_by_hash =
            [](     const std::list<std::reference_wrapper<fd_record>> &val,
                    const int32_t hash,
                    iterator_t &result){
        result = std::find_if(val.begin(), val.end(), [&](const fd_record &rec){
            return rec.hash == hash;
        });
        if (result == val.end()) return false;
        return true;
    };

    // Compare fd by path
    enum fstatus { SAME_DIRECTORY_DIFFERENT_NAME, DIFFERENT_DIRECTORY_DIFFERENT_NAME, DIFFERENT_DIRECTORY_SAME_NAME };
    auto fd_path_cmp = [](const fd_record &l, const fd_record &r) -> fstatus{
        std::filesystem::path p1 {l.path};
        std::filesystem::path p2 {r.path};
        if (p1.parent_path() == p2.parent_path()) {
            return SAME_DIRECTORY_DIFFERENT_NAME;
        }
        else {
            if (p1.filename() == p2.filename()) {
                return DIFFERENT_DIRECTORY_SAME_NAME;
            }
            else {
                return DIFFERENT_DIRECTORY_DIFFERENT_NAME;
        }
}
    };

    auto cmp_main = [&find_fd_by_hash, &fd_path_cmp](std::list<std::reference_wrapper<fd_record>> &potentially_removed,
                       std::list<std::reference_wrapper<fd_record>> &potentially_added,
                        std::list<moved_record> &moved,
                        std::list<moved_record> &moved_and_renamed,
                        std::list<moved_record> &renamed){
        std::list<std::reference_wrapper<fd_record>>::const_iterator add_it;
        for (iterator_t rem_it = potentially_removed.begin(); rem_it != potentially_removed.end();){
            if (find_fd_by_hash(potentially_added, rem_it->get().hash, add_it)){
                switch (fd_path_cmp(*rem_it, *add_it)){
                    case SAME_DIRECTORY_DIFFERENT_NAME:
                        renamed.emplace_back(rem_it->get().path, add_it->get().path);
                        break;
                    case DIFFERENT_DIRECTORY_SAME_NAME:
                        moved.emplace_back(rem_it->get().path, add_it->get().path);
                        break;
                    case DIFFERENT_DIRECTORY_DIFFERENT_NAME:
                        moved_and_renamed.emplace_back(rem_it->get().path, add_it->get().path);
                        break;
                }

                rem_it = potentially_removed.erase(rem_it);
                potentially_added.erase(add_it);
            }
            else {
                ++rem_it;
            }
        }
    };

    // files
    cmp_main(potentially_removed_files, potentially_added_files,
             moved_files, moved_and_renamed_files, renamed_files);

    // dirs
    cmp_main(potentially_removed_directories, potentially_added_directories,
             moved_disrectories, moved_and_renamed_directories, renamed_disrectories);

}

std::string fs_comparator::changes_summary() {
    std::string result;
    size_t f_count = first.get_file_count();
    size_t f_count_m = second.get_file_count();
    size_t dir_count = first.get_fd_count() - f_count;
    size_t dir_count_m = second.get_fd_count() - f_count_m;

    int64_t f_diff = f_count_m - f_count;
    int64_t dir_diff = dir_count_m - dir_count;

    str_compose(result, "================dircmp comparison summary=================",
                '\n', "New files: ", std::to_string(potentially_added_files.size()),
                '\n', "New directories: ", std::to_string(potentially_added_directories.size()),
                '\n', "Removed files: ", std::to_string(potentially_removed_files.size()),
                '\n', "Removed directories: ", std::to_string(potentially_removed_directories.size()),
                '\n', "Files before: ", std::to_string(f_count),
                '\n', "Files after: " , std::to_string(f_count_m), '(', std::to_string(f_diff), ')',
                '\n', "Directories before: ", std::to_string(dir_count),
                '\n', "Directories after: ", std::to_string(dir_count_m), '(', std::to_string(dir_diff), ')',
                '\n');

    auto get_finfo = [](std::string &str, const fd_record &rec){
        str_compose(str, "     ", rec.path, '\n');
    };

    auto mrec_out = [](std::string &str, const moved_record &rec) {
        str_compose(str, "     ", rec.path_old.string(), " -> ",rec.path_new.string(), '\n');
    };

    str_compose(result, '\n', "===========================================================");

    str_compose(result, '\n', " Files: ",
                '\n', "  ", "Removed:", '\n');
    for (auto &el : potentially_removed_files){
        get_finfo(result, el);
    }

    str_compose(result, "  ", "Added:", '\n');
    for (auto &el : potentially_added_files){
        get_finfo(result, el);
    }

    str_compose(result, "  ", "Renamed:", '\n');
    for (auto &el : renamed_files){
        mrec_out(result, el);
    }

    str_compose(result, "  ", "Changed:", '\n');
    for (auto &el : changed_files){
        get_finfo(result, el);
    }

    str_compose(result, "  ", "Moved:", '\n');
    for (auto &el : moved_files){
        mrec_out(result, el);
    }

    str_compose(result, "  ", "Moved and renamed:", '\n');
    for (auto &el : moved_and_renamed_files){
        mrec_out(result, el);
    }

    str_compose(result, '\n', " Directories: ",
                '\n', "  ", "Removed:", '\n');
    for (auto &el : potentially_removed_directories){
        get_finfo(result, el);
    }

    str_compose(result, "  ", "Added:", '\n');
    for (auto &el : potentially_added_directories){
        get_finfo(result, el);
    }

    str_compose(result, "  ", "Renamed:", '\n');
    for (auto &el : renamed_disrectories){
        mrec_out(result, el);
    }

    str_compose(result, "  ", "Changed:", '\n');
    for (auto &el : changed_directories){
        get_finfo(result, el);
    }

    str_compose(result, "  ", "Moved:", '\n');
    for (auto &el : moved_disrectories){
        mrec_out(result, el);
    }

    str_compose(result, "  ", "Moved and renamed:", '\n');
    for (auto &el : moved_and_renamed_directories){
        mrec_out(result, el);
    }

    return result;
}

void fs_comparator::resolve_removed_directories() {
    decltype(potentially_removed_directories) additional_dirs;
    for (auto &el : potentially_removed_directories){
        directory_recursive_ops(el, potentially_removed_files, additional_dirs, first);
    }
    std::copy(additional_dirs.begin(), additional_dirs.end(), std::back_inserter(potentially_removed_directories));
}

void fs_comparator::resolve_added_directories() {
    for (auto &el : potentially_added_directories){
        directory_recursive_ops(el, potentially_added_files, potentially_added_directories, second);
    }
}

void fs_comparator::directory_recursive_ops(const fd_record &dir,
                                            std::list<std::reference_wrapper<fd_record>> &storage_f,
                                            std::list<std::reference_wrapper<fd_record>> &storage_d,
                                            fs_view &pick_from) {
    auto ch = pick_from.get_fd_records_by_ids(pick_from.get_directory_children(dir.dir_ID));
    for (const auto &el : ch) {
        if (el.get().is_dir()) {
            storage_d.push_back(el);
            directory_recursive_ops(el, storage_f, storage_d, pick_from);
        } else {
            storage_f.push_back(el);
        }
    }
}