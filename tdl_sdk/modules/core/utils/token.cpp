#include "token.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
namespace cvitdl {
std::set<std::pair<std::string, std::string>> get_pairs(const std::vector<std::string>& word) {
  std::set<std::pair<std::string, std::string>> pairs;
  std::string prev_char = word[0];
  for (size_t i = 1; i < word.size(); ++i) {
    pairs.insert(std::make_pair(prev_char, word[i]));
    prev_char = word[i];
  }
  return pairs;
}

struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2>& pair) const {
    return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
  }
};

std::unordered_map<std::pair<std::string, std::string>, int, pair_hash> create_pairs_map(
    const std::string& filename, int start_line, int end_line) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error opening file." << std::endl;
    return {};
  }

  std::string line;
  std::vector<std::pair<std::string, std::string>> pairs;
  int line_count = 0;

  while (std::getline(file, line) && line_count < end_line) {
    ++line_count;
    if (line_count >= start_line) {
      std::istringstream iss(line);
      std::string word1, word2;
      iss >> word1 >> word2;
      pairs.emplace_back(word1, word2);
    }
  }

  file.close();

  std::unordered_map<std::pair<std::string, std::string>, int, pair_hash> pairs_map;
  int value = 0;
  for (const auto& pair : pairs) {
    pairs_map[pair] = value;
    ++value;
  }

  return pairs_map;
}

void mergePairs(
    std::vector<std::string>& word, std::set<std::pair<std::string, std::string>>& pairs,
    const std::unordered_map<std::pair<std::string, std::string>, int, pair_hash>& bpe_ranks) {
  while (true) {
    std::pair<std::string, std::string> bigram;

    if (pairs.size() > 1) {
      bigram = *std::min_element(pairs.begin(), pairs.end(),
                                 [&bpe_ranks](const std::pair<std::string, std::string>& pair1,
                                              const std::pair<std::string, std::string>& pair2) {
                                   int pair1Value = bpe_ranks.find(pair1) != bpe_ranks.end()
                                                        ? bpe_ranks.at(pair1)
                                                        : std::numeric_limits<int>::max();
                                   int pair2Value = bpe_ranks.find(pair2) != bpe_ranks.end()
                                                        ? bpe_ranks.at(pair2)
                                                        : std::numeric_limits<int>::max();
                                   return pair1Value < pair2Value;
                                 });
    } else {
      bigram = *pairs.begin();
    }

    if (bpe_ranks.find(bigram) == bpe_ranks.end()) {
      break;
    }

    std::string first = bigram.first;
    std::string second = bigram.second;
    std::vector<std::string> new_word;
    size_t i = 0;

    while (i < word.size()) {
      try {
        auto it = std::find(word.begin() + i, word.end(), first);
        if (it == word.end()) {
          new_word.insert(new_word.end(), word.begin() + i, word.end());
          break;
        }
        size_t j = std::distance(word.begin(), it);
        new_word.insert(new_word.end(), word.begin() + i, word.begin() + j);
        i = j;
      } catch (...) {
        new_word.insert(new_word.end(), word.begin() + i, word.end());
        break;
      }

      if (word[i] == first && i < word.size() - 1 && word[i + 1] == second) {
        new_word.push_back(first + second);
        i += 2;
      } else {
        new_word.push_back(word[i]);
        i += 1;
      }
    }
    word = new_word;
    if (word.size() == 1) {
      break;
    } else {
      pairs.clear();
      auto new_pairs = get_pairs(word);
      pairs.insert(new_pairs.begin(), new_pairs.end());
    }
  }
}

void readVocab(const char* fp, std::unordered_map<std::string, uint32_t>& vocab) {
  std::ifstream file(fp);
  if (!file) {
    std::cerr << "Cannot open vocabulary file " << fp << std::endl;
    exit(EXIT_FAILURE);
  }
  std::cerr << "Loading vocabulary from " << fp << " ..." << std::endl;
  std::string line;
  while (std::getline(file, line)) {
    // Split the line by ": "
    size_t pos = line.find(": ");
    if (pos == std::string::npos) {
      std::cerr << "Invalid line format: " << line << std::endl;
      continue;
    }
    std::string word = line.substr(0, pos);
    int count = stoi(line.substr(pos + 2));

    vocab[word] = count;
  }
  std::cerr << "Read " << vocab.size() << " words from vocabulary file." << std::endl;
}

std::vector<std::string> split_word(const std::string& token) {
  std::vector<std::string> word;
  for (size_t i = 0; i < token.size() - 1; ++i) {
    word.push_back(token.substr(i, 1));
  }
  word.push_back(token.substr(token.size() - 1) + "</w>");
  return word;
}

int token_bpe(const std::string& encoderFile, const std::string& bpeFile,
              const std::string& textFile, std::vector<std::vector<int32_t>>& tokens) {
  // build vocabulary list
  std::unordered_map<std::string, uint32_t> vocab;
  readVocab(encoderFile.c_str(), vocab);

  // build bpe_ranks
  const int start_line = 2;
  const int end_line = 48895;
  std::unordered_map<std::pair<std::string, std::string>, int, pair_hash> pairs_map =
      create_pairs_map(bpeFile, start_line, end_line);

  // split sentence
  std::string special = "[@#$%^&*!]";
  std::regex pattern(
      special +
          R"(|'s|'t|'re|'ve|'m|'ll|'d|[[:alpha:]]+|[[:digit:]]|[^[:space:][:alpha:][:digit:]]+)",
      std::regex_constants::icase);

  std::vector<std::string> text;
  std::ifstream file(textFile);

  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      text.push_back(line);
    }
    file.close();
  } else {
    std::cerr << "Unable to open file." << std::endl;
    return 1;
  }

  // tokens.resize(text.size());
  // process each sentence
  for (int j = 0; j < text.size(); j++) {
    std::transform(text[j].begin(), text[j].end(), text[j].begin(),
                   ::tolower);  // convert all to lowercase
    std::vector<std::string> words;

    // std::sregex_iterator is an iterator class in C++ used for matching regular expressions in
    // strings
    std::sregex_iterator it(text[j].begin(), text[j].end(), pattern);
    std::sregex_iterator end;

    while (it != end) {
      std::smatch match = *it;
      words.push_back(match.str());
      ++it;
    }

    tokens[j].push_back(vocab["<start_of_text>"]);

    // process each word
    for (int i = 0; i < words.size(); i++) {
      std::vector<std::string> word =
          split_word(words[i]);  // split the word and add </w> after the last character
      std::set<std::pair<std::string, std::string>> pairs =
          get_pairs(word);  // return the set of all possible character pairs in a word

      if (pairs.empty()) {
        std::string token = words[i] + "</w>";
        tokens[j].push_back(vocab[token]);
        continue;
      }

      mergePairs(word, pairs, pairs_map);
      for (const auto& word_split : word) {
        tokens[j].push_back(vocab[word_split]);
      }
    }

    tokens[j].push_back(vocab["<end_of_text>"]);

    // check if the length of tokens [j] is less than 77, and add zeros if it is less than 77
    if (tokens[j].size() < 77) {
      int zerosToAdd = 77 - tokens[j].size();
      for (int k = 0; k < zerosToAdd; ++k) {
        tokens[j].push_back(0);
      }
    } else {
      printf("line %d statement is too long, shortened.\n", j);
      return 1;
    }
  }

  return 0;
}
}  // namespace cvitdl