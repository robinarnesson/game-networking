#ifndef WORLD_HPP_
#define WORLD_HPP_

#include <cstdint>
#include <random>
#include <vector>
#include <boost/optional.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include "command.hpp"
#include "player.hpp"

class world {
public:
  static const uint8_t CLASS_ID = 2;

  static const int WIDTH = 5; // meter
  static const int HEIGTH = 5; // meter

  bool add_player(player& player) {
    uint8_t player_id = generate_player_id();

    if (!player_id)
      return false;

    player.set_id(player_id);
    assign_random_position(player);
    players_.push_back(player);

    return true;
  }

  void remove_player(uint8_t player_id) {
    auto i = players_.begin();

    while (i != players_.end()) {
      if (i->get_id() == player_id) {
        players_.erase(i);
        break;
      }
      i++;
    }
  }

  boost::optional<player&> get_player(uint8_t player_id) {
    boost::optional<player&> opt_player;

    for (auto& p : players_) {
      if (p.get_id() == player_id) {
        opt_player = p;
        break;
      }
    }

    return opt_player;
  }

  std::vector<player>& get_players() {
    return players_;
  }

  bool player_exists(uint8_t player_id) const {
    return !player_id_is_free(player_id);
  }

  void run_command(const command& cmd, uint8_t player_id) {
    boost::optional<player&> opt_p = get_player(player_id);

    if (opt_p)
      opt_p.get().run_command(cmd);
  }

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & players_;
  }

  bool player_id_is_free(uint8_t id) const {
    for (const player& p : players_)
      if (p.get_id() == id)
        return false;

    return true;
  }

  void assign_random_position(player& player) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    std::uniform_int_distribution<int> dis1(-WIDTH, WIDTH);
    player.set_x(dis1(gen));

    std::uniform_int_distribution<int> dis2(-HEIGTH, HEIGTH);
    player.set_z(dis2(gen));
  }

  uint8_t generate_player_id() const {
    uint8_t new_id = 1;

    while (new_id) {
      if (player_id_is_free(new_id))
        return new_id;
      new_id++;
    }

    return 0;
  }

  std::vector<player> players_;
};

#endif // WORLD_HPP_
