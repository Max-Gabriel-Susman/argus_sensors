#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "rclcpp/rclcpp.hpp"
#include "argus_core/msg/neural_frame.hpp"
#include "argus_core/argus_wire.h"

class NeuralUdpReceiver : public rclpp::Node
{
public:
    NeuralUdpReceiver() : Node("neural_udp_receiver")
    {

    }
private:
    void drain()
    {
        argus_frame_packet_t pkt;
        while (true) {
            ssize_t n = recv(sock_, &pkt, sizeof(pkt), 0);
            if (n < 0) {
                return; // EAGAIN - nothing left
            }
            if (n != sizeof(pkt)) {
                bad_size_++;
                continue;
            }

            if (pkt.magic != FRAME_MAGIC) {
                bad_magic_++; 
                continue;
            }
            if (pkt.version != FRAME_VERSION) {
                bad_ver_++; 
                continue;
            }

            uint16_t want = argus_crc16(
                reinterpret_cast<const uint8_t*>(&pkt),
                offsetof(argus_frame_packet_t, crc));
            if (want != pkt.crc) {
                bad_crc_++;
                continue;
            }

            argus_core::msg::NeuralFrame msg;
            msg.sample = pkt.sample;
            msg.t = pkt.t;
            msg.channel_count = std::min<uint16_t>(
                pkt.channel_count, MAX_CHANNELS);
            for (size_t i = 0; i < MAX_CHANNELS; ++i) {
                msg.channels[i] = pkt.channels[i];
            }
            pub_->publish(msg);
        }
    }
};