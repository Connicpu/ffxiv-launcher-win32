#include "Common.h"
#include "Http.h"

template <typename T> using ComPtr = ATL::CComPtr<T>;

static const char *const USER_AGENT = "SQEXAuthor/2.0.0(Windows 6.2; ja-jp; 15c5fd77b2)";

static const char *const VERBS[2] =
{
    "GET",
    "POST",
};

struct UrlDecomp
{
    UrlDecomp(const std::string_view &input);

    std::string host;
    std::string path;
};

static std::string CombineHeaders(const Request &req)
{
    std::ostringstream result;
    for (auto &h : req.headers)
    {
        result << h.first;
        result << ": ";
        result << h.second;
        result << "\r\n";
    }
    return result.str();
}

HRESULT DoRequest(const Request &req, Response &response)
{
    BOOL res;
    HRESULT status = S_OK;
    DWORD position, numRead;
    HINTERNET hI = nullptr, hCon = nullptr, hReq = nullptr;
    char headerBuf[4096];
    DWORD bufsize;
    UrlDecomp url(req.url);

    if (url.host.empty())
    {
        status = E_INVALIDARG;
        goto cleanup;
    }

    hI = InternetOpenA(USER_AGENT, INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (hI == nullptr) goto fail;

    hCon = InternetConnectA(hI, url.host.c_str(), 443, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
    if (hCon == nullptr) goto fail;

    hReq = HttpOpenRequestA(hCon, VERBS[(int)req.method], url.path.c_str(), "HTTP/1.1", nullptr, nullptr, INTERNET_FLAG_SECURE, 0);
    if (hReq == nullptr) goto fail;

    if (req.headers.size() > 0)
    {
        auto allHeaders = CombineHeaders(req);
        auto res = HttpAddRequestHeadersA(hReq, allHeaders.data(), allHeaders.size(), HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
        if (!res) goto fail;
    }

    res = HttpSendRequestA(hReq, nullptr, 0, (LPVOID)req.body.data(), req.body.size());
    if (!res) goto fail;

    bufsize = sizeof(headerBuf);
    res = HttpQueryInfoA(hReq, HTTP_QUERY_STATUS_CODE, headerBuf, &bufsize, 0);
    if (!res) goto fail;
    response.status = atoi(headerBuf);

    bufsize = sizeof(headerBuf);
    res = HttpQueryInfoA(hReq, HTTP_QUERY_SET_COOKIE, headerBuf, &bufsize, 0);
    if (res)
    {
        response.cookie.assign(headerBuf, bufsize);
    }

    strcpy_s(headerBuf, "X-Patch-Unique-Id");
    bufsize = sizeof(headerBuf);
    res = HttpQueryInfoA(hReq, HTTP_QUERY_CUSTOM, headerBuf, &bufsize, 0);
    if (res)
    {
        response.patch_id.assign(headerBuf, bufsize);
    }

    static const int CHUNK_SIZE = 4096;
    position = 0;
    do
    {
        response.body.resize(position + CHUNK_SIZE);

        res = InternetReadFile(hReq, response.body.data() + position, response.body.size() - position, &numRead);
        if (!res) goto fail;

        position += numRead;
    } while (numRead > 0);

    response.body.resize(position);

cleanup:
    if (hReq) InternetCloseHandle(hReq);
    if (hReq) InternetCloseHandle(hCon);
    if (hReq) InternetCloseHandle(hI);
    return status;

fail:
    auto err = HRESULT_FROM_WIN32(GetLastError());
    goto cleanup;
}

UrlDecomp::UrlDecomp(const std::string_view & input)
{
    URL_COMPONENTSA urlcomp = { sizeof(urlcomp) };
    urlcomp.dwHostNameLength = 1;
    urlcomp.dwUrlPathLength = 1;
    urlcomp.dwExtraInfoLength = 1;

    if (InternetCrackUrlA(input.data(), input.size(), 0, &urlcomp))
    {
        host = { urlcomp.lpszHostName, urlcomp.dwHostNameLength };
        path = { urlcomp.lpszUrlPath, urlcomp.dwUrlPathLength };
        path.append(urlcomp.lpszExtraInfo, urlcomp.dwExtraInfoLength);
    }
}
