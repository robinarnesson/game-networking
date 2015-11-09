#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include <cstdint>
#include <boost/serialization/access.hpp>

class command {
public:
  static const uint8_t CLASS_ID = 7;

  int id;
  int buttons; // keyboard buttons pressed (keyboard::button)
  float horz_delta_angel;
  float vert_delta_angel;
  int duration_ms; // frame time

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & id;
    ar & buttons;
    ar & horz_delta_angel;
    ar & vert_delta_angel;
    ar & duration_ms;
  }
};

#endif // COMMAND_HPP_
