/////
// lifx_messages.cpp
//! @file Expose LIFX message static types
/////

#include <lib-lifx/lifx.h>

#include <stdint.h>

namespace lifx
{

namespace message
{

  namespace device
  {
    constexpr uint16_t GetService::type;
    constexpr bool GetService::has_response;

    constexpr uint16_t StateService::type;
    constexpr bool StateService::has_response;

    constexpr uint16_t GetHostInfo::type;
    constexpr bool GetHostInfo::has_response;

    constexpr uint16_t StateHostInfo::type;
    constexpr bool StateHostInfo::has_response;

    constexpr uint16_t GetHostFirmware::type;
    constexpr bool GetHostFirmware::has_response;

    constexpr uint16_t StateHostFirmware::type;
    constexpr bool StateHostFirmware::has_response;

    constexpr uint16_t GetWifiInfo::type;
    constexpr bool GetWifiInfo::has_response;

    constexpr uint16_t StateWifiInfo::type;
    constexpr bool StateWifiInfo::has_response;

    constexpr uint16_t GetWifiFirmware::type;
    constexpr bool GetWifiFirmware::has_response;

    constexpr uint16_t StateWifiFirmware::type;
    constexpr bool StateWifiFirmware::has_response;

    constexpr uint16_t GetPower::type;
    constexpr bool GetPower::has_response;

    constexpr uint16_t SetPower::type;
    constexpr bool SetPower::has_response;

    constexpr uint16_t StatePower::type;
    constexpr bool StatePower::has_response;

    constexpr uint16_t GetLabel::type;
    constexpr bool GetLabel::has_response;

    constexpr uint16_t SetLabel::type;
    constexpr bool SetLabel::has_response;

    constexpr uint16_t StateLabel::type;
    constexpr bool StateLabel::has_response;

    constexpr uint16_t GetVersion::type;
    constexpr bool GetVersion::has_response;

    constexpr uint16_t StateVersion::type;
    constexpr bool StateVersion::has_response;

    constexpr uint16_t GetInfo::type;
    constexpr bool GetInfo::has_response;

    constexpr uint16_t StateInfo::type;
    constexpr bool StateInfo::has_response;

    constexpr uint16_t Acknowledgement::type;
    constexpr bool Acknowledgement::has_response;

    constexpr uint16_t GetLocation::type;
    constexpr bool GetLocation::has_response;

    constexpr uint16_t StateLocation::type;
    constexpr bool StateLocation::has_response;

    constexpr uint16_t GetGroup::type;
    constexpr bool GetGroup::has_response;

    constexpr uint16_t StateGroup::type;
    constexpr bool StateGroup::has_response;

    constexpr uint16_t EchoRequest::type;
    constexpr bool EchoRequest::has_response;

    constexpr uint16_t EchoResponse::type;
    constexpr bool EchoResponse::has_response;
  } // namespace device

  namespace light
  {
    constexpr uint16_t Get::type;
    constexpr bool Get::has_response;

    constexpr uint16_t SetColor::type;
    constexpr bool SetColor::has_response;

    constexpr uint16_t State::type;
    constexpr bool State::has_response;

    constexpr uint16_t GetPower::type;
    constexpr bool GetPower::has_response;

    constexpr uint16_t SetPower::type;
    constexpr bool SetPower::has_response;

    constexpr uint16_t StatePower::type;
    constexpr bool StatePower::has_response;
  } // namespace light

} // namespace message

} // namespace lifx
