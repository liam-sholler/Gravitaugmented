#include <filesystem>
#include <windows.h>
#include "robomo.hpp"

int main()
{
    // 1: choose the process
    std::filesystem::path path = robomo::chooseProcess();
    if (path.empty() || path.filename().string().find("robomo") == std::string::npos) exit(-1);

    std::string pathStr = path.string();
    PROCESS_INFORMATION info{};
    HANDLE hOutput = nullptr;

    // 2: open the process suspended
    bool opened = robomo::openSubprocess(
        pathStr, 
        info, 
        hOutput,
        path.parent_path().string(),
        true
    );
    if (!opened) exit(-1);

    // 3: hook the built DLL (intercept.cpp)
    auto dir = robomo::getExeDirectory() / L"libintercept.dll";
    std::string fileStr = dir.string();

    bool injected = robomo::injectDll(info, fileStr);
    if (!injected) exit(-1);

    // resume the subprocess
    ResumeThread(info.hThread);

    char buffer[4096];
    DWORD bytesRead = 0;

    while (ReadFile(
        hOutput,
        buffer,
        sizeof(buffer),
        &bytesRead,
        nullptr
    ) && bytesRead > 0) {
        // Output may contain binary data; use bytesRead.
        fwrite(buffer, 1, bytesRead, stdout);
        fflush(stdout);
    }


    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);
}
