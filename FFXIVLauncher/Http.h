#pragma once

#include "Win.h"
#include <string>
#include <string_view>
#include <vector>
#include <stdint.h>
#include <atlbase.h>

enum class Method;
struct Request;
struct Response;

HRESULT DoRequest(const Request &request, Response &response);

enum class Method
{
    GET,
    POST,
};

struct Request
{
    Method method;
    std::string url;

    std::vector<std::pair<const char *, std::string>> headers;
    // Should be empty if method is GET
    std::vector<uint8_t> body;
};

struct Response
{
    int status;
    std::string cookie;
    std::string patch_id;
    std::vector<uint8_t> body;
};
