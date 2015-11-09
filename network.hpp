#ifndef NETWORK_HPP_
#define NETWORK_HPP_

#include <cstdint>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include "world.hpp"

namespace network {
  const int HEADER_SIZE = 8; // bytes
  const int CLASS_ID_SIZE = 3; // bytes

  ///////////////////////////////////////////////////////////////////

  class world_snapshot {
  public:
    static const uint8_t CLASS_ID = 10;

    world snapshot;
    uint64_t server_time_ms;
    uint64_t client_time_ms;

    world_snapshot(world world)
      : snapshot(world),
        server_time_ms(0),
        client_time_ms(0)
    {
    }

  private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
      ar & snapshot;
      ar & server_time_ms;
      ar & client_time_ms;
    }
  };

  ///////////////////////////////////////////////////////////////////

  class connection {
  public:
    boost::asio::ip::tcp::socket socket;
    std::vector<uint8_t> read_buffer;
    uint8_t player_id;

    connection(boost::asio::io_service& io_service)
      : socket(io_service),
        player_id(0)
    {
    }
  };

  using connection_ptr = std::shared_ptr<connection>;

  ///////////////////////////////////////////////////////////////////

  class join_request {
  public:
    static const uint8_t CLASS_ID = 4;

    uint32_t player_color_AABBGGRR;

  private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
      ar & player_color_AABBGGRR;
    }
  };

  ///////////////////////////////////////////////////////////////////

  class server_accept {
  public:
    static const uint8_t CLASS_ID = 5;

    uint8_t player_id;

  private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
      ar & player_id;
    }
  };

  ///////////////////////////////////////////////////////////////////

  class server_deny {
  public:
    static const uint8_t CLASS_ID = 6;

    std::string reason;

  private:
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
      ar & reason;
    }
  };

  ///////////////////////////////////////////////////////////////////

  template <typename T>
  void deserialize(T& object, const std::vector<uint8_t>& body_data) {
    std::string archive_data(body_data.begin() + CLASS_ID_SIZE, body_data.end());
    std::istringstream archive_stream(archive_data);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> object;
  }

  template <typename T>
  void build_message(std::vector<uint8_t>& message, uint8_t class_id, const T& object) {
    // get object data (serialize)
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << object;
    const std::string& object_str = archive_stream.str();

    // set object data size
    std::ostringstream header_stream;
    header_stream << std::setw(HEADER_SIZE)
        << std::to_string(object_str.size() + CLASS_ID_SIZE);
    const std::string& header_str = header_stream.str();

    // set class id
    std::ostringstream class_id_stream;
    class_id_stream << std::setw(CLASS_ID_SIZE) << std::to_string(class_id);
    const std::string& class_id_str = class_id_stream.str();

    // set message header
    message.insert(message.end(), header_str.begin(), header_str.end());

    // set message body
    message.insert(message.end(), class_id_str.begin(), class_id_str.end());
    message.insert(message.end(), object_str.begin(), object_str.end());
  }

  void write_data(const std::vector<uint8_t>& data, boost::asio::ip::tcp::socket& socket) {
    boost::asio::async_write(socket,
        boost::asio::buffer(data, data.size()),
        [](boost::system::error_code, std::size_t){ /* do nothing */ });
  }

  template <typename T>
  void write_object(uint8_t class_id, const T& object, boost::asio::ip::tcp::socket& socket) {
    std::vector<uint8_t> data;
    build_message(data, class_id, object);
    write_data(data, socket);
  }

  int get_number(const std::vector<uint8_t>& data, size_t start, size_t size) {
    try {
      return std::stoi(std::string(data.begin() + start, data.begin() + size));
    } catch (std::exception& e) {
      return 0;
    }
  }

  uint8_t get_class_id(const std::vector<uint8_t>& body_data) {
    return get_number(body_data, 0, CLASS_ID_SIZE);
  }

  int get_body_size(const std::vector<uint8_t>& header_data) {
    return get_number(header_data, 0, HEADER_SIZE);
  }
}

#endif // NETWORK_HPP_
