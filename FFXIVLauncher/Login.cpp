#include "Common.h"
#include "Login.h"
#include "Http.h"
#include "Credentials.h"
#include "GameDirSearch.h"

struct LoginResponse
{
    void Parse(std::string_view str);

    std::string auth;
    std::string sid;
    int terms;
    int region;
    int etmadd;
    int playable;
    int ps3pkg;
    int maxex;
    int product;
};

static std::string GetLocalGamever();
static LoginResult GetStored(std::string &result);
static LoginResult GetBaseLogin(const std::string &stored, LoginResponse &result);
static LoginResult GetRealSID(const std::string &sid, std::string &result);
static std::vector<uint8_t> FormEncode(std::initializer_list<std::pair<const char *, std::string_view>> const &data);
static void EncodeURIElem(std::ostream &output, std::string_view data);
static void EncodeFormURIComponent(std::ostream &output, char c);
static bool HashFile(std::ostream &output, const fs::path &path);

static std::string REAL_SID;
static int LANGUAGE = 3;
static int REGION = 3;
static bool DX11 = true;
static int EXPANSION_LEVEL = 0;
static std::string GAME_VER;

constexpr const char *LOGIN_PAGE = "https://ffxiv-login.square-enix.com/oauth/ffxivarr/login/top?lng=en&rgn=";
constexpr const char *LOGIN_URL = "https://ffxiv-login.square-enix.com/oauth/ffxivarr/login/login.send";
constexpr const char *SESSION_URL = "https://patch-gamever.ffxiv.com/http/win32/ffxivneo_release_game/";

static const std::regex STORED_PAT{ R"#(<input type="hidden" name="_STORED_" value="([^"]+)")#" };
static const std::regex TOO_MANY_LOGINS_PAT{ R"#(^\s*window\.external\.user\("login=auth,ng,err,Because password entry has failed multiple times)#" };
static const std::regex INVALID_CRED_PAT{ R"#(^\s*window\.external\.user\("login=auth,ng,err)#" };
static const std::regex LOGIN_PAT{ R"#(^\s*window\.external\.user\("login=(.*)"\);)#" };

LoginResult PerformLogin()
{
    LoginResult res;

    GAME_VER = GetLocalGamever();

    std::string stored;
    res = GetStored(stored);
    if (res != LoginResult::Success) return res;

    LoginResponse login_info;
    res = GetBaseLogin(stored, login_info);
    if (res != LoginResult::Success) return res;

    EXPANSION_LEVEL = login_info.maxex;
    REGION = login_info.region;

    res = GetRealSID(login_info.sid, REAL_SID);
    if (res != LoginResult::Success) return res;

    return LoginResult::Success;
}

void LaunchGame()
{
    fs::path pexe = CREDENTIALS.game_dir / "game";
    pexe /= DX11 ? "ffxiv_dx11.exe" : "ffxiv.exe";
    auto exe = pexe.generic_string();

    std::ostringstream arb;
    arb << "language=1"
        << " DEV.UseSqPack=1"
        << " DEV.DataPathType=1"
        << " DEV.LobbyHost01=neolobby01.ffxiv.com"
        << " DEV.LobbyPort01=54994"
        << " DEV.LobbyHost02=neolobby02.ffxiv.com"
        << " DEV.LobbyPort02=54994"
        << " DEV.TestSID=" << REAL_SID
        << " DEV.MaxEntitledExpansionID=" << EXPANSION_LEVEL
        << " SYS.Region=" << REGION
        << " ver=" << GAME_VER;
    auto args = arb.str();

    auto cwd = (CREDENTIALS.game_dir / "boot").generic_string();

    STARTUPINFOA startup = { sizeof(startup) };
    PROCESS_INFORMATION info = { 0 };
    CreateProcessA(exe.c_str(), args.data(), nullptr, nullptr, 0, 0, nullptr, cwd.c_str(), &startup, &info);
}

void LaunchUpdater()
{
    auto self_path = GetSelfPath();
    auto boot = fs::canonical(CREDENTIALS.game_dir / "boot/ffxivboot.exe");

    if (self_path == boot)
    {
        auto temp = fs::temp_directory_path() / "ffxivboot.temp.exe";
        fs::copy_file(self_path, temp, fs::copy_options::overwrite_existing);

        auto pid = GetCurrentProcessId();
        auto args = L"update_ffxivboot " + std::to_wstring(pid);

        auto hWait = CreateSemaphoreW(nullptr, 0, 1, L"Global\\FFXIV_LAUNCHER_TEMP_UPDATER_WAIT");

        SHELLEXECUTEINFOW info = { sizeof(info) };
        info.lpVerb = L"runas";
        info.lpFile = temp.c_str();
        info.lpParameters = args.c_str();
        ShellExecuteExW(&info);

        WaitForSingleObject(hWait, 5000);
        CloseHandle(hWait);
    }
    else
    {
        auto cwd = CREDENTIALS.game_dir / "boot";

        STARTUPINFOW startup = { sizeof(startup) };
        PROCESS_INFORMATION info = { 0 };
        CreateProcessW(boot.c_str(), nullptr, nullptr, nullptr, 0, 0, nullptr, cwd.c_str(), &startup, &info);
    }
}

static std::string GetLocalGamever()
{
    char data[1024];

    auto path = (CREDENTIALS.game_dir / "game/ffxivgame.ver").generic_wstring();
    auto hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return "womp-womp";

    DWORD numRead, pos = 0;
    BOOL res;
    while (res = ReadFile(hFile, data + pos, sizeof(data) - pos, &numRead, nullptr))
    {
        pos += numRead;
        if (numRead == 0) break;
    }

    if (!res) return "womp-womp";

    return std::string(data, pos);
}

static LoginResult GetStored(std::string & result)
{
    Request req = {
        Method::GET,
        LOGIN_PAGE + std::to_string(LANGUAGE),
    };

    Response resp;
    auto hr = DoRequest(req, resp);
    if (FAILED(hr)) return LoginResult::NetworkError;

    std::string utf8_result(resp.body.begin(), resp.body.end());

    std::smatch matches;
    if (!std::regex_search(utf8_result, matches, STORED_PAT))
    {
        return LoginResult::CuckedBySquare;
    }

    result = matches[1];

    return LoginResult::Success;
}

static LoginResult GetBaseLogin(const std::string & stored, LoginResponse & result)
{
    Request req = {
        Method::POST,
        LOGIN_URL,
        {
            {"Referer", LOGIN_PAGE + std::to_string(LANGUAGE)},
            {"Content-Type", "application/x-www-form-urlencoded"},
        },
        FormEncode({
            {"_STORED_", stored},
            {"sqexid", CREDENTIALS.username},
            {"password", CREDENTIALS.password},
            {"otppw", CREDENTIALS.otp},
        }),
    };

    Response resp;
    auto hr = DoRequest(req, resp);
    if (FAILED(hr)) return LoginResult::NetworkError;

    std::string utf8_result(resp.body.begin(), resp.body.end());

    std::smatch matches;
    if (!std::regex_search(utf8_result, matches, LOGIN_PAT))
    {
        if (std::regex_search(utf8_result, TOO_MANY_LOGINS_PAT))
        {
            return LoginResult::TooManyLogins;
        }
        if (std::regex_search(utf8_result, INVALID_CRED_PAT))
        {
            return LoginResult::InvalidCredentials;
        }
        return LoginResult::CuckedBySquare;
    }

    result.Parse(matches[1].str());

    return LoginResult::Success;
}

static LoginResult GetRealSID(const std::string & sid, std::string & result)
{
    std::ostringstream hashstrb;
    hashstrb << "ffxivboot.exe/";
    if (BootWasReplaced())
    {
        HashFile(hashstrb, CREDENTIALS.game_dir / "boot/ffxivboot.exe.old");
    }
    else
    {
        HashFile(hashstrb, CREDENTIALS.game_dir / "boot/ffxivboot.exe");
    }
    hashstrb << ",ffxivboot64.exe/";
    HashFile(hashstrb, CREDENTIALS.game_dir / "boot/ffxivboot64.exe");
    hashstrb << ",ffxivlauncher.exe/";
    HashFile(hashstrb, CREDENTIALS.game_dir / "boot/ffxivlauncher.exe");
    hashstrb << ",ffxivlauncher64.exe/";
    HashFile(hashstrb, CREDENTIALS.game_dir / "boot/ffxivlauncher64.exe");
    hashstrb << ",ffxivupdater.exe/";
    HashFile(hashstrb, CREDENTIALS.game_dir / "boot/ffxivupdater.exe");
    hashstrb << ",ffxivupdater64.exe/";
    HashFile(hashstrb, CREDENTIALS.game_dir / "boot/ffxivupdater64.exe");
    auto hashstr = hashstrb.str();

    Request req = {
        Method::POST,
        SESSION_URL + GAME_VER + "/" + sid,
        {
            {"X-Hash-Check", "enabled"},
            {"Referer", LOGIN_PAGE + std::to_string(LANGUAGE)},
            {"Content-Type", "application/x-www-form-urlencoded"},
        },
    };

    req.body.assign(hashstr.begin(), hashstr.end());

    Response resp;
    auto hr = DoRequest(req, resp);
    if (FAILED(hr)) return LoginResult::NetworkError;

    if (resp.status == 409)
    {
        return LoginResult::UpdateRequired;
    }

    result = resp.patch_id;

    return LoginResult::Success;
}

static std::vector<uint8_t> FormEncode(std::initializer_list<std::pair<const char *, std::string_view>> const &data)
{
    std::ostringstream result;
    bool first = true;

    for (auto &item : data)
    {
        if (first) first = false;
        else result << '&';

        EncodeURIElem(result, item.first);
        result << '=';
        EncodeURIElem(result, item.second);
    }

    std::vector<uint8_t> buf;
    auto res = result.str();
    buf.assign(res.begin(), res.end());
    return std::move(buf);
}

static void EncodeURIElem(std::ostream & output, std::string_view data)
{
    for (char c : data)
    {
        EncodeFormURIComponent(output, c);
    }
}

static void EncodeFormURIComponent(std::ostream & output, char c)
{
    if (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z' || c >= '0' && c <= '9')
    {
        output << c;
        return;
    }

    switch (c)
    {
        case ' ':
        {
            output << '+';
            return;
        }

        case '-':
        case '_':
        case '.':
        {
            output << c;
            return;
        }

        default:
        {
            output << '%';
            output << std::setfill('0') << std::setw(2) << std::hex << (uint32_t)c;
            return;
        }
    }
}

bool HashFile(std::ostream & output, const fs::path & path)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    auto wPath = path.generic_wstring();
    auto hFile = CreateFileW(wPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    CryptAcquireContextW(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);

    BOOL res;
    BYTE buf[0x4000];
    DWORD numread;
    while (res = ReadFile(hFile, buf, sizeof(buf), &numread, nullptr))
    {
        if (numread == 0) break;
        CryptHashData(hHash, buf, numread, 0);
    }

    CloseHandle(hFile);

    if (!res)
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return false;
    }

    numread = 160 / 8;
    CryptGetHashParam(hHash, HP_HASHVAL, buf, &numread, 0);

    output << fs::file_size(path) << '/';
    output << std::hex;
    for (size_t i = 0; i < numread; i++)
    {
        output << std::setfill('0') << std::setw(2) << (uint32_t)buf[i];
    }
    output << std::dec;

    return true;
}

bool BootWasReplaced()
{
    BOOL res;

    // Get the buffer size required to fill the version info for ffxivboot.exe
    auto boot = fs::canonical(CREDENTIALS.game_dir / "boot/ffxivboot.exe");
    auto bufsize = GetFileVersionInfoSizeW(boot.c_str(), nullptr);

    // Create the buffer and get the version info
    std::unique_ptr<byte[]> buf(new byte[bufsize]);
    res = GetFileVersionInfoW(boot.c_str(), 0, bufsize, buf.get());
    if (!res) return false;

    // Get the language and codepage of the info
    struct { WORD lang, codep; } *translation;
    UINT subsize = sizeof(translation);
    res = VerQueryValueW(buf.get(), L"\\VarFileInfo\\Translation", (LPVOID *)&translation, &subsize);
    if (!res) return false;

    // Create the query string containing the hex lang/cp info
    wchar_t compparam[] = L"\\StringFileInfo\\XXXXXXXX\\CompanyName";
    wnsprintfW(compparam + 16, 9, L"%04x%04x", translation->lang, translation->codep);
    compparam[24] = L'\\';

    // Get the company name
    wchar_t *compname = nullptr;
    subsize = sizeof(compname);
    res = VerQueryValueW(buf.get(), compparam, (LPVOID *)&compname, &subsize);
    if (!res) return false;

    // If it was made by never been mad, it was replaced
    return StrCmpW(compname, L"SQUARE ENIX CO., LTD.") != 0;
}

static int stoi(std::string_view v)
{
    char *ptr = (char *)v.data() + v.length();
    return std::strtol(v.data(), &ptr, 10);
}

void LoginResponse::Parse(std::string_view str)
{
    std::string_view key, value;
    size_t next;
    for (;;)
    {
        next = str.find_first_of(',');
        if (next == std::string::npos) break;
        key = str.substr(0, next);
        str = str.substr(next + 1);

        next = str.find_first_of(',');
        if (next == std::string::npos) next = str.length();
        value = str.substr(0, next);
        str = str.substr(min(next + 1, str.length()));

        if (key == "auth")
            auth = value;
        else if (key == "sid")
            sid = value;
        else if (key == "terms")
            terms = stoi(value);
        else if (key == "region")
            region = stoi(value);
        else if (key == "etmadd")
            etmadd = stoi(value);
        else if (key == "playable")
            playable = stoi(value);
        else if (key == "ps3pkg")
            ps3pkg = stoi(value);
        else if (key == "maxex")
            maxex = stoi(value);
        else if (key == "product")
            product = stoi(value);
    }
}
