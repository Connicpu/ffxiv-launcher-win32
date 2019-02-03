#pragma once

#include <string>
#include <codecvt>

class Credentials
{
public:
    void Load();
    void Save();

    static bool REMEMBER_USERNAME;
    static bool REMEMBER_PASSWORD;
    static bool USE_OTP;

    static std::string USERNAME;
    static std::string PASSWORD;
    static std::string OTP;
};

extern Credentials CREDENTIALS;

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> UTF_CONVERT;
