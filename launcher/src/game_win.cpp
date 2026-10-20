// Win32 side of game.h: finding the repo, starting ps2EntryRunner and opening folders.
#include "game.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include <ctime>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace
{
HANDLE g_process = nullptr;

std::wstring wide(const std::string &s)
{
    if (s.empty())
        return std::wstring();
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

std::string utf8(const fs::path &p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

fs::path pathOf(const std::string &s) { return fs::path(wide(s)); }

std::string lastError()
{
    wchar_t *msg = nullptr;
    const DWORD code = GetLastError();
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0,
                   reinterpret_cast<wchar_t *>(&msg), 0, nullptr);
    std::wstring w = msg ? msg : L"";
    if (msg)
        LocalFree(msg);
    while (!w.empty() && (w.back() == L'\n' || w.back() == L'\r' || w.back() == L'.'))
        w.pop_back();
    return utf8(fs::path(w)) + " (" + std::to_string(code) + ")";
}
} // namespace

GamePaths findGame(const std::string &launcherDir)
{
    GamePaths g;
    std::error_code ec;
    fs::path dir = pathOf(launcherDir);
    fs::path root;
    for (int i = 0; i < 10 && !dir.empty(); ++i)
    {
        if (fs::exists(dir / "game" / "elf-name.txt", ec))
        {
            root = dir;
            break;
        }
        if (dir == dir.parent_path())
            break;
        dir = dir.parent_path();
    }
    if (root.empty())
    {
        g.error = "Game not found: extract the disc first (scripts\\extract-iso.ps1)";
        return g;
    }
    g.root = utf8(root);

    std::ifstream nameFile(root / "game" / "elf-name.txt");
    std::getline(nameFile, g.elfName);
    while (!g.elfName.empty() && (g.elfName.back() == '\r' || g.elfName.back() == ' '))
        g.elfName.pop_back();
    const fs::path elf = root / "game" / "disc" / pathOf(g.elfName);
    if (g.elfName.empty() || !fs::exists(elf, ec))
    {
        g.error = "Game ELF missing: extract the disc again (scripts\\extract-iso.ps1)";
        return g;
    }
    g.elf = utf8(elf);

    // the newest of the runner builds
    fs::file_time_type newest{};
    for (const char *config : {"RelWithDebInfo", "Release", "Debug"})
    {
        const fs::path exe = root / "out" / "build" / "ps2xRuntime" / config / "ps2EntryRunner.exe";
        if (!fs::exists(exe, ec))
            continue;
        const fs::file_time_type t = fs::last_write_time(exe, ec);
        if (g.runner.empty() || t > newest)
        {
            g.runner = utf8(exe);
            newest = t;
        }
    }
    if (g.runner.empty())
        g.error = "Runner not built: run scripts\\run.ps1 -Config RelWithDebInfo once";
    return g;
}

std::string newLogPath(const std::string &root)
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    char name[64];
    std::strftime(name, sizeof(name), "launcher-%Y%m%d-%H%M%S.log", &local);
    return utf8(pathOf(root) / "out" / "logs" / name);
}

bool launchGame(const GamePaths &paths, const std::vector<std::pair<std::string, std::string>> &env, const std::string &logPath,
                std::string &error)
{
    if (g_process)
    {
        CloseHandle(g_process);
        g_process = nullptr;
    }
    std::error_code ec;
    fs::create_directories(pathOf(logPath).parent_path(), ec);

    // the child inherits the launcher's environment
    for (const auto &[name, value] : env)
        SetEnvironmentVariableW(wide(name).c_str(), value.empty() ? nullptr : wide(value).c_str());

    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE log = CreateFileW(wide(logPath).c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
    if (log == INVALID_HANDLE_VALUE)
    {
        error = "Could not create the log: " + lastError();
        return false;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = nullptr;
    si.hStdOutput = log;
    si.hStdError = log;
    PROCESS_INFORMATION pi{};
    std::wstring cmd = L"\"" + wide(paths.runner) + L"\" \"" + wide(paths.elf) + L"\"";
    const BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                                   wide(paths.root).c_str(), &si, &pi);
    if (!ok)
        error = "Could not start the runner: " + lastError();
    CloseHandle(log);
    if (!ok)
        return false;
    CloseHandle(pi.hThread);
    g_process = pi.hProcess;
    return true;
}

long gameStatus()
{
    if (!g_process)
        return -2;
    if (WaitForSingleObject(g_process, 0) == WAIT_TIMEOUT)
        return -1;
    DWORD code = 0;
    GetExitCodeProcess(g_process, &code);
    return static_cast<long>(code);
}

void openFolder(const std::string &path)
{
    std::error_code ec;
    fs::create_directories(pathOf(path), ec);
    ShellExecuteW(nullptr, L"open", wide(path).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
