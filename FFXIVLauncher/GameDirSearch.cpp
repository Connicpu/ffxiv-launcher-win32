#include "Common.h"
#include "GameDirSearch.h"

enum class LocationType
{
    DriveRoot,
    ProgramFileRoot,
    SquareDir,
    SteamDir,
    BootDir,
};

static std::optional<fs::path> DescendFolder(LocationType type, const fs::path &path);
static std::optional<fs::path> DoDescendFolder(LocationType type, const fs::path &path);

using EnumerateSearchRoot = std::optional<std::pair<LocationType, fs::path>>(*)(int);

// Search roots
static std::optional<std::pair<LocationType, fs::path>> RootCwd(int i);
static std::optional<std::pair<LocationType, fs::path>> RootDrives(int i);
static EnumerateSearchRoot SEARCH_ROOTS[] = { RootCwd, RootDrives };

std::filesystem::path GetSelfPath()
{
    wchar_t self_path_buf[2048] = { 0 };
    GetModuleFileNameW(GetModuleHandleA(nullptr), self_path_buf, 2048);
    return fs::canonical(self_path_buf);
}

std::optional<fs::path> TryFindGameDir()
{
    for (auto root_iter : SEARCH_ROOTS)
    {
        int i = 0;
        while (auto root = root_iter(i++))
        {
            if (auto result = DescendFolder(root->first, root->second))
            {
                return fs::canonical(*result);
            }
        }
    }

    return std::nullopt;
}

static const wchar_t *FFXIV_FOLDER_NAME = L"FINAL FANTASY XIV - A Realm Reborn";
static const wchar_t *FFXIV_STEAM_NAME = L"Final Fantasy XIV Online";

static std::optional<fs::path> DescendFolder(LocationType type, const fs::path &path)
{
    try
    {
        return DoDescendFolder(type, path);
    }
    catch (fs::filesystem_error)
    {
        return std::nullopt;
    }
}

static std::optional<fs::path> DoDescendFolder(LocationType type, const fs::path &path)
{
    if (!fs::exists(path))
    {
        return std::nullopt;
    }

    OutputDebugStringW(L"Checking for the game in ");
    OutputDebugStringW(path.c_str());
    OutputDebugStringW(L"...\n");

    if (IsGameDir(path))
    {
        return path;
    }

    // Check for the FFXIV Folder at any time
    if (IsGameDir(path / FFXIV_FOLDER_NAME))
    {
        return path / FFXIV_FOLDER_NAME;
    }
    if (IsGameDir(path / FFXIV_STEAM_NAME))
    {
        return path / FFXIV_STEAM_NAME;
    }

    // Check for a SquareEnix folder at any time
    if (std::optional result = DescendFolder(LocationType::SquareDir, path / L"SquareEnix"))
    {
        return result;
    }

    switch (type)
    {
        case LocationType::BootDir:
        {
            if (IsGameDir(path / L".."))
            {
                return path / L"..";
            }

            break;
        }

        case LocationType::DriveRoot:
        {
            if (std::optional result = DescendFolder(LocationType::ProgramFileRoot, path / L"Program Files"))
            {
                return result;
            }
            if (std::optional result = DescendFolder(LocationType::ProgramFileRoot, path / L"Program Files (x86)"))
            {
                return result;
            }
        }
        case LocationType::ProgramFileRoot:
        {
            if (std::optional result = DescendFolder(LocationType::SteamDir, path / L"Steam"))
            {
                return result;
            }
            if (std::optional result = DescendFolder(LocationType::SteamDir, path / L"SteamLibrary"))
            {
                return result;
            }
            if (std::optional result = DescendFolder(LocationType::ProgramFileRoot, path / L"Games"))
            {
                return result;
            }

            break;
        }

        case LocationType::SteamDir:
        {
            if (std::optional result = DescendFolder(LocationType::ProgramFileRoot, path / L"steamapps/common"))
            {
                return result;
            }

            break;
        }

        case LocationType::SquareDir:
        {
            // nothing else to check
            break;
        }
    }

    return std::nullopt;
}

bool IsGameDir(const fs::path &path)
{
    auto hasgame = fs::exists(path / "game");
    auto hasboot = fs::exists(path / "boot");
    auto hasexe = fs::exists(path / "game/ffxiv_dx11.exe");
    return hasgame && hasboot && hasexe;
}

std::optional<std::pair<LocationType, fs::path>> RootCwd(int i)
{
    if (i > 0) return std::nullopt;
    return { { LocationType::BootDir, fs::current_path() } };
}

std::optional<std::pair<LocationType, fs::path>> RootDrives(int i)
{
    static thread_local wchar_t DRIVE_LETTER_BUF[2048];
    static thread_local size_t BUF_LEN = GetLogicalDriveStringsW(2048, DRIVE_LETTER_BUF);

    const size_t num_drives = BUF_LEN / 4;
    if (i < num_drives)
    {
        return { { LocationType::DriveRoot, &DRIVE_LETTER_BUF[i * 4] } };
    }
    else
    {
        return std::nullopt;
    }
}