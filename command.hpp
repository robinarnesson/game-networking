#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include <cstdint>
#include <boost/serialization/access.hpp>

class command {
public:
  static const uint8_t CLASS_ID = 7;

  uint32_t id;
  uint32_t buttons;     // bit field with pressed keyboard buttons
  uint16_t duration_ms; // how long the buttons were pressed

private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & id;
    ar & buttons;
    ar & duration_ms;
  }
};

#endif // COMMAND_HPP_
