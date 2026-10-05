#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <shobjidl.h> 
#include <shellapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

#include "Globals.h"
#include "ModLogic.h"
#include "ImageLoader.h"

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);




int main(int, char**) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    ImGui_ImplWin32_EnableDpiAwareness();
    
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"MadManager", nullptr };
    ::RegisterClassExW(&wc);
    
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 500;
    int winH = 550;
    int winX = (screenW - winW) / 2;
    int winY = (screenH - winH) / 2;

    HWND hwnd = ::CreateWindowExW(WS_EX_LAYERED, wc.lpszClassName, L"Mad Manager", WS_POPUP, winX, winY, winW, winH, nullptr, nullptr, wc.hInstance, nullptr);
    DragAcceptFiles(hwnd, TRUE);
    
    // Set initial opacity to 0
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Apply exact ASI Inventory Editor theme
    style.FramePadding = ImVec2(4.0f, 3.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    style.FrameBorderSize = 1.0f;
    colors[ImGuiCol_Border] = ImVec4(0.8f, 0.8f, 0.8f, 0.5f);
    colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(1.0f, 1.0f, 1.0f, 0.15f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    colors[ImGuiCol_Tab] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    colors[ImGuiCol_TabActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors[ImGuiCol_CheckboxSelectedBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    load_config();
    std::map<std::string, bool> ui_mod_vars;
    refresh_mods(ui_mod_vars);

    if (!fs::exists(MODS_DIR)) fs::create_directory(MODS_DIR);
    HANDLE hDir = FindFirstChangeNotificationA(MODS_DIR.c_str(), FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);

    ID3D11ShaderResourceView* splash_tex = nullptr;
    int splash_w = 0, splash_h = 0;
    
    HRSRC hResource = FindResourceA(GetModuleHandle(NULL), MAKEINTRESOURCEA(100), RT_RCDATA);
    if (hResource) {
        HGLOBAL hMemory = LoadResource(GetModuleHandle(NULL), hResource);
        if (hMemory) {
            DWORD dwSize = SizeofResource(GetModuleHandle(NULL), hResource);
            LPVOID lpAddress = LockResource(hMemory);
            if (lpAddress) {
                if (!LoadTextureFromMemory((const unsigned char*)lpAddress, dwSize, g_pd3dDevice, &splash_tex, &splash_w, &splash_h)) {
                    g_phase = PHASE_UI_IN;
                }
            } else g_phase = PHASE_UI_IN;
        } else g_phase = PHASE_UI_IN;
    } else g_phase = PHASE_UI_IN;

    bool done = false;
    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        if (!g_dropped_files.empty()) {
            for (const auto& path : g_dropped_files) {
                install_mod_logic(path, ui_mod_vars);
            }
            g_dropped_files.clear();
        }

        if (hDir != INVALID_HANDLE_VALUE) {
            if (WaitForSingleObject(hDir, 0) == WAIT_OBJECT_0) {
                refresh_mods(ui_mod_vars);
                FindNextChangeNotification(hDir);
            }
        }

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        static DWORD last_time = GetTickCount();
        DWORD current_time = GetTickCount();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        static float win_alpha = 0.0f;
        static DWORD splash_start = 0;
        static float fade_speed = 1.0f; // 1.0s fade

        if (g_AppClosing && g_phase != PHASE_UI_OUT) {
            g_phase = PHASE_UI_OUT;
        }

        DWORD layer_flags = LWA_ALPHA;
        COLORREF color_key = RGB(0, 0, 0);
        if (g_phase == PHASE_SPLASH_IN || g_phase == PHASE_SPLASH_WAIT || g_phase == PHASE_SPLASH_OUT) {
            layer_flags |= LWA_COLORKEY;
        }

        if (g_phase == PHASE_SPLASH_IN) {
            win_alpha += fade_speed * delta_time;
            if (win_alpha >= 1.0f) {
                win_alpha = 1.0f;
                g_phase = PHASE_SPLASH_WAIT;
                splash_start = current_time;
            }
            SetLayeredWindowAttributes(hwnd, color_key, (BYTE)(win_alpha * 255.0f), layer_flags);
        } else if (g_phase == PHASE_SPLASH_WAIT) {
            if (current_time - splash_start > 1500) {
                g_phase = PHASE_SPLASH_OUT;
            }
            SetLayeredWindowAttributes(hwnd, color_key, (BYTE)(win_alpha * 255.0f), layer_flags);
        } else if (g_phase == PHASE_SPLASH_OUT) {
            win_alpha -= fade_speed * delta_time;
            if (win_alpha <= 0.0f) {
                win_alpha = 0.0f;
                g_phase = PHASE_UI_IN;
                if (splash_tex) {
                    splash_tex->Release();
                    splash_tex = nullptr;
                }
            }
            SetLayeredWindowAttributes(hwnd, color_key, (BYTE)(win_alpha * 255.0f), layer_flags);
        } else if (g_phase == PHASE_UI_IN) {
            win_alpha += fade_speed * delta_time;
            if (win_alpha >= 1.0f) {
                win_alpha = 1.0f;
                g_phase = PHASE_UI_ACTIVE;
            }
            SetLayeredWindowAttributes(hwnd, color_key, (BYTE)(win_alpha * 255.0f), layer_flags);
        } else if (g_phase == PHASE_UI_OUT) {
            win_alpha -= fade_speed * 1.5f * delta_time;
            if (win_alpha <= 0.0f) {
                win_alpha = 0.0f;
                done = true;
            }
            SetLayeredWindowAttributes(hwnd, color_key, (BYTE)(win_alpha * 255.0f), layer_flags);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_phase == PHASE_SPLASH_IN || g_phase == PHASE_SPLASH_WAIT || g_phase == PHASE_SPLASH_OUT) {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(500, 550));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0)); // fully transparent ImGui window
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::Begin("Splash", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
            if (splash_tex) {
                float max_w = 490.0f; // a little bit smaller than the UI width (500)
                float max_h = 540.0f; // a little bit smaller than the UI height (550)
                float scale = 1.0f;
                if (splash_w > max_w || splash_h > max_h) {
                    float scale_w = max_w / splash_w;
                    float scale_h = max_h / splash_h;
                    scale = scale_w < scale_h ? scale_w : scale_h;
                }
                float draw_w = splash_w * scale;
                float draw_h = splash_h * scale;

                ImVec2 center = ImVec2(250, 275);
                ImGui::SetCursorPos(ImVec2(center.x - draw_w/2.0f, center.y - draw_h/2.0f));
                ImGui::Image((void*)splash_tex, ImVec2(draw_w, draw_h));
            }
            ImGui::End();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        } else {


        // UI Definition
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x, main_viewport->WorkSize.y));
        
        ImGui::Begin("Mad Manager", nullptr, window_flags);
        
        ImGui::Text("Mad Manager");
        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        if (ImGui::Button("X", ImVec2(20, 20))) {
            g_AppClosing = true;
        }
        ImGui::PopStyleVar();

        ImGui::Spacing();


        bool ctrl_down = ImGui::GetIO().KeyCtrl;
        bool shift_down = ImGui::GetIO().KeyShift;

        if (ctrl_down && ImGui::IsKeyPressed(ImGuiKey_A, false)) {
            if (!ui_mod_vars.empty() && g_selected_mods.size() == ui_mod_vars.size()) {
                g_selected_mods.clear();
            } else {
                g_selected_mods.clear();
                for (const auto& pair : ui_mod_vars) {
                    g_selected_mods.insert(pair.first);
                }
            }
        }
        if (ctrl_down && ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
            bool any_disabled = false;
            for (const auto& pair : ui_mod_vars) {
                if (!pair.second) { any_disabled = true; break; }
            }
            bool new_state = any_disabled;
            for (auto& pair : ui_mod_vars) {
                if (pair.second != new_state) {
                    pair.second = new_state;
                    toggle_mod(pair.first, new_state);
                }
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && !g_selected_mods.empty()) {
            g_show_delete_all_confirm = true;
        }

        if (g_show_delete_confirm) {
            ImGui::OpenPopup("Delete Mod?");
        }
        if (g_show_delete_all_confirm) {
            ImGui::OpenPopup("Delete Selected Mods?");
        }
        
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(vp->GetCenter().x, vp->GetCenter().y), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Delete Mod?", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
            ImGui::Text("Delete %s?", g_mod_to_delete.c_str());
            ImGui::Separator();
            if (ImGui::Button("Yes", ImVec2(120, 0))) {
                delete_mod(g_mod_to_delete);
                g_show_delete_confirm = false;
                ImGui::CloseCurrentPopup();
                refresh_mods(ui_mod_vars);
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(120, 0))) {
                g_show_delete_confirm = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::SetNextWindowPos(ImVec2(vp->GetCenter().x, vp->GetCenter().y), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Delete Selected Mods?", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
            if (g_selected_mods.size() == ui_mod_vars.size()) {
                ImGui::Text("Delete All Mods?");
            } else if (g_selected_mods.size() == 1) {
                ImGui::Text("Delete %s?", g_selected_mods.begin()->c_str());
            } else {
                ImGui::Text("Delete %zu Mods?", g_selected_mods.size());
            }
            ImGui::Separator();
            if (ImGui::Button("Yes", ImVec2(120, 0))) {
                for (const auto& mod : g_selected_mods) {
                    delete_mod(mod);
                }
                g_selected_mods.clear();
                g_show_delete_all_confirm = false;
                ImGui::CloseCurrentPopup();
                refresh_mods(ui_mod_vars);
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(120, 0))) {
                g_show_delete_all_confirm = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem("Main Menu")) {


                float bottom_space = g_game_path.empty() ? -98.0f : -60.0f;
                ImGui::BeginChild("ModsList", ImVec2(0, bottom_space), true);
                
                std::vector<std::string> mod_names;
                for (const auto& pair : ui_mod_vars) mod_names.push_back(pair.first);

                for (size_t i = 0; i < mod_names.size(); i++) {
                    const std::string& mname = mod_names[i];
                    bool checked = ui_mod_vars[mname];
                    bool is_selected = g_selected_mods.find(mname) != g_selected_mods.end();

                    ImGui::PushID(mname.c_str());
                    ImVec2 pos = ImGui::GetCursorPos();

                    float row_height = ImGui::GetFrameHeight();
                    bool clicked_left = ImGui::Selectable("##sel", is_selected, ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, row_height));
                    bool hovered = ImGui::IsItemHovered();
                    bool clicked_right = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right);

                    ImGui::SetCursorPos(pos);
                    bool checkbox_clicked = ImGui::Checkbox(mname.c_str(), &checked);

                    // Prevent Checkbox clicks from also triggering the row selection logic and messing up selections
                    if (checkbox_clicked) {
                        clicked_left = false;
                    }

                    if (clicked_left || clicked_right) {
                        if (clicked_left) {
                            if (ctrl_down) {
                                // Windows Blueprint: Ctrl+Click toggles individual item selection without clearing
                                if (is_selected) g_selected_mods.erase(mname);
                                else g_selected_mods.insert(mname);
                                g_last_clicked_mod = mname;
                            } else if (shift_down) {
                                // Windows Blueprint: Shift+Click clears current selection and selects the range from Anchor
                                if (!g_last_clicked_mod.empty()) {
                                    int start_idx = -1, end_idx = -1;
                                    for (int j = 0; j < (int)mod_names.size(); j++) {
                                        if (mod_names[j] == g_last_clicked_mod) start_idx = j;
                                        if (mod_names[j] == mname) end_idx = j;
                                    }
                                    if (start_idx != -1 && end_idx != -1) {
                                        if (start_idx > end_idx) std::swap(start_idx, end_idx);
                                        g_selected_mods.clear();
                                        for (int j = start_idx; j <= end_idx; j++) {
                                            g_selected_mods.insert(mod_names[j]);
                                        }
                                    }
                                }
                            } else {
                                // Windows Blueprint: Normal Left Click clears selection and selects just this item
                                g_selected_mods.clear();
                                g_selected_mods.insert(mname);
                                g_last_clicked_mod = mname;
                            }
                        } else if (clicked_right) {
                            // Windows Blueprint: Right click selects item only if it wasn't already selected
                            if (!is_selected) {
                                g_selected_mods.clear();
                                g_selected_mods.insert(mname);
                                g_last_clicked_mod = mname;
                            }
                        }
                    }

                    if (clicked_right) {
                        ImGui::OpenPopup("Context");
                    }

                    if (checkbox_clicked) {
                        ui_mod_vars[mname] = checked;
                        toggle_mod(mname, checked);

                        // If Shift was held while checking the box, bulk check/uncheck the range
                        if (shift_down && !g_last_clicked_mod.empty()) {
                            int start_idx = -1, end_idx = -1;
                            for (int j = 0; j < (int)mod_names.size(); j++) {
                                if (mod_names[j] == g_last_clicked_mod) start_idx = j;
                                if (mod_names[j] == mname) end_idx = j;
                            }
                            if (start_idx != -1 && end_idx != -1) {
                                if (start_idx > end_idx) std::swap(start_idx, end_idx);
                                for (int j = start_idx; j <= end_idx; j++) {
                                    if (ui_mod_vars[mod_names[j]] != checked) {
                                        ui_mod_vars[mod_names[j]] = checked;
                                        toggle_mod(mod_names[j], checked);
                                    }
                                }
                            }
                        }
                        g_last_clicked_mod = mname;
                    }

                    if (ImGui::BeginPopup("Context")) {
                        if (!g_selected_mods.empty() && g_selected_mods.find(mname) != g_selected_mods.end()) {
                            std::string del_text = "Delete";
                            if (g_selected_mods.size() == ui_mod_vars.size()) {
                                del_text = "Delete All Mods";
                            } else if (g_selected_mods.size() == 1) {
                                del_text = "Delete " + *g_selected_mods.begin();
                            } else {
                                del_text = "Delete " + std::to_string(g_selected_mods.size()) + " Mods";
                            }
                            if (ImGui::Selectable(del_text.c_str())) {
                                g_show_delete_all_confirm = true;
                            }
                        } else {
                            if (ImGui::Selectable("Delete")) {
                                g_mod_to_delete = mname;
                                g_show_delete_confirm = true;
                            }
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();
                }

                if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    g_selected_mods.clear();
                    g_last_clicked_mod = "";
                }

                ImGui::EndChild();
                
                float total_btn_h = g_game_path.empty() ? (30.0f * 2.0f + 8.0f) : 30.0f;
                float avail_y = ImGui::GetContentRegionAvail().y;
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail_y - total_btn_h) * 0.5f);

                float btn_width = 200.0f;
                if (g_game_path.empty()) {
                    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - btn_width) * 0.5f);
                    if (ImGui::Button("Select Mad Max Directory", ImVec2(btn_width, 30.0f))) {
                        select_game_folder();
                    }
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
                }
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - btn_width) * 0.5f);
                
                ImVec2 btn_pos = ImGui::GetCursorScreenPos();
                ImVec2 btn_size = ImVec2(btn_width, 30.0f);
                
                if (ImGui::Button("Install Mods", btn_size)) {
                    ImGui::OpenPopup("InstallModsOptions");
                }
                


                ImGui::SetNextWindowPos(ImVec2(btn_pos.x + btn_size.x * 0.5f, btn_pos.y - 9.0f), ImGuiCond_Appearing, ImVec2(0.5f, 1.0f));
                if (ImGui::BeginPopup("InstallModsOptions", ImGuiWindowFlags_NoMove)) {
                    if (ImGui::Selectable("Install from Archives (.zip, .rar, .7z)")) {
                        install_mods_from_archives(ui_mod_vars);
                    }
                    ImGui::Separator();
                    if (ImGui::Selectable("Install from Folders")) {
                        install_mods_from_folders(ui_mod_vars);
                    }
                    ImGui::EndPopup();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Settings")) {

                
                float btn_width = 200.0f;
                float block_height = 140.0f;
                float avail_height = ImGui::GetContentRegionAvail().y;
                if (avail_height > block_height) {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail_height - block_height) * 0.5f - 30.0f);
                }

                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - btn_width) * 0.5f);
                if (ImGui::Button("Open Mad Max Directory", ImVec2(btn_width, 30.0f))) {
                    if (!g_game_path.empty()) {
                        ShellExecuteA(NULL, "open", g_game_path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
                    }
                }
                ImGui::Spacing();
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - btn_width) * 0.5f);
                if (ImGui::Button("Open Mods Directory", ImVec2(btn_width, 30.0f))) {
                    ShellExecuteA(NULL, "open", fs::absolute(MODS_DIR).string().c_str(), NULL, NULL, SW_SHOWDEFAULT);
                }
                ImGui::Spacing();
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - btn_width) * 0.5f);
                if (ImGui::Button("Open Mad Manager Directory", ImVec2(btn_width, 30.0f))) {
                    ShellExecuteA(NULL, "open", fs::current_path().string().c_str(), NULL, NULL, SW_SHOWDEFAULT);
                }
                
                ImGui::Spacing();
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - btn_width) * 0.5f);
                if (ImGui::Button("About Mad Manager", ImVec2(btn_width, 30.0f))) {
                    ImGui::OpenPopup("About Mad Manager");
                }
                


                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(ImVec2(190, 0), ImGuiCond_Always);
                ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.06f, 0.06f, 0.06f, 1.0f));
                if (ImGui::BeginPopup("About Mad Manager", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
                    float title_w = ImGui::CalcTextSize("Mad Manager by 0xOFF").x;
                    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - title_w) * 0.5f);
                    ImGui::Text("Mad Manager by 0xOFF");
                    ImGui::Spacing();
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    
                    float link_w1 = ImGui::CalcTextSize("Nexus Mods").x;
                    float link_w2 = ImGui::CalcTextSize("Github").x;
                    float total_links_w = link_w1 + 15.0f + link_w2;
                    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - total_links_w) * 0.5f);
                    
                    ImVec2 p_min = ImGui::GetCursorScreenPos();
                    ImGui::Text("Nexus Mods");
                    ImGui::GetWindowDrawList()->AddText(ImVec2(p_min.x + 1.0f, p_min.y), IM_COL32(255, 255, 255, 255), "Nexus Mods");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        if (ImGui::IsMouseClicked(0)) {
                            ShellExecuteA(NULL, "open", "https://www.nexusmods.com/madmax/mods/145", NULL, NULL, SW_SHOWNORMAL);
                        }
                    }
                    
                    ImGui::SameLine(0.0f, 15.0f);
                    
                    p_min = ImGui::GetCursorScreenPos();
                    ImGui::Text("Github");
                    ImGui::GetWindowDrawList()->AddText(ImVec2(p_min.x + 1.0f, p_min.y), IM_COL32(255, 255, 255, 255), "Github");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        if (ImGui::IsMouseClicked(0)) {
                            ShellExecuteA(NULL, "open", "https://github.com/y0xOFF/Mad-Manager", NULL, NULL, SW_SHOWNORMAL);
                        }
                    }
                    
                    ImGui::PopStyleColor();
                    ImGui::Spacing();
                    
                    ImGui::EndPopup();
                }
                ImGui::PopStyleColor();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
        }

        ImGui::Render();
        ImVec4 active_clear_color = clear_color;
        if (g_phase == PHASE_SPLASH_IN || g_phase == PHASE_SPLASH_WAIT || g_phase == PHASE_SPLASH_OUT) {
            active_clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        }

        const float clear_color_with_alpha[4] = { active_clear_color.x * active_clear_color.w, active_clear_color.y * active_clear_color.w, active_clear_color.z * active_clear_color.w, active_clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    if (hDir != INVALID_HANDLE_VALUE) FindClose(hDir);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    CoUninitialize();
    return 0;
}

// Helpers
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        UINT count = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);
        for (UINT i = 0; i < count; i++) {
            char path[MAX_PATH];
            DragQueryFileA(hDrop, i, path, MAX_PATH);
            g_dropped_files.push_back(path);
        }
        DragFinish(hDrop);
        return 0;
    }
    case WM_NCHITTEST:
    {
        LRESULT hit = ::DefWindowProcW(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            ::ScreenToClient(hWnd, &pt);
            if (pt.y < 30 && pt.x < 460) // Top 30 pixels, but not over the X button
                return HTCAPTION;
        }
        return hit;
    }
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_CLOSE:
        g_AppClosing = true;
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
