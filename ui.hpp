#ifndef UI_HPP_
#define UI_HPP_

#include <cstdint>

class ui {
public:
  virtual ~ui() { }

  virtual void poll_events() = 0;

  virtual bool check_event_quit() = 0;

  virtual bool check_event_button_released(keyboard::button button) = 0;

  virtual int get_pressed_buttons() = 0;

  virtual void get_delta_angles(float& horizontal, float& vertical) = 0;

  virtual void draw_clear() = 0;

  virtual void draw_world(world& world, uint8_t perspective_player_id, bool draw_phantoms) = 0;

  virtual void draw_update() = 0;
};

#endif // UI_HPP_
