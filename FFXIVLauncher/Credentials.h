#pragma once

#include <string>
#include <codecvt>
#include <filesystem>

class Credentials
{
public:
    void Load();
    LSTATUS Save();

    bool remember_username;
    bool remember_password;
    bool use_otp;
    bool skip_login;
    bool hide_username;

    std::filesystem::path game_dir;
    std::string username;
    std::string password;
    std::string otp;
};

extern Credentials CREDENTIALS;
extern std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> UTF_CONVERT;
