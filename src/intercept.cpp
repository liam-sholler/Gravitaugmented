#include <windows.h>
#include "augment/augment.hpp"
#include <string>

constexpr int GAME_STRUCT_OFFSET = 0x22FD88;
constexpr int SATISFACTION_OFFSET = 0x10618;
constexpr int GAME_LOOP_PROC_OFFSET = 0xA0700;

bool write_file(const std::string& path, const std::string& data)
{
    HANDLE hFile = CreateFileA(
        path.c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD written = 0;
    DWORD size = static_cast<DWORD>(data.size());

    bool success = WriteFile(
        hFile,
        data.data(),
        size,
        &written,
        nullptr
    ) && written == size;

    CloseHandle(hFile);
    return success;
}

static void* grab_game_offset(int offset) {
    // grab the base address of Robomo's executable
    static uint8_t* base = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    return base + offset;
}

using OriginalFn = void(*)();

static OriginalFn original = nullptr;

static void game_loop_hook(void) {
    std::string path = "C:/Users/gci.252892/ProgramFiles/tmp/test_hook.txt";
    std::string text = "Hooked game loop test!\n";
    write_file(path, text);
    if (original != nullptr) {
        original();
    }
}   

static void install_hooks() {
    void* loopAddr = grab_game_offset(GAME_LOOP_PROC_OFFSET);
    AugmentRegOpts opts = {};
    augment_hook_ptr(loopAddr, reinterpret_cast<void*>(&game_loop_hook), reinterpret_cast<void**>(&original));

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
