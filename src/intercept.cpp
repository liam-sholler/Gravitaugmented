#include <windows.h>
#include "augment/augment.hpp"
#include <string>

// GameStruct
constexpr int GAME_STRUCT_PTR_OFFSET       = 0x22FD88;
constexpr int SATISFACTION_OFFSET          = 0x10618;

// main loop
constexpr int GAME_LOOP_PROC_OFFSET        = 0xA0700;

// dijni editor
constexpr int GAME_EDITOR_PTR_OFFSET       = 0x22FD80;
constexpr int DEV_OVERLAY_OFFSET           = 0x1C1;

using OriginalLoopFn = void(*)();
static OriginalLoopFn s_originalLoop = nullptr;
static bool s_init = false;

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
        uint8_t* options_enabled =
            reinterpret_cast<uint8_t*>(editor_struct + DEBUG_OPTIONS_ENABLED_OFFSET);

        // enable the debug overlay editor code path
        *overlay_gate = 1;
    }
}

static void game_loop_hook(void) {
    enable_editor_overlay();

    // grab the game's struct offset location (dereference the ptr)
    auto game_struct = *reinterpret_cast<uintptr_t*>(
        grab_game_offset(GAME_STRUCT_PTR_OFFSET)
    );

    // grab the satisfaction float relative to that game offset
    float* satisfaction_meter_float =
        reinterpret_cast<float*>(game_struct + SATISFACTION_OFFSET);

    // set it to max
    *satisfaction_meter_float = 100.0f;

    // call the original loop function
    if (s_originalLoop != nullptr) {
        s_originalLoop();
    }
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
