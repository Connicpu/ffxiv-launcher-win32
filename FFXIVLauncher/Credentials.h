#pragma once

#include <string>
#include <codecvt>
#include <filesystem>

class Credentials
{
public:
    void Load();
    LSTATUS Save();

    bool REMEMBER_USERNAME;
    bool REMEMBER_PASSWORD;
    bool USE_OTP;
    bool SKIP_LOGIN;
    bool HIDE_USERNAME;

    std::filesystem::path GAME_DIR;
    std::string USERNAME;
    std::string PASSWORD;
    std::string OTP;
};

extern Credentials CREDENTIALS;
extern std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> UTF_CONVERT;
