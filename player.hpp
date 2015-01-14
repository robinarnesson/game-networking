#ifndef PLAYER_HPP_
#define PLAYER_HPP_

#include <cstdint>
#include <boost/serialization/access.hpp>
#include "command.hpp"
#include "keyboard.hpp"

class player {
public:
  static const uint8_t CLASS_ID = 1;

  static const int DEFAULT_MOVE_SPEED = 200; // pixels per second
  static const int DEFAULT_TURN_SPEED = 5; // radians per second
  static const uint32_t DEFAULT_COLOR = 0xffffffff; // 0xAABBGGRR

  player()
    : id_(0),
      color_(DEFAULT_COLOR),
      x_(0.0),
      y_(0.0),
      angel_(0.0),
      last_command_id_(0)
  {
  }

  uint8_t get_id() const {
    return id_;
  }

  void set_id(uint8_t id) {
    id_ = id;
  }

  uint32_t get_color_AABBGGRR() const {
    return color_;
  }

  void set_color_AABBGGRR(uint32_t color) {
    color_ = color;
  }

  double get_x() const {
    return x_;
  }

  void set_x(double x) {
    x_ = x;
  }

  double get_y() const {
    return y_;
  }

  void set_y(double y) {
    y_ = y;
  }

  double get_angel() const {
    return angel_;
  }

  void set_angel(double angel) {
    angel_ = angel;
  }

  uint32_t get_last_command_id() const {
    return last_command_id_;
  }

  void set_last_command_id(uint32_t command_id) {
    last_command_id_ = command_id;
  }

  void run_command(const command& cmd) {
    // buttons up and down
    if ((cmd.buttons & keyboard::button::up) || (cmd.buttons & keyboard::button::down)) {
      double base_distance = cmd.duration_ms * DEFAULT_MOVE_SPEED / 1000.0;

      double add_x = base_distance *  std::cos(angel_ + M_PI / 2);
      double add_y = base_distance * -std::sin(angel_ + M_PI / 2);

      x_ += (cmd.buttons & keyboard::button::up) ? add_x : -add_x;
      y_ += (cmd.buttons & keyboard::button::up) ? add_y : -add_y;
    }

    // buttons left and right
    if ((cmd.buttons & keyboard::button::left) || (cmd.buttons & keyboard::button::right)) {
      double add_angel = cmd.duration_ms * DEFAULT_TURN_SPEED / 1000.0;

      angel_ += (cmd.buttons & keyboard::button::left) ? add_angel : -add_angel;

      if (angel_ > 360 || angel_ < -360)
        angel_ = 0.0;
    }

    // remember last command
    last_command_id_ = cmd.id;
  }

private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & id_;
    ar & color_;
    ar & x_ & y_ & angel_;
    ar & last_command_id_;
  }

  uint8_t id_;
  uint32_t color_;
  double x_, y_, angel_;
  uint32_t last_command_id_;
};

#endif // PLAYER_HPP_