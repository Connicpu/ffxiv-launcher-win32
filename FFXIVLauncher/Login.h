#pragma once

enum class LoginResult
{
    Success,
    InvalidCredentials,
    TooManyLogins,
    UpdateRequired,
    CuckedBySquare,
    NetworkError,
};

LoginResult PerformLogin();
void LaunchGame();
void LaunchUpdater();
bool BootWasReplaced();
