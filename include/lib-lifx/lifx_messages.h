/////
// lifx_messages.h
//! @file Defines LIFX headers & messages
/////

#pragma once

#include <stdint.h>

namespace lifx
{

#pragma pack(push, 1)
  typedef struct
  {
    // Frame - 64
    uint16_t  size;
    uint16_t  protocol : 12;
    uint8_t   addressable : 1;
    uint8_t   tagged : 1;
    uint8_t   origin : 2;
    uint32_t  source;
    // Frame Address - 128
    uint8_t   target[8];
    uint8_t   site[6];
    uint8_t   ack_required : 1;
    uint8_t   res_required : 1;
    uint8_t:6;
    uint8_t   sequence;
    // Protocol Header - 96
    uint64_t  at_time;
    uint16_t  type;
    uint16_t:16;
  } Header;

#ifdef _WIN32
  typedef struct
  {
    uint64_t frame;
    uint64_t target;
    uint64_t frame_address;
    uint64_t time;
    uint16_t type;
    uint64_t:64;
  } NetworkHeader;
#else
  typedef Header NetworkHeader;
#endif

  typedef struct
  {
    uint16_t  hue;
    uint16_t  saturation;
    uint16_t  brightness;
    uint16_t  kelvin;
  } HSBK;
#pragma pack(pop)


namespace message
{
#pragma pack(push, 1)

  namespace device
  {
    struct GetService
    {
      static constexpr uint16_t type = 2;
      static constexpr bool has_response = true;
    };

    struct StateService
    {
      static constexpr uint16_t type = 3;
      static constexpr bool has_response = false;
      uint8_t service;
      uint32_t port;
    };

    struct GetHostInfo
    {
      static constexpr uint16_t type = 12;
      static constexpr bool has_response = true;
    };

    struct StateHostInfo
    {
      static constexpr uint16_t type = 13;
      static constexpr bool has_response = false;
      float signal;
      uint32_t tx;
      uint32_t rx;
      int16_t:16;
    };

    struct GetHostFirmware
    {
      static constexpr uint16_t type = 14;
      static constexpr bool has_response = true;
    };

    struct StateHostFirmware
    {
      static constexpr uint16_t type = 15;
      static constexpr bool has_response = false;
      uint64_t build;
      uint64_t:64;
      uint32_t version;
    };

    struct GetWifiInfo
    {
      static constexpr uint16_t type = 16;
      static constexpr bool has_response = true;
    };

    struct StateWifiInfo
    {
      static constexpr uint16_t type = 17;
      static constexpr bool has_response = false;
      float signal;
      uint32_t tx;
      uint32_t rx;
      int16_t:16;
    };

    struct GetWifiFirmware
    {
      static constexpr uint16_t type = 18;
      static constexpr bool has_response = true;
    };

    struct StateWifiFirmware
    {
      static constexpr uint16_t type = 19;
      static constexpr bool has_response = false;
      uint64_t build;
      uint64_t:64;
      uint32_t version;
    };

    struct GetPower
    {
      static constexpr uint16_t type = 20;
      static constexpr bool has_response = true;
    };

    struct SetPower
    {
      static constexpr uint16_t type = 21;
      static constexpr bool has_response = false;
      uint16_t level;
    };

    struct StatePower
    {
      static constexpr uint16_t type = 22;
      static constexpr bool has_response = false;
      uint16_t level;
    };

    struct GetLabel
    {
      static constexpr uint16_t type = 23;
      static constexpr bool has_response = true;
    };

    struct SetLabel
    {
      static constexpr uint16_t type = 24;
      static constexpr bool has_response = false;
      char label[32];
    };

    struct StateLabel
    {
      static constexpr uint16_t type = 25;
      static constexpr bool has_response = false;
      char label[32];
    };

    struct GetVersion
    {
      static constexpr uint16_t type = 32;
      static constexpr bool has_response = true;
    };

    struct StateVersion
    {
      static constexpr uint16_t type = 33;
      static constexpr bool has_response = false;
      uint32_t vendor;
      uint32_t product;
      uint32_t version;
    };

    struct GetInfo
    {
      static constexpr uint16_t type = 34;
      static constexpr bool has_response = true;
    };

    struct StateInfo
    {
      static constexpr uint16_t type = 35;
      static constexpr bool has_response = false;
      uint64_t time;
      uint64_t uptime;
      uint64_t downtime;
    };

    struct Acknowledgement
    {
      static constexpr uint16_t type = 45;
      static constexpr bool has_response = false;
    };

    struct GetLocation
    {
      static constexpr uint16_t type = 48;
      static constexpr bool has_response = true;
    };

    struct StateLocation
    {
      static constexpr uint16_t type = 50;
      static constexpr bool has_response = false;
      uint8_t location[16];
      char label[32];
      uint64_t updated_at;
    };

    struct GetGroup
    {
      static constexpr uint16_t type = 51;
      static constexpr bool has_response = true;
    };

    struct StateGroup
    {
      static constexpr uint16_t type = 53;
      static constexpr bool has_response = false;
      uint8_t group[16];
      char label[32];
      uint64_t updated_at;
    };

    struct EchoRequest
    {
      static constexpr uint16_t type = 58;
      static constexpr bool has_response = true;
      uint64_t payload;
    };

    struct EchoResponse
    {
      static constexpr uint16_t type = 59;
      static constexpr bool has_response = false;
      uint64_t payload;
    };
  } // namespace device

  namespace light
  {
    struct Get
    {
      static constexpr uint16_t type = 101;
      static constexpr bool has_response = true;
    };

    struct SetColor
    {
      static constexpr uint16_t type = 102;
      static constexpr bool has_response = false;
      uint8_t:8;
      HSBK color;
      uint32_t duration;
    };

    struct State
    {
      static constexpr uint16_t type = 107;
      static constexpr bool has_response = false;
      HSBK color;
      int16_t:16;
      uint16_t power;
      char label[32];
      uint64_t:64;
    };

    struct GetPower
    {
      static constexpr uint16_t type = 116;
      static constexpr bool has_response = true;
    };

    struct SetPower
    {
      static constexpr uint16_t type = 117;
      static constexpr bool has_response = false;
      uint16_t level;
      uint32_t duration;
    };

    struct StatePower
    {
      static constexpr uint16_t type = 118;
      static constexpr bool has_response = false;
      uint16_t level;
    };
  } // namespace light

#pragma pack(pop)
} // namespace message

} // namespace lifx
