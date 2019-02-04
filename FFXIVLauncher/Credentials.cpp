#include "Common.h"
#include "Credentials.h"
#include <optional>

static std::optional<std::string> Encrypt(const std::string &data);
static std::optional<std::string> Decrypt(const std::string &data);

static LSTATUS SaveToRegistry(const std::vector<uint8_t> &data) noexcept;
static LSTATUS ReadFromRegistry(std::vector<uint8_t> &data);

static void WriteBufferStr(std::vector<uint8_t> &buf, const std::string &data);
static bool ReadBufferStr(const std::vector<uint8_t> &buf, size_t &pos, std::string &data);

bool Credentials::REMEMBER_USERNAME;
bool Credentials::REMEMBER_PASSWORD;
bool Credentials::USE_OTP;
bool Credentials::SKIP_LOGIN;
bool Credentials::HIDE_USERNAME;

fs::path Credentials::GAME_DIR{ "C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV - A Realm Reborn" };
std::string Credentials::USERNAME;
std::string Credentials::PASSWORD;
std::string Credentials::OTP;

Credentials CREDENTIALS;
std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> UTF_CONVERT;

void Credentials::Load()
{
    std::vector<uint8_t> data;
    if (ReadFromRegistry(data) != S_OK) return;
    if (data.size() < 9) return;

    REMEMBER_USERNAME = data[0] != 0;
    REMEMBER_PASSWORD = data[1] != 0;
    USE_OTP = data[2] != 0;
    SKIP_LOGIN = data[3] != 0;
    HIDE_USERNAME = data[4] != 0;

    size_t pos = 5;
    std::string temp;
    if (ReadBufferStr(data, pos, temp))
    {
        if (auto username = Decrypt(temp))
        {
            USERNAME = std::move(*username);
        }
    }
    if (ReadBufferStr(data, pos, temp))
    {
        if (auto password = Decrypt(temp))
        {
            PASSWORD = std::move(*password);
        }
    }
    if (ReadBufferStr(data, pos, temp))
    {
        GAME_DIR = temp;
    }
}

LSTATUS Credentials::Save()
{
    auto encUsername = Encrypt(USERNAME);
    if (!encUsername) return GetLastError();

    auto encPassword = Encrypt(PASSWORD);
    if (!encPassword) return GetLastError();

    if (!REMEMBER_USERNAME)
    {
        encUsername->clear();
    }
    if (!REMEMBER_PASSWORD)
    {
        encPassword->clear();
    }

    if (encUsername->size() >= 0x10000 || encPassword->size() >= 0x10000)
    {
        return ERROR_INVALID_DATA;
    }

    std::vector<uint8_t> data;
    data.reserve(9 + encUsername->size() + encPassword->size());

    data.push_back(REMEMBER_USERNAME);
    data.push_back(REMEMBER_PASSWORD);
    data.push_back(USE_OTP);
    data.push_back(SKIP_LOGIN);
    data.push_back(HIDE_USERNAME);

    WriteBufferStr(data, *encUsername);
    WriteBufferStr(data, *encPassword);
    WriteBufferStr(data, GAME_DIR.generic_string());

    return SaveToRegistry(data);
}

static uint8_t ENTROPY[] = "\
\x0b\xb5\x91\xe2\xe1\x5f\x6d\xe2\x44\x88\x12\xf1\x1b\x41\xf5\x63\
\x9e\x29\x2a\xa6\xd2\x87\xf7\x8d\x06\x39\x06\x25\xc2\x30\xc4\xb8\
\x9b\x20\xc5\x99\x4a\x59\xf7\x37\xce\x97\x99\x83\xd4\x3a\xc2\xbc\
\xbc\xf4\x45\x9b\x9f\xb8\xca\x70\x3e\xde\x04\x23\xa7\xee\xbe\x9f";

static DATA_BLOB ENTROPY_BLOB = { sizeof(ENTROPY) - 1, ENTROPY };

template <typename Func>
static std::optional<std::string> DoCrypt(const std::string &data, Func docrypt)
{
    DATA_BLOB input;
    input.pbData = (uint8_t *)data.c_str();
    input.cbData = (uint32_t)data.size();

    DATA_BLOB output = { 0, nullptr };

    const auto res = docrypt(&input, nullptr, &ENTROPY_BLOB, nullptr, nullptr, 0, &output);
    if (!res)
    {
        return std::nullopt;
    }

    std::string result((char *)output.pbData, output.cbData);
    LocalFree(output.pbData);
    return std::optional(std::move(result));
}

static std::optional<std::string> Encrypt(const std::string &data)
{
    return DoCrypt(data, CryptProtectData);
}

static std::optional<std::string> Decrypt(const std::string &data)
{
    return DoCrypt(data, CryptUnprotectData);
}

static const wchar_t *REG_KEY = L"Software\\NeverBeenMad\\FFXIVFastLauncher";
static const wchar_t *REG_PARAM = L"CredentialsAndPreferences";

static LSTATUS SaveToRegistry(const std::vector<uint8_t> &data) noexcept
{
    HKEY hKey = nullptr;
    LSTATUS res = 0;

    res = RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    if (res != S_OK)
    {
        return res;
    }

    res = RegSetValueExW(hKey, REG_PARAM, 0, REG_BINARY, data.data(), (DWORD)data.size());
    if (res != S_OK)
    {
        return res;
    }

    return S_OK;
}

static LSTATUS ReadFromRegistry(std::vector<uint8_t> &data)
{
    HKEY hKey = nullptr;
    LSTATUS res = 0;

    res = RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &hKey);
    if (res != S_OK)
    {
        return res;
    }

    DWORD type = 0;
    DWORD len = 0;
    res = RegQueryValueExW(hKey, REG_PARAM, nullptr, &type, nullptr, &len);
    if (res != S_OK || type != REG_BINARY)
    {
        return ERROR_INVALID_DATA;
    }

    data.resize(len);
    res = RegQueryValueExW(hKey, REG_PARAM, nullptr, &type, data.data(), &len);
    if (res != S_OK)
    {
        return res;
    }

    return S_OK;
}

static void WriteBufferStr(std::vector<uint8_t> &buf, const std::string &data)
{
    buf.push_back(data.size() & 0xFF);
    buf.push_back((data.size() >> 8) & 0xFF);
    buf.insert(buf.end(), data.begin(), data.end());
}

static bool ReadBufferStr(const std::vector<uint8_t> &buf, size_t &pos, std::string &data)
{
    data.clear();

    if (pos + 2 > buf.size())
    {
        return false;
    }

    size_t len = buf[pos] | (((size_t)buf[pos + 1]) << 8);
    pos += 2;

    if (pos + len > buf.size())
    {
        return false;
    }

    if (len == 0)
    {
        return true;
    }

    data.assign((char *)&buf[pos], len);
    pos += len;

    return true;
}
