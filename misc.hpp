#ifndef MISC_HPP_
#define MISC_HPP_

#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#define DEBUG(_m_) do {                     \
  if (_DEBUG) {                             \
    std::cerr                               \
      << misc::get_datetime() << " "        \
      << __FILE__ << ":" << __LINE__ << " " \
      << __FUNCTION__ << "() | " << _m_     \
      << std::endl;                         \
  }                                         \
} while (0)

#define INFO(_m_) do {                      \
  std::cout                                 \
    << misc::get_datetime() << " | " << _m_ \
    << std::endl;                           \
  std::cout.flush();                        \
} while (0)

namespace misc {
  bool is_number(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
  }

  void sleep_ms(int sleep_time_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
  }

  uint64_t get_time_ms() {
    return std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
  }

  uint32_t get_random_color_AABBGGRR() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int> dis(0x80, 0xff); // bright colors
    return (0xff << 24) | (dis(gen) << 16) | (dis(gen) << 8) | dis(gen);
  }

  std::string get_datetime() {
    char buffer[32];
    time_t rawtime;
    time(&rawtime);
    strftime(buffer, 32, "%F %I:%M:%S", localtime(&rawtime));
    return std::string(buffer);
  }
}

#endif // MISC_HPP_