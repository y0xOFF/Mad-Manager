#include "Globals.h"

std::string g_game_path = "";
const std::string MODS_DIR = "Mods";
const std::string CONFIG_FILE = "config.ini";

std::string g_mod_to_delete = "";
bool g_show_delete_confirm = false;
bool g_show_delete_all_confirm = false;
std::vector<std::string> g_dropped_files;
std::set<std::string> g_selected_mods;
std::string g_last_clicked_mod = "";

AppPhase g_phase = PHASE_SPLASH_IN;
bool g_AppClosing = false;
