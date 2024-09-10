#pragma once
#include <string>
#include <vector>

namespace cvitdl {
int token_bpe(const std::string& encoderFile, const std::string& bpeFile,
              const std::string& textFile, std::vector<std::vector<int32_t>>& tokens);
}  // namespace cvitdl