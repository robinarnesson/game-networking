#ifndef PLAYER_HPP_
#define PLAYER_HPP_

#include <cstdint>
#include <boost/serialization/access.hpp>
#include "command.hpp"
#include "keyboard.hpp"

class player {
public:
  static const uint8_t CLASS_ID = 1;

  static const int DEFAULT_MOVE_SPEED = 2; // m/s
  static const int DEFAULT_TURN_SPEED = 3; // rad/s

  player()
    : id_(0),
      color_(0xffffffff), // 0xAABBGGRR
      x_(0.0),
      y_(0.0),
      z_(0.0),
      horz_angel_(0.0),
      vert_angel_(0.0),
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

  float get_x() const {
    return x_;
  }

  void set_x(float x) {
    x_ = x;
  }

  float get_y() const {
    return y_;
  }

  void set_y(float y) {
    y_ = y;
  }

  float get_z() const {
    return z_;
  }

  void set_z(float z) {
    z_ = z;
  }

  float get_horz_angel() const {
    return horz_angel_;
  }

  void set_horz_angel(float angel) {
    horz_angel_ = angel;
  }

  float get_vert_angel() const {
    return vert_angel_;
  }

  void set_vert_angel(float angel) {
    vert_angel_ = angel;
  }

  int get_last_command_id() const {
    return last_command_id_;
  }

  void set_last_command_id(int command_id) {
    last_command_id_ = command_id;
  }

  void run_command(const command& cmd) {
    // remember this command
    last_command_id_ = cmd.id;

    // update angels
    horz_angel_ += cmd.horz_delta_angel;
    vert_angel_ += cmd.vert_delta_angel;

    // handle action turn left and turn right
    if ((cmd.buttons & keyboard::button::left) || (cmd.buttons & keyboard::button::right)) {
      float add_angel = cmd.duration_ms * DEFAULT_TURN_SPEED / 1000.0;

      horz_angel_ += (cmd.buttons & keyboard::button::left) ? add_angel : -add_angel;
    }

    // if no more actions except turn left and right then return
    if (cmd.buttons == (keyboard::button::left | keyboard::button::right))
      return;

    float add_x, add_z;
    float base_distance = cmd.duration_ms * DEFAULT_MOVE_SPEED / 1000.0;

    // handle action move forward and move backward
    if ((cmd.buttons & keyboard::button::forward) || (cmd.buttons & keyboard::button::backward)) {
      add_x = base_distance *  std::cos(horz_angel_);
      add_z = base_distance * -std::sin(horz_angel_);

      x_ += (cmd.buttons & keyboard::button::forward) ? add_x : -add_x;
      z_ += (cmd.buttons & keyboard::button::forward) ? add_z : -add_z;
    }

    // handle action strafe left and strafe right
    if ((cmd.buttons & keyboard::button::step_left) ||
        (cmd.buttons & keyboard::button::step_right)) {
      add_x = base_distance *  std::cos(horz_angel_ - M_PI / 2);
      add_z = base_distance * -std::sin(horz_angel_ - M_PI / 2);

      x_ += (cmd.buttons & keyboard::button::step_right) ? add_x : -add_x;
      z_ += (cmd.buttons & keyboard::button::step_right) ? add_z : -add_z;
    }

    // handle action move up
    if ((cmd.buttons & keyboard::button::up))
      y_ += base_distance;

    // handle action move down
    if ((cmd.buttons & keyboard::button::down))
      y_ -= base_distance;
  }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & id_;
    ar & color_;
    ar & x_ & y_ & z_;
    ar & horz_angel_ & vert_angel_;
    ar & last_command_id_;
  }

  uint8_t id_;
  uint32_t color_;
  float x_, y_, z_;
  float horz_angel_, vert_angel_;
  int last_command_id_;
};

#endif // PLAYER_HPP_
