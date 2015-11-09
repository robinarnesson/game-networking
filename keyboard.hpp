#ifndef KEYBOARD_HPP_
#define KEYBOARD_HPP_

namespace keyboard {
  enum button {
    up         = (1 << 0),
    down       = (1 << 1),
    forward    = (1 << 2),
    backward   = (1 << 3),
    left       = (1 << 4),
    right      = (1 << 5),
    step_left  = (1 << 6),
    step_right = (1 << 7),
    quit       = (1 << 8),
    f1         = (1 << 9),
    f2         = (1 << 10),
    end        = (1 << 11)
  };
}

#endif // KEYBOARD_HPP_
