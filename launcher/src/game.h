// Finding and starting the port from the launcher. Win32 lives in game_win.cpp: windows.h clashes with raylib's names
// (CloseWindow, DrawText, Rectangle...), so main.cpp only sees this interface. Strings are UTF-8.
#pragma once

#include <string>
#include <utility>
#include <vector>

struct GamePaths
{
    std::string root;    // the repo folder: the runner's working directory (memory cards, game/, out/)
    std::string runner;  // out/build/ps2xRuntime/<config>/ps2EntryRunner.exe, the newest build
    std::string elf;     // game/disc/<name in game/elf-name.txt>
    std::string elfName; // SCUS_974.81
    std::string error;   // empty when everything was found
};

// Walks up from the launcher's folder to the repo (the folder with game/elf-name.txt) and finds the runner and the ELF.
GamePaths findGame(const std::string &launcherDir);

// Starts the runner with the ELF, in the repo folder, with these environment variables (an empty value removes the
// variable); stdout and stderr go to logPath. Returns false and fills error if it could not start.
bool launchGame(const GamePaths &paths, const std::vector<std::pair<std::string, std::string>> &env, const std::string &logPath,
                std::string &error);

// The game started by launchGame: -2 none, -1 still running, otherwise its exit code.
long gameStatus();

// Opens a folder in Explorer (created first if missing).
void openFolder(const std::string &path);

// "out/logs/launcher-YYYYMMDD-HHMMSS.log" under root.
std::string newLogPath(const std::string &root);
