#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "argus_core/msg/neural_frame.hpp"
#include "argus_wire.h"

class NeuralUdpReceiver : public rclcpp::Node
{
public:
  NeuralUdpReceiver()
  : Node("neural_udp_receiver")
  {
    bind_port_ = this->declare_parameter<int>("bind_port", ARGUS_UDP_PORT);
    output_topic_ = this->declare_parameter<std::string>(
      "output_topic", "/argus/neural_interface_bridge/neural_data");
    poll_period_us_ = this->declare_parameter<int>("poll_period_us", 1000);

    pub_ = this->create_publisher<argus_core::msg::NeuralFrame>(
      output_topic_, rclcpp::SensorDataQoS());

    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
      throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));
    }

    int reuse = 1;
    setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(bind_port_));

    if (bind(sock_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
      close(sock_);
      throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));
    }

    int flags = fcntl(sock_, F_GETFL, 0);
    fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

    timer_ = this->create_wall_timer(
      std::chrono::microseconds(poll_period_us_),
      std::bind(&NeuralUdpReceiver::drain, this));

    stats_timer_ = this->create_wall_timer(
      std::chrono::seconds(5),
      std::bind(&NeuralUdpReceiver::log_stats, this));

    RCLCPP_INFO(
      this->get_logger(),
      "neural_udp_receiver listening on UDP :%d, publishing '%s'",
      bind_port_, output_topic_.c_str());
  }

  ~NeuralUdpReceiver() override
  {
    if (sock_ >= 0) {
      close(sock_);
    }
  }

private:
  void drain()
  {
    argus_frame_packet_t pkt;
    while (true) {
      ssize_t n = recv(sock_, &pkt, sizeof(pkt), 0);
      if (n < 0) {
        return;  // EAGAIN - nothing left
      }
      if (n != static_cast<ssize_t>(sizeof(pkt))) {
        bad_size_++;
        continue;
      }
      if (pkt.magic != ARGUS_FRAME_MAGIC) {
        bad_magic_++;
        continue;
      }
      if (pkt.version != ARGUS_FRAME_VERSION) {
        bad_ver_++;
        continue;
      }

      uint16_t want = crc16_ccitt(
        reinterpret_cast<const uint8_t *>(&pkt),
        offsetof(argus_frame_packet_t, crc));
      if (want != pkt.crc) {
        bad_crc_++;
        continue;
      }

      argus_core::msg::NeuralFrame msg;
      msg.sample = pkt.sample;
      msg.t = pkt.t;
      msg.channel_count = std::min<uint16_t>(pkt.channel_count, ARGUS_MAX_CHANNELS);
      for (size_t i = 0; i < ARGUS_MAX_CHANNELS; ++i) {
        msg.channels[i] = pkt.channels[i];
      }
      pub_->publish(msg);
      good_++;
    }
  }

  void log_stats()
  {
    if (good_ == 0 && bad_size_ == 0 && bad_magic_ == 0 &&
      bad_ver_ == 0 && bad_crc_ == 0)
    {
      RCLCPP_WARN(this->get_logger(), "No frames received on :%d", bind_port_);
      return;
    }
    RCLCPP_INFO(
      this->get_logger(),
      "frames ok=%lu size=%lu magic=%lu ver=%lu crc=%lu",
      good_, bad_size_, bad_magic_, bad_ver_, bad_crc_);
  }

  int sock_{-1};
  int bind_port_{ARGUS_UDP_PORT};
  int poll_period_us_{1000};
  std::string output_topic_;

  unsigned long good_{0}, bad_size_{0}, bad_magic_{0}, bad_ver_{0}, bad_crc_{0};

  rclcpp::Publisher<argus_core::msg::NeuralFrame>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::TimerBase::SharedPtr stats_timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<NeuralUdpReceiver>());
  rclcpp::shutdown();
  return 0;
}
