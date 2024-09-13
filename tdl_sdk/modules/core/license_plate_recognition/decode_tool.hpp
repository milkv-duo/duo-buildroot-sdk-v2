#pragma once
#include <string>

enum LP_FORMAT { TAIWAN = 0, CHINA };

namespace LPR {
bool greedy_decode(float *y, std::string &id_number, LP_FORMAT format = TAIWAN);

std::string decode_tw(int *code);
std::string decode_cn(int *code);
}  // namespace LPR
