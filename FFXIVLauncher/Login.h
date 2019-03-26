#pragma once

enum class LoginResult
{
    Success,
    InvalidCredentials,
    TooManyLogins,
    UpdateRequired,
    CuckedBySquare,
    NetworkError,
    Maintenance,
};

LoginResult PerformLogin();
void LaunchGame();
void LaunchUpdater();
bool BootWasReplaced();
bool IsLobbyServerReady();

bool HashFile(byte(&buf)[160 / 8], const fs::path &path);

struct FileHash { byte buf[160 / 8]; };
inline std::optional<FileHash> HashFile(const fs::path &path)
{
    FileHash hash;
    if (!HashFile(hash.buf, path))
        return {};
    return hash;
}

inline bool operator==(const FileHash &lhs, const FileHash &rhs)
{
    return memcmp(lhs.buf, rhs.buf, sizeof(lhs.buf)) == 0;
}

inline bool operator!=(const FileHash &lhs, const FileHash &rhs)
{
    return !(lhs == rhs);
}
