#ifndef ARGUS_SENSORS__NEURAL_FRAME_PARSE_HPP_
#define ARGUS_SENSORS__NEURAL_FRAME_PARSE_HPP_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "argus_core/msg/neural_frame.hpp"
#include "argus_wire.h"

namespace argus_sensors
{

enum class FrameParse { ok, bad_size, bad_magic, bad_ver, bad_crc };

/* One UDP datagram -> NeuralFrame. Magic and version are checked before the
 * length, so a frame from older firmware is counted as a version mismatch
 * (which is what it is) rather than as a wrong-sized packet: every version
 * bump so far has changed the size. */
inline FrameParse parse_frame(
  const uint8_t * buf, size_t n, argus_core::msg::NeuralFrame & msg)
{
  if (n < offsetof(argus_frame_packet_t, version) + 1) {
    return FrameParse::bad_size;
  }
  uint32_t magic;
  std::memcpy(&magic, buf + offsetof(argus_frame_packet_t, magic), sizeof(magic));
  if (magic != ARGUS_FRAME_MAGIC) {
    return FrameParse::bad_magic;
  }
  if (buf[offsetof(argus_frame_packet_t, version)] != ARGUS_FRAME_VERSION) {
    return FrameParse::bad_ver;
  }
  if (n != sizeof(argus_frame_packet_t)) {
    return FrameParse::bad_size;
  }

  argus_frame_packet_t pkt;
  std::memcpy(&pkt, buf, sizeof(pkt));
  uint16_t want = crc16_ccitt(buf, offsetof(argus_frame_packet_t, crc));
  if (want != pkt.crc) {
    return FrameParse::bad_crc;
  }

  msg.sample = pkt.sample;
  msg.t = pkt.t;
  msg.channel_count = std::min<uint16_t>(pkt.channel_count, ARGUS_MAX_CHANNELS);
  for (size_t i = 0; i < ARGUS_MAX_CHANNELS; ++i) {
    msg.channels[i] = pkt.channels[i];
    msg.power[i] = pkt.power[i];
  }
  return FrameParse::ok;
}

}  // namespace argus_sensors

#endif  // ARGUS_SENSORS__NEURAL_FRAME_PARSE_HPP_
