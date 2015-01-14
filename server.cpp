#include <iostream>
#include <string>
#include "server.hpp"
#include "misc.hpp"

int main(int argc, char const *argv[]) {
  if (argc < 2 || !misc::is_number(argv[1])) {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return 1;
  }

  try {
    server(std::stoi(argv[1]));
  } catch (std::exception& e) {
    std::cerr << "exception: " << e.what() << std::endl;
    return 2;
  }

  return 0;
}