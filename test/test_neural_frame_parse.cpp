#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

#include "argus_sensors/neural_frame_parse.hpp"

using argus_sensors::FrameParse;
using argus_sensors::parse_frame;

namespace
{

/* A well-formed current-version frame, as the firmware would send it. */
std::vector<uint8_t> make_frame()
{
  argus_frame_packet_t pkt;
  std::memset(&pkt, 0, sizeof(pkt));
  pkt.magic = ARGUS_FRAME_MAGIC;
  pkt.sample = 1234;
  pkt.t = 61.7f;
  pkt.version = ARGUS_FRAME_VERSION;
  pkt.channel_count = ARGUS_MAX_CHANNELS;
  for (size_t i = 0; i < ARGUS_MAX_CHANNELS; ++i) {
    pkt.channels[i] = static_cast<uint16_t>(i % 7);
    pkt.power[i] = 100000u * static_cast<uint32_t>(i) + 3914u;
  }
  pkt.crc = crc16_ccitt(
    reinterpret_cast<const uint8_t *>(&pkt), offsetof(argus_frame_packet_t, crc));
  std::vector<uint8_t> buf(sizeof(pkt));
  std::memcpy(buf.data(), &pkt, sizeof(pkt));
  return buf;
}

/* A version-2 frame byte for byte: the v3 header, counts, then the CRC,
 * with no power block -- 210 bytes. */
std::vector<uint8_t> make_v2_frame()
{
  const size_t head = offsetof(argus_frame_packet_t, power);
  std::vector<uint8_t> v3 = make_frame();
  std::vector<uint8_t> buf(v3.begin(), v3.begin() + head);
  buf[offsetof(argus_frame_packet_t, version)] = 2;
  uint16_t crc = crc16_ccitt(buf.data(), buf.size());
  buf.push_back(static_cast<uint8_t>(crc & 0xFF));
  buf.push_back(static_cast<uint8_t>(crc >> 8));
  return buf;
}

}  // namespace

TEST(NeuralFrameParse, AcceptsCurrentVersionAndCopiesPower)
{
  auto buf = make_frame();
  argus_core::msg::NeuralFrame msg;
  ASSERT_EQ(parse_frame(buf.data(), buf.size(), msg), FrameParse::ok);
  EXPECT_EQ(msg.sample, 1234u);
  EXPECT_FLOAT_EQ(msg.t, 61.7f);
  EXPECT_EQ(msg.channel_count, ARGUS_MAX_CHANNELS);
  for (size_t i = 0; i < ARGUS_MAX_CHANNELS; ++i) {
    EXPECT_EQ(msg.channels[i], i % 7) << "channel " << i;
    EXPECT_EQ(msg.power[i], 100000u * i + 3914u) << "channel " << i;
  }
}

TEST(NeuralFrameParse, RejectsVersion2AsBadVersion)
{
  auto buf = make_v2_frame();
  ASSERT_EQ(buf.size(), 210u);
  argus_core::msg::NeuralFrame msg;
  EXPECT_EQ(parse_frame(buf.data(), buf.size(), msg), FrameParse::bad_ver);
}

TEST(NeuralFrameParse, RejectsVersion2EvenAtCurrentSize)
{
  auto buf = make_frame();
  buf[offsetof(argus_frame_packet_t, version)] = 2;
  argus_core::msg::NeuralFrame msg;
  EXPECT_EQ(parse_frame(buf.data(), buf.size(), msg), FrameParse::bad_ver);
}

TEST(NeuralFrameParse, RejectsBadMagic)
{
  auto buf = make_frame();
  buf[0] ^= 0xFF;
  argus_core::msg::NeuralFrame msg;
  EXPECT_EQ(parse_frame(buf.data(), buf.size(), msg), FrameParse::bad_magic);
}

TEST(NeuralFrameParse, RejectsWrongLength)
{
  auto buf = make_frame();
  argus_core::msg::NeuralFrame msg;
  EXPECT_EQ(parse_frame(buf.data(), buf.size() - 1, msg), FrameParse::bad_size);
  buf.push_back(0);
  EXPECT_EQ(parse_frame(buf.data(), buf.size(), msg), FrameParse::bad_size);
  EXPECT_EQ(parse_frame(buf.data(), 4, msg), FrameParse::bad_size);
}

TEST(NeuralFrameParse, RejectsCorruptPower)
{
  auto buf = make_frame();
  buf[offsetof(argus_frame_packet_t, power) + 17] ^= 0x01;
  argus_core::msg::NeuralFrame msg;
  EXPECT_EQ(parse_frame(buf.data(), buf.size(), msg), FrameParse::bad_crc);
}
