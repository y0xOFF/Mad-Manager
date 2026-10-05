#include "ModLogic.h"
#include "Globals.h"
#include "FileBrowser.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>

namespace fs = std::filesystem;

void load_config() {
    if (!fs::exists(CONFIG_FILE)) {
        std::ofstream f(CONFIG_FILE);
        f << "[SETTINGS]\n";
        f << "MADMAX_DIRECTORY=\n";
        f << "MODS_DIRECTORY=" << (fs::current_path() / MODS_DIR).string() << "\n";
        f << "MADMANAGER_DIRECTORY=" << fs::current_path().string() << "\n";
        return;
    }
    std::ifstream f(CONFIG_FILE);
    std::string line;
    while (std::getline(f, line)) {
        if (line.find("MADMAX_DIRECTORY=") == 0) {
            g_game_path = line.substr(17);
        }
    }
}

void save_config() {
    std::ofstream f(CONFIG_FILE);
    f << "[SETTINGS]\n";
    f << "MADMAX_DIRECTORY=" << g_game_path << "\n";
    f << "MODS_DIRECTORY=" << (fs::current_path() / MODS_DIR).string() << "\n";
    f << "MADMANAGER_DIRECTORY=" << fs::current_path().string() << "\n";
}

void install_mod_if_new(const std::string& mod_name) {
    fs::path source_dropzone = fs::path(MODS_DIR) / mod_name / "dropzone";
    fs::path manifest_path = fs::path(MODS_DIR) / mod_name / "manifest.ini";
    if (fs::exists(manifest_path)) return;

    std::vector<std::string> files;
    if (fs::exists(source_dropzone) && fs::is_directory(source_dropzone)) {
        for (const auto& entry : fs::recursive_directory_iterator(source_dropzone)) {
            if (entry.is_regular_file()) {
                fs::path rel_path = fs::relative(entry.path(), source_dropzone);
                files.push_back(rel_path.string());
                if (!g_game_path.empty()) {
                    fs::path dest_file = fs::path(g_game_path) / "dropzone" / rel_path;
                    fs::create_directories(dest_file.parent_path());
                    std::error_code ec;
                    fs::copy_file(entry.path(), dest_file, fs::copy_options::overwrite_existing, ec);
                }
            }
        }
    }
    std::ofstream f(manifest_path);
    f << "[FILES]\n";
    for (const auto& file : files) {
        f << "FILE=" << file << "\n";
    }
}

void select_game_folder() {
    std::string path = select_directory();
    if (!path.empty()) {
        g_game_path = path;
        save_config();

        if (!g_game_path.empty()) {
            fs::path game_dropzone = fs::path(g_game_path) / "dropzone";
            fs::create_directories(game_dropzone);
            for (const auto& entry : fs::directory_iterator(MODS_DIR)) {
                if (entry.is_directory()) {
                    fs::path source_dropzone = entry.path() / "dropzone";
                    fs::path manifest_path = entry.path() / "manifest.ini";
                    if (fs::exists(manifest_path)) {
                        std::ifstream in(manifest_path);
                        std::string line;
                        while (std::getline(in, line)) {
                            if (line.find("FILE=") == 0) {
                                std::string file = line.substr(5);
                                fs::path src = source_dropzone / file;
                                fs::path dst = game_dropzone / file;
                                if (fs::exists(src)) {
                                    fs::create_directories(dst.parent_path());
                                    std::error_code ec;
                                    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void refresh_mods(std::map<std::string, bool>& ui_mod_vars) {
    ui_mod_vars.clear();
    if (!fs::exists(MODS_DIR)) fs::create_directory(MODS_DIR);

    for (const auto& entry : fs::directory_iterator(MODS_DIR)) {
        if (entry.is_directory()) {
            fs::path dropzone_path = entry.path() / "dropzone";
            if (fs::exists(dropzone_path) && fs::is_directory(dropzone_path)) {
                std::string mod_name = entry.path().filename().string();
                install_mod_if_new(mod_name);
                
                fs::path manifest_path = entry.path() / "manifest.ini";
                bool is_enabled = true;
                if (fs::exists(manifest_path)) {
                    std::ifstream in(manifest_path);
                    std::string line;
                    while (std::getline(in, line)) {
                        if (line.find("FILE=") == 0) {
                            if (line.size() >= 9 && line.substr(line.size() - 9) == ".disabled") {
                                is_enabled = false;
                            }
                            break;
                        }
                    }
                }
                ui_mod_vars[mod_name] = is_enabled;
            }
        }
    }
}

void toggle_mod(const std::string& mod_name, bool enable) {
    fs::path manifest_path = fs::path(MODS_DIR) / mod_name / "manifest.ini";
    if (!fs::exists(manifest_path)) return;

    std::vector<std::string> files;
    std::ifstream in(manifest_path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.find("FILE=") == 0) {
            files.push_back(line.substr(5));
        }
    }
    in.close();

    fs::path source_dropzone = fs::path(MODS_DIR) / mod_name / "dropzone";
    fs::path game_dropzone = g_game_path.empty() ? fs::path("") : (fs::path(g_game_path) / "dropzone");

    std::vector<std::string> new_files;
    for (const auto& file : files) {
        std::string new_file = file;
        if (enable) {
            if (new_file.size() >= 9 && new_file.substr(new_file.size() - 9) == ".disabled") {
                new_file = new_file.substr(0, new_file.size() - 9);
            }
        } else {
            if (new_file.size() < 9 || new_file.substr(new_file.size() - 9) != ".disabled") {
                new_file += ".disabled";
            }
        }

        if (file != new_file) {
            std::error_code ec;
            fs::rename(source_dropzone / file, source_dropzone / new_file, ec);
            if (!g_game_path.empty()) {
                fs::rename(game_dropzone / file, game_dropzone / new_file, ec);
            }
        }
        new_files.push_back(new_file);
    }

    std::ofstream out(manifest_path);
    out << "[FILES]\n";
    for (const auto& nf : new_files) {
        out << "FILE=" << nf << "\n";
    }
}

void install_mod_logic(const std::string& path, std::map<std::string, bool>& ui_mod_vars) {
    if (path.empty()) return;
    std::string mod_name;
    if (fs::is_directory(path)) {
        fs::path src_dir(path);
        mod_name = src_dir.filename().string();
        fs::path dest_dir = fs::path(MODS_DIR) / mod_name;
        std::error_code ec;
        fs::copy(src_dir, dest_dir, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    } else {
        std::string ext = fs::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".zip" || ext == ".rar" || ext == ".7z") {
            fs::path src_file(path);
            mod_name = src_file.stem().string();
            fs::path dest_dir = fs::path(MODS_DIR) / mod_name;
            fs::create_directories(dest_dir);
            std::string cmd = "tar -xf \"" + path + "\" -C \"" + dest_dir.string() + "\"";
            system(cmd.c_str());
        } else {
            return;
        }
    }
    refresh_mods(ui_mod_vars);
    toggle_mod(mod_name, true);
    refresh_mods(ui_mod_vars);
}

void install_mods_from_folders(std::map<std::string, bool>& ui_mod_vars) {
    for (const std::string& path : select_directories()) {
        install_mod_logic(path, ui_mod_vars);
    }
}

void install_mods_from_archives(std::map<std::string, bool>& ui_mod_vars) {
    for (const std::string& path : select_archives()) {
        install_mod_logic(path, ui_mod_vars);
    }
}

void delete_mod(const std::string& mod_name) {
    fs::path manifest_path = fs::path(MODS_DIR) / mod_name / "manifest.ini";
    if (fs::exists(manifest_path)) {
        std::ifstream in(manifest_path);
        std::string line;
        std::vector<std::string> files;
        while (std::getline(in, line)) {
            if (line.find("FILE=") == 0) {
                files.push_back(line.substr(5));
            }
        }
        in.close();

        if (!g_game_path.empty()) {
            fs::path game_dropzone = fs::path(g_game_path) / "dropzone";
            for (const auto& file : files) {
                std::error_code ec;
                fs::remove(game_dropzone / file, ec);
            }
            for (const auto& file : files) {
                fs::path dir_path = (game_dropzone / file).parent_path();
                while (fs::exists(dir_path)) {
                    std::error_code ec;
                    if (fs::equivalent(dir_path, game_dropzone, ec)) break;
                    if (fs::is_directory(dir_path, ec) && fs::is_empty(dir_path, ec)) {
                        fs::remove(dir_path, ec);
                        dir_path = dir_path.parent_path();
                    } else {
                        break;
                    }
                }
            }
        }
    }
    std::error_code ec;
    fs::remove_all(fs::path(MODS_DIR) / mod_name, ec);
}
