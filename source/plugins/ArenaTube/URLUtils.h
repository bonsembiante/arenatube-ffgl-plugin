#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

std::string url_get_youtube_mp4(const char* youtube_url, int max_quality = 720);
std::string url_exec_command(const char* cmd);