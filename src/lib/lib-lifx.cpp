/////
// lib-lifx.cpp
//! @file Library implementation
/////

#include <lib-lifx/lifx.h>

#include <array>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace
{
  constexpr int sockAddrLen = sizeof(sockaddr_in);
  struct sockaddr_in listen_addr = { };
  struct sockaddr_in send_direct_addr = { };
  struct sockaddr_in broadcast_addr = { };
  int sock;
}

namespace lifx
{
  LifxClient::LifxClient(int sourceId)
    : m_sourceId(std::move(sourceId))
  {
    // TODO: Error checking

#ifdef _WIN32
    // Start WinSock
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    // Setup addresses
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(56700);
    listen_addr.sin_family = AF_INET;

    //send_direct_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    send_direct_addr.sin_port = htons(56700);
    send_direct_addr.sin_family = AF_INET;

    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    broadcast_addr.sin_port = htons(56700);
    broadcast_addr.sin_family = AF_INET;

    // Create our actual socket
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Allow this socket to broadcast
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&yes, sizeof(int));

    // Allow this socket to reuse addresses
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int));

    // Bind the socket to our listening address
    bind(sock, (struct sockaddr*)&listen_addr, sockAddrLen);
  }

  LifxClient::~LifxClient()
  {
#ifdef _WIN32
    // Close the socket
    closesocket(sock);
    // Stop WinSock
    WSACleanup();
#else
    // Close the socket
    close(sock);
#endif
  }

  int LifxClient::SendBuffer(const std::vector<char>& buffer)
  {
    if (buffer.empty())
      return 0;

    int ret = sendto(sock, buffer.data(), buffer.size(), 0,
      (struct sockaddr*)&broadcast_addr, sockAddrLen);
    return std::move(ret);
  }

  template<typename ... T> void LifxClient::ReceiveMessageTypes(const Header& header, const char* buffer)
  {
    // Fancy hack in a way:
    // Abuse initialize lists so that we call @ref TryReceiveMessage
    // for each of the provided types in the variadic template
    // Thanks to Casey; ref: http://stackoverflow.com/a/21194071
    int _[] = {0, ( TryReceiveMessage<T>(header, buffer), 0)...};
    (void)_;
  }

  LifxClient::RunResult LifxClient::RunOnce(long seconds, long milliseconds)
  {
    struct timeval timeout;
    timeout.tv_sec = std::move(seconds);
    timeout.tv_usec = std::move(milliseconds);

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(static_cast<uint32_t>(sock), &rfds);

    int ret = select(sock+1, &rfds, nullptr, nullptr, &timeout);
    if (ret == -1)
    {
      return RunResult::RUN_ERROR;
    }
    else if (ret && FD_ISSET(sock, &rfds))
    {
      std::array<char, MAX_LIFX_PACKET_SIZE> buffer;
      recv(sock, buffer.data(), MAX_LIFX_PACKET_SIZE, 0);

      NetworkHeader nh = { };
      memcpy(&nh, buffer.data(), LIFX_HEADER_SIZE);
      auto header = FromNetwork(nh);

      ReceiveMessageTypes<
        message::device::GetService,
        message::device::StateService,
        message::device::GetHostInfo,
        message::device::StateHostInfo,
        message::device::GetHostFirmware,
        message::device::StateHostFirmware,
        message::device::GetWifiInfo,
        message::device::StateWifiInfo,
        message::device::GetWifiFirmware,
        message::device::StateWifiFirmware,
        message::device::GetPower,
        message::device::SetPower,
        message::device::StatePower,
        message::device::GetLabel,
        message::device::SetLabel,
        message::device::StateLabel,
        message::device::GetVersion,
        message::device::StateVersion,
        message::device::GetInfo,
        message::device::StateInfo,
        message::device::Acknowledgement,
        message::device::GetLocation,
        message::device::StateLocation,
        message::device::GetGroup,
        message::device::StateGroup,
        message::device::EchoRequest,
        message::device::EchoResponse,
        message::light::Get,
        message::light::SetColor,
        message::light::State,
        message::light::GetPower,
        message::light::SetPower,
        message::light::StatePower
      >(header, buffer.data());

      return RunResult::RUN_RECEIVED_DATA;
    }

    if (!m_pendingSends.empty())
    {
      static auto start_time = std::chrono::steady_clock::now();
      static uint32_t messages_sent = 0;

      // Check if we have exceeded the number of messages per second
      auto seconds_since_start =
        std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - start_time);
      if (seconds_since_start.count() > 0)
      {
        auto messages_per_second = (messages_sent / seconds_since_start.count());
        if (messages_per_second >= MAX_MESSAGES_PER_SECOND)
        {
          // hold off sending until we are under the limit
          return RunResult::RUN_SENT_LIMIT;
        }
      }

      auto tosend = m_pendingSends.begin();
      SendBuffer(tosend->second);
      m_pendingSends.erase(tosend);
      ++messages_sent;

      return RunResult::RUN_SENT_DATA;
    }

    return RunResult::RUN_WAITING;
  }

  bool LifxClient::WaitingToSend() const
  {
    return ! m_pendingSends.empty();
  }

  NetworkHeader LifxClient::ToNetwork(const Header& h)
  {
#ifdef _WIN32
    NetworkHeader nh = { };
    nh.frame = h.size;
    nh.frame |= h.protocol << 16;
    nh.frame |= h.addressable << 28;
    nh.frame |= h.tagged << 29;
    nh.frame |= h.origin << 30;
    nh.frame |= static_cast<uint64_t>(h.source) << 32;

    nh.target = h.target[0];
    nh.target |= h.target[1] << 8;
    nh.target |= h.target[2] << 16;
    nh.target |= h.target[3] << 24;
    nh.target |= static_cast<uint64_t>(h.target[4]) << 32;
    nh.target |= static_cast<uint64_t>(h.target[5]) << 40;
    nh.target |= static_cast<uint64_t>(h.target[6]) << 48;
    nh.target |= static_cast<uint64_t>(h.target[7]) << 56;

    nh.frame_address = h.site[0];
    nh.frame_address |= h.site[1] << 8;
    nh.frame_address |= h.site[2] << 16;
    nh.frame_address |= h.site[3] << 24;
    nh.frame_address |= static_cast<uint64_t>(h.site[4]) << 32;
    nh.frame_address |= static_cast<uint64_t>(h.site[5]) << 40;
    nh.frame_address |= static_cast<uint64_t>(h.ack_required) << 48;
    nh.frame_address |= static_cast<uint64_t>(h.res_required) << 49;
    //nh.frame_address |= static_cast<uint64_t>(h.reserved) << 50;
    nh.frame_address |= static_cast<uint64_t>(h.sequence) << 56;

    nh.time = h.at_time;

    nh.type = h.type;

    //nh.reserved = h.reserved;

    return std::move(nh);
#else
    return h;
#endif
  }

  Header LifxClient::FromNetwork(const NetworkHeader& nh)
  {
#ifdef _WIN32
    Header h = { };

    // reserved = nh.reserved; // 16

    h.type = nh.type; // 16

    h.at_time = nh.time; // 64

    h.target[0] = nh.target & 0xFF; // 8
    h.target[1] = (nh.target >> 8) & 0xFF; // 8
    h.target[2] = (nh.target >> 16) & 0xFF; // 8
    h.target[3] = (nh.target >> 24) & 0xFF; // 8
    h.target[4] = (nh.target >> 32) & 0xFF; // 8
    h.target[5] = (nh.target >> 40) & 0xFF; // 8
    h.target[6] = (nh.target >> 48) & 0xFF; // 8
    h.target[7] = (nh.target >> 56) & 0xFF; // 8

    h.site[0] = nh.frame_address & 0xFF; // 8
    h.site[1] = (nh.frame_address >> 8) & 0xFF; // 8
    h.site[2] = (nh.frame_address >> 16) & 0xFF; // 8
    h.site[3] = (nh.frame_address >> 24) & 0xFF; // 8
    h.site[4] = (nh.frame_address >> 32) & 0xFF; // 8
    h.site[5] = (nh.frame_address >> 40) & 0xFF; // 8
    h.ack_required = (nh.frame_address >> 48) & 0x1; // 1
    h.res_required = (nh.frame_address >> 49) & 0x1; // 1
                                                     //h.reserved = (nh.frame_address >> 50) & 0x3f; // 6
    h.sequence = (nh.frame_address >> 56) & 0xFF; // 8

    h.size = nh.frame & 0xFFFF; // 16
    h.protocol = (nh.frame >> 16) & 0xFFF; // 12
    h.addressable = (nh.frame >> 28) & 0x1; // 1
    h.tagged = (nh.frame >> 29) & 0x1; //1
    h.origin = (nh.frame >> 30) & 0x3; // 2
    h.source = (nh.frame >> 32) & 0xFFFFFFFF; //32

    return std::move(h);
#else
    return nh;
#endif
  }

} // namespace lifx
