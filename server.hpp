#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <cstdint>
#include <list>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "misc.hpp"
#include "network.hpp"
#include "world.hpp"

class server {
public:
  static const int CLIENT_UPDATE_INTERVAL_MS = 50;

  server(int port)
    : game_time_ms_(0),
      io_service_(),
      endpoint_(boost::asio::ip::tcp::v4(), port),
      acceptor_(io_service_, endpoint_),
      timer_(io_service_)
  {
    start_socket_acceptor();
    start_client_updater();
    io_service_thread_ = std::thread([this](){ io_service_.run(); });
    INFO("server started");

    std::cin.get(); // exit on key pressed
    stop();
  }

private:
  void process_message(network::connection_ptr connection) {
    switch (network::get_class_id(connection->read_buffer)) {
      case network::join_request::CLASS_ID:
        process_join_request(connection);
        break;
      case command::CLASS_ID:
        process_command(connection);
        break;
    }
  }

  void process_join_request(network::connection_ptr connection) {
    network::join_request m;
    network::deserialize(m, connection->read_buffer);

    player p;
    p.set_color_AABBGGRR(m.player_color_AABBGGRR);

    if (world_.add_player(p)) {
      connection->player_id = p.get_id();

      // confirm join
      network::server_accept accept;
      accept.player_id = p.get_id();
      network::write_object(network::server_accept::CLASS_ID, accept, connection->socket);

      INFO("player joined, id: " << std::to_string(p.get_id()));
    } else {
      // reject join
      network::server_deny deny;
      deny.reason = "player limit reached";
      network::write_object(network::server_deny::CLASS_ID, deny, connection->socket);

      INFO("player rejected, reason: " << deny.reason);
    }
  }

  void process_command(network::connection_ptr connection) {
    command c;
    network::deserialize(c, connection->read_buffer);

    //
    // TODO: validate command
    //

    // simulate
    world_.run_command(c, connection->player_id);
  }

  void update_clients() {
    game_time_ms_ += CLIENT_UPDATE_INTERVAL_MS;

    if (!connections_.size())
      return;

    // build world snapshot
    std::vector<uint8_t> data;
    network::world_snapshot s(world_);
    s.server_time_ms = game_time_ms_;
    network::build_message(data, network::world_snapshot::CLASS_ID, s);

    // send to all clients
    for (auto& c : connections_)
      network::write_data(data, c->socket);
  }

  void stop() {
    INFO("stopping server");
    io_service_.stop();
    io_service_thread_.join();
  }

  void start_socket_acceptor() {
    network::connection_ptr c(new network::connection(io_service_));
    acceptor_.async_accept(c->socket,
        boost::bind(&server::handle_socket_accept, this, c,
            boost::asio::placeholders::error));
  }

  void handle_socket_accept(network::connection_ptr connection,
      const boost::system::error_code& error) {
    if (!error) {
      connections_.push_back(connection);
      start_read_header(connection);
      INFO("new client connected");
    } else {
      DEBUG("async_accept(): " << error.message());
    }

    start_socket_acceptor();
  }

  void start_client_updater() {
    timer_.expires_from_now(boost::posix_time::milliseconds(CLIENT_UPDATE_INTERVAL_MS));
    timer_.async_wait(
      boost::bind(&server::handle_client_update, this, boost::asio::placeholders::error));
  }

  void handle_client_update(const boost::system::error_code& error) {
    if (!error) {
      update_clients();
      start_client_updater();
    } else {
      DEBUG("async_wait(): " << error.message());
    }
  }

  void start_read_header(network::connection_ptr connection) {
    connection->read_buffer.clear();
    connection->read_buffer.resize(network::HEADER_SIZE);

    boost::asio::async_read(connection->socket,
        boost::asio::buffer(connection->read_buffer, network::HEADER_SIZE),
        boost::bind(&server::handle_read_header, this, connection,
            boost::asio::placeholders::error));
  }

  void handle_read_header(network::connection_ptr connection,
      const boost::system::error_code& error) {
    if (!error) {
      start_read_body(connection, network::get_body_size(connection->read_buffer));
    } else {
      close_connection(connection);
      DEBUG("async_read(): " << error.message());
    }
  }

  void start_read_body(network::connection_ptr connection, size_t read_size) {
    connection->read_buffer.clear();
    connection->read_buffer.resize(read_size);

    boost::asio::async_read(connection->socket,
        boost::asio::buffer(connection->read_buffer, read_size),
        boost::bind(&server::handle_read_body, this, connection,
            boost::asio::placeholders::error));
  }

  void handle_read_body(network::connection_ptr connection,
      const boost::system::error_code& error) {
    if (!error) {
      process_message(connection);
      start_read_header(connection);
    } else {
      close_connection(connection);
      DEBUG("async_read(): " << error.message());
    }
  }

  void close_connection(network::connection_ptr connection) {
    remove_connection_from_list(connection);
    world_.remove_player(connection->player_id);
    connection->socket.close();
    INFO("client disconnected");
  }

  void remove_connection_from_list(network::connection_ptr connection) {
    auto i = connections_.begin();
    while (i != connections_.end()) {
      if (i->get() == connection.get()) {
        connections_.erase(i);
        break;
      }
      i++;
    }
  }

  // game
  world world_;
  uint64_t game_time_ms_;

  // network
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::endpoint endpoint_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::deadline_timer timer_;
  std::list<network::connection_ptr> connections_;

  // other
  std::thread io_service_thread_;
};

#endif // SERVER_HPP_