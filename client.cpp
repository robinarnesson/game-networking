#include <iostream>
#include "client.hpp"
#include "misc.hpp"

int main(int argc, char const *argv[]) {
  if (argc < 3 || !misc::is_number(argv[2])) {
    std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl << std::endl;
    std::cerr << "Controls: " << std::endl;
    std::cerr << "  Move around with the arrow keys." << std::endl;
    std::cerr << "  Toggle debug mode with key '1'." << std::endl;
    std::cerr << "  Toggle prediction and interpolation with key '2'." << std::endl;
    return 1;
  }

  try {
    client(argv[1], argv[2]).start_ui();
  } catch (std::exception& e) {
    std::cerr << "exception: " << e.what() << std::endl;
    return 2;
  }

  return 0;
}
