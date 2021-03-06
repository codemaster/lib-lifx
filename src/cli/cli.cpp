/////
// cli.cpp
//! @file CLI tool implementation
/////

#include "lightbulb.h"

#include <iostream>
#include <regex>
#include <unordered_map>

#include <stdio.h>

std::unordered_map<std::string, lifx::HSBK> colors =
{
  { "red", { 62978, 65535, 65535, 3500 }},
  { "orange", { 5525, 65535, 65535, 3500 }},
  { "yellow", { 7615, 65535, 65535, 3500 } },
  { "green", { 16173, 65535, 65535, 3500 } },
  { "cyan", { 29814, 65535, 65535, 3500 } },
  { "blue", { 43634, 65535, 65535, 3500 } },
  { "purple", { 50486, 65535, 65535, 3500 } },
  { "pink", { 58275, 65535, 47142, 3500 } },
  { "white", { 58275, 0, 65535, 5500 } },
  { "cold_white", { 58275, 0, 65535, 9000 } },
  { "warm_white", { 58275, 0, 65535, 3200 } },
  { "gold", { 58275, 0, 65535, 2500 } },
};

std::string MacToString(const uint8_t address[8])
{
  std::stringstream ss;
  ss << address[0] << ":"
    << address[1] << ":"
    << address[2] << ":"
    << address[3] << ":"
    << address[4] << ":"
    << address[5];
  return std::move(ss.str());
}

uint64_t MacToNum(const uint8_t address[8])
{
  uint64_t num =
    static_cast<uint64_t>(address[0]) << 56 |
    static_cast<uint64_t>(address[1]) << 48 |
    static_cast<uint64_t>(address[2]) << 40 |
    static_cast<uint64_t>(address[3]) << 32 |
    address[4] << 24 |
    address[5] << 16 |
    address[6] << 8 |
    address[7] << 0;
  return std::move(num);
}

std::unordered_map<uint64_t, Lightbulb> g_lightbulbs;
lifx::LifxClient g_client;

template<typename T>
void HandleCallback(
  std::function<void(Lightbulb& bulb, const T& msg)> func)
{
  g_client.RegisterCallback<T>(
    [func = std::move(func)]
    (const lifx::Header& header, const T& msg)
  {
    auto num = MacToNum(header.target);
    auto bulb = g_lightbulbs.find(num);
    if (bulb != g_lightbulbs.end())
    {
      func(bulb->second, msg);
    }
  });
}

bool DoForFilteredLightbulbs(const std::string& filter,
  std::function<bool(const Lightbulb& bulb)> func)
{
  bool ret = false;

  std::regex filterRegex(".*" + filter + ".*");
  for (auto iter : g_lightbulbs)
  {
    // If the filter is "all", just run func on every bulb
    if (filter == "all")
    {
      ret = func(iter.second) || ret;
      continue;
    }

    // Try to match to various data
    if (std::regex_match(iter.second.group.label, filterRegex) ||
      std::regex_match(iter.second.label, filterRegex) ||
      std::regex_match(iter.second.location.label, filterRegex) ||
      std::regex_match(MacToString(iter.second.mac_address.data()), filterRegex))
    {
      ret = func(iter.second) || ret;
    }
  }

  return ret;
}

void PrintUsage(char** const argv)
{
  std::cout << "Usage: " << argv[0] <<
  " <filter> <command> [arguments, ...]" << std::endl << std::endl;
}

void PrintHelp(char** const argv)
{
  std::cout <<
  "LIFX Command Line Interface tool" << std::endl;
  PrintUsage(argv);
  std::cout <<
  "Filter:" << std::endl <<
  "'all' to perform the command on all LAN-discovered lights." << std::endl <<
  "Alternatively, a full or partial name can be provided." << std::endl << std::endl <<
  "Commands:" << std::endl <<
  "help:   Display usage informaiton." << std::endl <<
  "off:    Turns off a light." << std::endl <<
  "on:     Turns on a light." << std::endl <<
  "status: Obtains the status & info of a light." << std::endl <<
  "color:  Sets the color of a light." << std::endl <<
  "        1st argument is the name of the color." << std::endl <<
  "        2nd argument is how many miliseconds it should " <<
  "take to tranition to the new color." << std::endl <<
  std::endl;
}

void RunCommands(int argc, char** argv)
{
  if (argc < 2)
  {
    PrintUsage(argv);
    return;
  }

  std::string filter("all");
  std::string command("");
  std::vector<std::string> arguments;
  // Options provided
  if (argc > 2)
  {
    // Filter and command provided
    filter = argv[1];
    command = argv[2];
    // Append the arguments
    if (argc > 3)
    {
      for (auto i = 3; i < argc; ++i)
      {
        arguments.emplace_back(std::move(argv[i]));
      }
    }
  }
  else
  {
    // No filter, command only
    command = argv[1];
  }
  // Convert the filter to all lowercase
  std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
  // Convert the command to all lowercase
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  // Convert all arguments to all lowercase
  for (size_t i = 0; i < arguments.size(); ++i)
  {
    std::transform(arguments[i].begin(), arguments[i].end(),
      arguments[i].begin(), ::tolower);
  }

  // Handle the help command
  if (command == "help" ||
      command == "-help" ||
      command == "--help")
  {
    PrintHelp(argv);
    return;
  }

  // Find all the lightbulbs based on the filter
  DoForFilteredLightbulbs(filter,
    [&command, &arguments, &argv](const Lightbulb& bulb) -> bool
  {
    if (command == "off")
    {
      g_client.Send<lifx::message::device::SetPower>(bulb.mac_address.data());
      // unknown or v1 products sometimes have issues setting power, so
      // we have to send it twice
      g_client.Send<lifx::message::device::SetPower>(bulb.mac_address.data());
    }

    if (command == "on")
    {
      lifx::message::device::SetPower powerMsg{ 65535 };
      g_client.Send<lifx::message::device::SetPower>(powerMsg, bulb.mac_address.data());
      // unknown or v1 products sometimes have issues setting power, so
      // we have to send it twice
      g_client.Send<lifx::message::device::SetPower>(powerMsg, bulb.mac_address.data());
    }

    if (command == "status")
    {
      std::cout << bulb;
    }

    if (command == "color")
    {
      // At least 1 argument (color) must be provided
      if (arguments.size() > 0)
      {
        // Find the color provided
        auto color = colors.find(arguments[0]);
        if (color == colors.end())
        {
          std::cerr << "Unknown color specified: '" << arguments[0] << "'" << std::endl;
        } else {
          // By default, immediately set the color
          lifx::message::light::SetColor colorMsg{color->second, 0};
          // Optional second argument is for the number of milliseconds to change the color over
          if (arguments.size() > 1)
          {
            colorMsg.duration = std::stoul(arguments[1]);
          }
          g_client.Send<lifx::message::light::SetColor>(colorMsg, bulb.mac_address.data());
        }
      } else {
        std::cerr << "You must specify a color." << std::endl;
      }
    }

    return true;
  });

  while (g_client.WaitingToSend())
  {
    g_client.RunOnce();
  }
}

int main(int argc, char** argv)
{
  // Check if we are fast-tracking to the help command
  if (argc > 1)
  {
    if (strcmp(argv[1], "help") == 0 ||
        strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-help") == 0)
    {
      PrintHelp(argv);
      return 0;
    }
  } else if (argc == 1) {
    PrintUsage(argv);
    return 0;
  }

  g_client.RegisterCallback<lifx::message::device::StateService>(
    [](const lifx::Header& header, const lifx::message::device::StateService& msg)
  {
    if (msg.service == lifx::SERVICE_UDP)
    {
      Lightbulb bulb {};
      for (auto i : {0, 1, 2, 3, 4, 5, 6, 7})
      {
        bulb.mac_address[i] = header.target[i];
      }

      g_lightbulbs[MacToNum(header.target)] = bulb;

      HandleCallback<lifx::message::device::StateLocation>(
        [](Lightbulb& bulb, const lifx::message::device::StateLocation& msg)
      {
        bulb.location = { msg.label, msg.updated_at };
      });

      HandleCallback<lifx::message::device::StateVersion>(
        [](Lightbulb& bulb, const lifx::message::device::StateVersion& msg)
      {
        bulb.version = { msg.vendor, msg.product, msg.version };

        g_client.Send<lifx::message::device::GetLocation>(bulb.mac_address.data());
      });

      HandleCallback<lifx::message::device::StateGroup>(
        [](Lightbulb& bulb, const lifx::message::device::StateGroup& msg)
      {
        bulb.group = { msg.label, msg.updated_at };

        g_client.Send<lifx::message::device::GetVersion>(bulb.mac_address.data());
      });

      HandleCallback<lifx::message::light::State>(
        [](Lightbulb& bulb, const lifx::message::light::State& msg)
      {
        bulb.power = msg.power > 0;
        bulb.label = msg.label;
        bulb.color = msg.color;

        g_client.Send<lifx::message::device::GetGroup>(bulb.mac_address.data());
      });
      
      g_client.Send<lifx::message::light::Get>(bulb.mac_address.data());
    }
  });

  g_client.Broadcast<lifx::message::device::GetService>({});
  unsigned int num_identified = 0;
  for (;;)
  {
    g_client.RunOnce();

    if (!g_lightbulbs.empty())
    {
      num_identified = 0;
      std::for_each(g_lightbulbs.cbegin(), g_lightbulbs.cend(),
        [&num_identified](const std::pair<uint64_t, Lightbulb>& entry) {
        if (!entry.second.location.label.empty() &&
          entry.second.location.updated_at != 0)
        {
          ++num_identified;
        }
      });

      if (num_identified == g_lightbulbs.size())
      {
        RunCommands(argc, argv);
        break;
      }
    }
  }

  return 0;
}
