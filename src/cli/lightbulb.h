/////
// lightbulb.h
//! @file Lightbulb structure definition
/////

#include <lib-lifx/lifx.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <string>

struct Lightbulb
{
  std::string label;
  std::array<uint8_t, 8> mac_address;
  bool power;
  lifx::HSBK color;

  struct {
    std::string label;
    uint64_t updated_at;
  } group;

  struct {
    uint32_t vendor;
    uint32_t product;
    uint32_t version;
  } version;

  struct {
    std::string label;
    uint64_t updated_at;
  } location;

  operator std::string() const
  {
    std::stringstream ss;
    ss <<
      "Name: " <<
      label <<
      std::endl;

    if (!group.label.empty())
    {
      ss <<
        "Group: " <<
        group.label <<
        std::endl;
    }

    if (!location.label.empty())
    {
      ss <<
        "Location: " <<
        location.label <<
        std::endl;
    }

    ss <<
      "Power: " <<
      (power ? "On" : "Off") <<
      std::endl;

    ss <<
      "MAC: " <<
      std::setfill('0') << std::setw(2) <<
      std::hex << static_cast<int>(mac_address[0]) << ":" <<
      std::setfill('0') << std::setw(2) <<
      std::hex << static_cast<int>(mac_address[1]) << ":" <<
      std::setfill('0') << std::setw(2) <<
      std::hex << static_cast<int>(mac_address[2]) << ":" <<
      std::setfill('0') << std::setw(2) <<
      std::hex << static_cast<int>(mac_address[3]) << ":" <<
      std::setfill('0') << std::setw(2) <<
      std::hex << static_cast<int>(mac_address[4]) << ":" <<
      std::setfill('0') << std::setw(2) <<
      std::hex << static_cast<int>(mac_address[5]) <<
      std::setw(0) << std::dec <<
      std::endl;

    ss <<
      "Details: " <<
      std::endl <<
      "  Vendor: " <<
      version.vendor <<
      std::endl <<
      "  Product: " <<
      version.product <<
      std::endl <<
      "  Version : " <<
      (version.version >> 2) <<
      '.' <<
      (version.version & 0xFF) <<
      std::endl;

    ss <<
      "Color: " <<
      std::endl <<
      "  Hue: " <<
      color.hue <<
      std::endl <<
      "  Saturation: " <<
      color.saturation <<
      std::endl <<
      "  Brightness: " <<
      color.brightness <<
      std::endl <<
      "  Kelvin: " <<
      color.kelvin <<
      std::endl;

    ss << std::endl;


    return std::move(ss.str());
  }

};

std::ostream& operator << (std::ostream& os, const Lightbulb& bulb)
{
  std::string output = bulb;
  os << std::move(output);
  return os;
}
