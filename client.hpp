#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include "keyboard.hpp"
#include "misc.hpp"
#include "network.hpp"
#include "player.hpp"
#include "ui.hpp"
#include "world.hpp"

class client {
public:
  static const int MAIN_LOOP_SLEEP_MS = 10;
  static const int INTERPOLATION_TIME_MS = 150;

  client(std::string host, std::string port)
    : player_id_(0),
      game_time_ms_(0),
      io_service_(),
      socket_(io_service_),
      resolver_(io_service_),
      endpoint_iterator_(resolver_.resolve({ host, port })),
      host_(host),
      port_(port),
      exit_program_(false),
      predict_and_interpolate_(true),
      debug_(true)
  {
    start_server_connect();
    io_service_thread_ = std::thread([this](){ io_service_.run(); });
    INFO("client started");
  }

  ~client() {
    INFO("stopping client");
    io_service_.stop();
    io_service_thread_.join();
  }

  void start_ui() {
    INFO("starting ui");

    ui interface;
    if (!interface.start(std::string("client program, " + host_ + ":" + port_)))
      exit_program_ = true;

    // wait for server to accept join request and send world snapshot
    while (!game_ready() && !exit_program_) {
      interface.draw_loading();
      misc::sleep_ms(200);
    }

    main_loop(interface);

    interface.stop();
  }

private:
  void main_loop(ui& interface) {
    command command;
    int command_id = 1;
    uint64_t frame_time_ms = 0;
    uint64_t start_ms, stop_ms;

    // main loop
    while (!exit_program_) {
      start_ms = misc::get_time_ms();

      // update interface event queue
      interface.poll_events();

      // check for quit
      if (interface.check_event_quit())
        break;

      // update debug state
      if (interface.check_event_button_released(keyboard::button::n1))
        debug_ = !debug_;

      // update prediction and interpolation state
      if (interface.check_event_button_released(keyboard::button::n2))
        predict_and_interpolate_ = !predict_and_interpolate_;

      // build move command
      command.id = command_id++;
      command.buttons = interface.get_pressed_buttons();
      command.duration_ms = frame_time_ms;

      // check if quit button was pressed
      if (command.buttons & keyboard::button::escape)
        break;

      world_mutex_.lock();
      commands_mutex_.lock();

      if (command.buttons) {
        // handle client-side prediction
        if (predict_and_interpolate_) {
          // save command for next world update
          commands_.push_back(command);

          // process command (predict)
          world_.run_command(command, player_id_);
        }

        // send command to server
        network::write_object(command::CLASS_ID, command, socket_);
      }

      // handle entity interpolation
      if (predict_and_interpolate_) {
        uint64_t render_time = get_interpolation_time_point_ms();

        boost::optional<network::world_snapshot&> from, to;
        get_snapshots_adjacent_to_time_point(from, to, render_time);

        if (from && to)
          interpolate(from, to, render_time);
      }

      // draw smoothed world
      interface.draw_world(world_, false);

      // draw actual world
      if (debug_)
        interface.draw_world(world_snapshots_.back().snapshot, true);

      commands_mutex_.unlock();
      world_mutex_.unlock();

      stop_ms = misc::get_time_ms();

      frame_time_ms = MAIN_LOOP_SLEEP_MS + stop_ms - start_ms;
      game_time_ms_ += frame_time_ms;
      misc::sleep_ms(MAIN_LOOP_SLEEP_MS);
    }
  }

  void process_message() {
    switch (network::get_class_id(read_buffer_)) {
      case network::world_snapshot::CLASS_ID:
        process_world_update();
        break;
      case network::server_accept::CLASS_ID:
        process_join_accept();
        break;
      case network::server_deny::CLASS_ID:
        process_join_deny();
        break;
    }
  }

  void process_world_update() {
    world_mutex_.lock();

    // decide if to create new snapshot or overwrite last
    if (!world_snapshots_.size() || world_snapshots_.back().client_time_ms != game_time_ms_)
      world_snapshots_.emplace_back(world()); // create new

    // put snapshot last
    network::deserialize(world_snapshots_.back(), read_buffer_);
    world_snapshots_.back().client_time_ms = game_time_ms_;

    // clean up
    remove_old_world_snapshots();

    // update world
    world_ = world(world_snapshots_.back().snapshot);

    world_mutex_.unlock();

    // handle client-side prediction
    if (predict_and_interpolate_ && game_ready()) {
      world_mutex_.lock();
      commands_mutex_.lock();

      // get command id for last processed command on server
      boost::optional<player&> p = world_.get_player(player_id_);
      uint32_t last_command_id = p.get().get_last_command_id();

      // remove commands older than last command
      auto i = commands_.begin();
      while (i != commands_.end()) {
        if (i->id <= last_command_id)
          i = commands_.erase(i);
        else
          i++;
      }

      //
      // TODO: correct player position and clear commands, if needed
      //

      // run remaining commands in updated world
      for (auto& c : commands_)
        world_.run_command(c, player_id_);

      commands_mutex_.unlock();
      world_mutex_.unlock();
    }
  }

  void process_join_accept() {
    network::server_accept m;
    network::deserialize(m, read_buffer_);
    player_id_ = m.player_id;
    INFO("joined game, player_id: " << std::to_string(player_id_));
  }

  void process_join_deny() {
    network::server_deny m;
    network::deserialize(m, read_buffer_);
    INFO("join rejected, reason: " << m.reason);
    signal_exit();
  }

  void make_join_request() {
    network::join_request m;
    m.player_color_AABBGGRR = misc::get_random_color_AABBGGRR(); // random for now
    network::write_object(network::join_request::CLASS_ID, m, socket_);
    INFO("join request sent");
  }

  bool game_ready() {
    return join_request_accepted() && player_added_to_world();
  }

  bool join_request_accepted() {
    return player_id_;
  }

  bool player_added_to_world() {
    if (world_mutex_.try_lock()) {
      bool exists = world_snapshots_.size() > 0
          && world_snapshots_.front().snapshot.player_exists(player_id_);
      world_mutex_.unlock();
      return exists;
    }
    return false;
  }

  uint64_t get_interpolation_time_point_ms() {
    uint64_t time_point = 0;
    if (game_time_ms_ > INTERPOLATION_TIME_MS)
      time_point = game_time_ms_ - INTERPOLATION_TIME_MS;
    return time_point;
  }

  void get_snapshots_adjacent_to_time_point(boost::optional<network::world_snapshot&>& from,
      boost::optional<network::world_snapshot&>& to, uint64_t time_point) {
    for (auto& s : world_snapshots_) {
      if (s.client_time_ms > time_point) {
        to = s;
        break;
      }
      from = s;
    }
  }

  void interpolate(boost::optional<network::world_snapshot&>& from,
      boost::optional<network::world_snapshot&>& to, uint64_t time_point) {
    // get interpolation time fraction
    double fraction = get_time_fraction(from.get().client_time_ms,
        to.get().client_time_ms, time_point);

    for (player& p_from : from.get().snapshot.get_players()) {
      for (player& p_to : to.get().snapshot.get_players()) {
        // player exists in both snapshots and is not client player, interpolate
        if (p_from.get_id() == p_to.get_id() && p_to.get_id() != player_id_) {
          // get player from world that will be rendered
          boost::optional<player&> p_real = world_.get_player(p_to.get_id());
          if (p_real) {
            // calc interpolated player values
            double new_x = (p_to.get_x() - p_from.get_x()) * fraction + p_from.get_x();
            double new_y = (p_to.get_y() - p_from.get_y()) * fraction + p_from.get_y();
            double new_angel = (p_to.get_angel() - p_from.get_angel())
                * fraction + p_from.get_angel();

            // assign values to player
            p_real.get().set_x(new_x);
            p_real.get().set_y(new_y);
            p_real.get().set_angel(new_angel);
          }
        }
      }
    }
  }

  double get_time_fraction(uint64_t start_ms, uint64_t stop_ms, uint64_t between_ms) {
    return static_cast<double>(between_ms - start_ms) / static_cast<double>(stop_ms - start_ms);
  }

  void remove_old_world_snapshots() {
    bool remove_next = false;
    uint64_t render_time = get_interpolation_time_point_ms();
    auto i = world_snapshots_.end();
    while (i != world_snapshots_.begin()) {
      if (remove_next)
        i = world_snapshots_.erase(i);
      else if (i->client_time_ms <= render_time && i->client_time_ms > INTERPOLATION_TIME_MS)
        remove_next = true;
      i--;
    }
  }

  void start_server_connect() {
    boost::asio::async_connect(socket_, endpoint_iterator_,
        boost::bind(&client::handle_server_connect, this,boost::asio::placeholders::error));
  }

  void handle_server_connect(const boost::system::error_code& error) {
    if (!error) {
      INFO("connected to server");
      make_join_request();
      start_read_header();
    } else {
      signal_exit();
      DEBUG("async_connect(): " << error.message());
    }
  }

  void start_read_header() {
    read_buffer_.clear();
    read_buffer_.resize(network::HEADER_SIZE);

    boost::asio::async_read(socket_,
        boost::asio::buffer(read_buffer_, network::HEADER_SIZE),
        boost::bind(&client::handle_read_header, this, boost::asio::placeholders::error));
  }

  void handle_read_header(const boost::system::error_code& error) {
    if (!error) {
      start_read_body(network::get_body_size(read_buffer_));
    } else {
      signal_exit();
      DEBUG("async_read(): " << error.message());
    }
  }

  void start_read_body(size_t body_size) {
    read_buffer_.clear();
    read_buffer_.resize(body_size);

    boost::asio::async_read(socket_,
        boost::asio::buffer(read_buffer_, body_size),
        boost::bind(&client::handle_read_body, this, boost::asio::placeholders::error));
  }

  void handle_read_body(const boost::system::error_code& error) {
    if (!error) {
      process_message();
      start_read_header();
    } else {
      signal_exit();
      DEBUG("async_read(): " << error.message());
    }
  }

  void signal_exit() {
    socket_.close();
    exit_program_ = true;
  }

  // game
  world world_;
  std::list<network::world_snapshot> world_snapshots_;
  std::mutex world_mutex_;
  std::list<command> commands_;
  std::mutex commands_mutex_;
  std::atomic<uint8_t> player_id_;
  std::atomic<uint64_t> game_time_ms_;

  // network
  std::vector<uint8_t> read_buffer_;
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::ip::tcp::resolver resolver_;
  boost::asio::ip::tcp::resolver::iterator endpoint_iterator_;
  std::thread io_service_thread_;
  std::string host_;
  std::string port_;

  // other
  std::atomic<bool> exit_program_;
  std::atomic<bool> predict_and_interpolate_;
  std::atomic<bool> debug_;
};

#endif // CLIENT_HPP_