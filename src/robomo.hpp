#pragma once
#include <string>
#include <filesystem>
#include <windows.h>

namespace robomo {

// subprocess lifecycle
std::filesystem::path getExeDirectory();
std::filesystem::path chooseProcess();
bool openSubprocess(std::string& process, PROCESS_INFORMATION& outProcInfo, HANDLE& outReadPipe, const std::string& workingDir, bool suspended = false);

// interception
bool injectDll(const PROCESS_INFORMATION& info, const std::string& dllPath);

}
