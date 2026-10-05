#pragma once
#include <string>
#include <map>

void load_config();
void save_config();
void select_game_folder();
void refresh_mods(std::map<std::string, bool>& ui_mod_vars);
void toggle_mod(const std::string& mod_name, bool enable);
void install_mod_logic(const std::string& path, std::map<std::string, bool>& ui_mod_vars);
void install_mods_from_folders(std::map<std::string, bool>& ui_mod_vars);
void install_mods_from_archives(std::map<std::string, bool>& ui_mod_vars);
void delete_mod(const std::string& mod_name);
