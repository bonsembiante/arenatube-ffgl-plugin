#include "URLUtils.h"

std::string url_exec_command(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string url_get_youtube_mp4(const char* youtube_url, int max_quality) {
    char cmd[300];
    sprintf_s(cmd, 300, "C:\\ArenaTube\\youtube-dl.exe -g -f \"bestvideo[ext = webm][height <= %d]\" %s", max_quality, youtube_url);
    return url_exec_command((const char*)cmd).c_str();
}

