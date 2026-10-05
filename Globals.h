#pragma once
#include <string>
#include <vector>
#include <set>

extern std::string g_game_path;
extern const std::string MODS_DIR;
extern const std::string CONFIG_FILE;

extern std::string g_mod_to_delete;
extern bool g_show_delete_confirm;
extern bool g_show_delete_all_confirm;
extern std::vector<std::string> g_dropped_files;
extern std::set<std::string> g_selected_mods;
extern std::string g_last_clicked_mod;

enum AppPhase {
    PHASE_SPLASH_IN,
    PHASE_SPLASH_WAIT,
    PHASE_SPLASH_OUT,
    PHASE_UI_IN,
    PHASE_UI_ACTIVE,
    PHASE_UI_OUT
};
extern AppPhase g_phase;
extern bool g_AppClosing;
