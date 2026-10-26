#include <windows.h>
#include "augment/augment.hpp"
#include <string>

// GameStruct
constexpr int GAME_STRUCT_PTR_OFFSET       = 0x22FD88;
constexpr int SATISFACTION_OFFSET          = 0x10618;
constexpr int MAX_SATISFACTION_OFFSET      = 0x10620;

// main loop
constexpr int GAME_LOOP_PROC_OFFSET        = 0xA0700;

// dijni editor
constexpr int GAME_EDITOR_PTR_OFFSET       = 0x22FD80;
constexpr int DEV_OVERLAY_OFFSET           = 0x1C1;

// ImGui, statically linked into Robomo.exe: we call the game's own copies
constexpr int IMGUI_BEGIN_OFFSET           = 0xC67B0;
constexpr int IMGUI_END_OFFSET             = 0xC9AA0;
constexpr int IMGUI_CHECKBOX_OFFSET        = 0xFE500;

using ImGuiBeginFn = bool (*)(const char *name, bool *open, int flags);
using ImGuiEndFn = void(*)();
using ImGuiCheckboxFn = bool(*)(const char* label, bool* value);

using OriginalLoopFn = void(*)();
static OriginalLoopFn s_originalLoop = nullptr;
static bool s_init = false;

// toggled from the cheat menu
struct CheatState {
    bool lockSatisfaction = false;
};
static CheatState s_cheats;

static void* grab_game_offset(int offset) {
    // grab the base address of Robomo's executable
    static uint8_t* base = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    return base + offset;
}

static void enable_editor_overlay(void) {
    // idempotent, since we call this every frame in the game loop
    if (!s_init) {
        s_init = true;
        
        auto editor_struct = *reinterpret_cast<uintptr_t*>(grab_game_offset(GAME_EDITOR_PTR_OFFSET));

        uint8_t* overlay_gate =
            reinterpret_cast<uint8_t*>(editor_struct + DEV_OVERLAY_OFFSET);
    
        // enable the debug overlay editor code path
        *overlay_gate = 1;
    }
}

static void draw_cheat_menu(void){
    // must run while the game's ImGui frame is live
    static auto im_begin = reinterpret_cast<ImGuiBeginFn>(grab_game_offset(IMGUI_BEGIN_OFFSET));
    static auto im_end = reinterpret_cast<ImGuiEndFn>(grab_game_offset(IMGUI_END_OFFSET));
    static auto im_checkbox = reinterpret_cast<ImGuiCheckboxFn>(grab_game_offset(IMGUI_CHECKBOX_OFFSET));

    auto editor_struct = *reinterpret_cast<uintptr_t **>(grab_game_offset(GAME_EDITOR_PTR_OFFSET));

    // End() has to be called even when Begin() returns false (collapsed window)
    im_begin("Robomo Cheats", nullptr, 0);

    im_checkbox("lock satisfaction at max", &s_cheats.lockSatisfaction);

    if (editor_struct != nullptr) {
        im_checkbox("Dev overlay", reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(editor_struct) + DEV_OVERLAY_OFFSET));
    }

    im_end();
}

static void game_loop_hook(void) {
    enable_editor_overlay();

    // grab the game's struct offset location (dereference the ptr)
    auto game_struct = *reinterpret_cast<uintptr_t **>(
        grab_game_offset(GAME_STRUCT_PTR_OFFSET)
    );

    if (game_struct != nullptr && s_cheats.lockSatisfaction) {
        uint8_t* game_bytes = reinterpret_cast<uint8_t*>(game_struct);

        float* satisfaction_meter_float = reinterpret_cast<float*>(game_bytes + SATISFACTION_OFFSET);
        float max_satisfaction = *reinterpret_cast<float*>(game_bytes + MAX_SATISFACTION_OFFSET);

        // pin it to the level's real max (the game checks satistfaction == max)
        *satisfaction_meter_float = max_satisfaction;
    }

    // call the original loop function
    if (s_originalLoop != nullptr) {
        s_originalLoop();
    }

    // draw after the original: the game calls ImGui from inside that function
    //(hud text), so an ImGui frame is already open at this point
    draw_cheat_menu();
}

static void install_hooks() {
    void* loopAddr = grab_game_offset(GAME_LOOP_PROC_OFFSET);

    augment_hook_ptr(loopAddr, reinterpret_cast<void*>(&game_loop_hook), reinterpret_cast<void**>(&s_originalLoop));
    augment_install_all();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule); // skip unneeded notifications
            CreateThread(nullptr, 0,
                [](LPVOID) -> DWORD {
                    install_hooks();
                    return 0;
                },
                nullptr, 0, nullptr);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
