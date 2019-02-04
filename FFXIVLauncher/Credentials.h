#pragma once

#include <string>
#include <codecvt>
#include <filesystem>

class Credentials
{
public:
    static void Load();
    static LSTATUS Save();

    static bool REMEMBER_USERNAME;
    static bool REMEMBER_PASSWORD;
    static bool USE_OTP;
    static bool SKIP_LOGIN;
    static bool HIDE_USERNAME;

    static std::filesystem::path GAME_DIR;
    static std::string USERNAME;
    static std::string PASSWORD;
    static std::string OTP;
};

extern std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> UTF_CONVERT;
