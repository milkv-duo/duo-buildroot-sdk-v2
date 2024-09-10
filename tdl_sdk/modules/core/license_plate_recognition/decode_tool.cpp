#include "decode_tool.hpp"

#include <iostream>
#include <vector>

#define CODE_LENGTH_TW 18
#define CODE_LENGTH_CN 24
#define CHARS_NUM_TW 36
#define CHARS_NUM_CN 72

char const *CHAR_LIST_TW = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ-_";

// clang-format off
char const *CHAR_LIST_CN[CHARS_NUM_CN] = {
  "_",
  "<Anhui>","<Shanghai>","<Tianjin>","<Chongqing>","<Hebei>",
  "<Shanxi>","<InnerMongolia>","<Liaoning>","<Jilin>","<Heilongjiang>",
  "<Jiangsu>","<Zhejiang>","<Beijing>","<Fujian>","<Jiangxi>",
  "<Shandong>","<Henan>","<Hubei>","<Hunan>","<Guangdong>",
  "<Guangxi>","<Hainan>","<Sichuan>","<Guizhou>","<Yunnan>",
  "<Tibet>","<Shaanxi>","<Gansu>","<Qinghai>","<Ningxia>",
  "<Xinjiang>",
  "<Gang>","<Ao>","<Shi>","<Ling>","<Xue>","<Jing>",
  "0","1","2","3","4","5","6","7","8","9",
  "A","B","C","D","E","F","G","H","J","K",
  "L","M","N","P","Q","R","S","T","U","V",
  "W","X","Y","Z"};
// clang-format on

namespace LPR {

bool greedy_decode(float *y, std::string &id_number, LP_FORMAT format) {
  int chars_num;
  switch (format) {
    case TAIWAN:
      chars_num = CHARS_NUM_TW;
      break;
    case CHINA:
      chars_num = CHARS_NUM_CN;
      break;
    default:
      return false;
  }
  int code_length = (format == TAIWAN) ? CODE_LENGTH_TW : CODE_LENGTH_CN;
  int *code = new int[code_length];
  for (int i = 0; i < code_length; i++) {
    float max_value = y[i];
    code[i] = 0;
    for (int j = 1; j < chars_num; j++) {
      if (y[j * code_length + i] > max_value) {
        max_value = y[j * code_length + i];
        code[i] = j;
      }
    }
  }

  switch (format) {
    case TAIWAN:
      id_number = decode_tw(code);
      break;
    case CHINA:
      id_number = decode_cn(code);
      break;
    default:
      return false;
  }

  delete[] code;
  return true;
}

std::string decode_tw(int *code) {
  std::vector<int> de_code;
  int previous = -1;
  for (int i = 0; i < CODE_LENGTH_TW; i++) {
    if (code[i] != previous) {
      de_code.push_back(code[i]);
      previous = code[i];
    }
  }
  std::string id_number("");
  for (size_t i = 0; i < de_code.size(); i++) {
    if (de_code[i] != CHARS_NUM_TW - 1) {
      id_number.append(1, CHAR_LIST_TW[de_code[i]]);
    }
  }

  return id_number;
}

std::string decode_cn(int *code) {
  std::vector<int> de_code;
  int previous = -1;
  for (int i = 0; i < CODE_LENGTH_CN; i++) {
    if (code[i] != previous) {
      de_code.push_back(code[i]);
      previous = code[i];
    }
  }
  std::string id_number("");
  for (size_t i = 0; i < de_code.size(); i++) {
    if (de_code[i] != 0) {
      id_number += CHAR_LIST_CN[de_code[i]];
    }
  }

  return id_number;
}

}  // namespace LPR