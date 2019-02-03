#include "Common.h"
#include "Credentials.h"

static std::string Encrypt(const std::string &data);
static std::string Decrypt(const std::string &data);

bool Credentials::REMEMBER_USERNAME;
bool Credentials::REMEMBER_PASSWORD;
bool Credentials::USE_OTP;

std::string Credentials::USERNAME;
std::string Credentials::PASSWORD;
std::string Credentials::OTP;

Credentials CREDENTIALS;

void Credentials::Load()
{

}

void Credentials::Save()
{
}

static std::string Encrypt(const std::string &data)
{
    DATA_BLOB blob;
}

static std::string Decrypt(const std::string &data)
{

}
