/////
// lifx.h
//! @file Library header
/////

#pragma once

#include <lib-lifx/lifx_messages.h>

#include <functional>
#include <memory>
#include <random>
#include <vector>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <limits.h>
#include <string.h>

namespace lifx
{

// Constant definitions
constexpr uint32_t MAX_LIFX_PACKET_SIZE = 512;
constexpr uint8_t SERVICE_UDP = 1;
constexpr uint32_t MAX_MESSAGES_PER_SECOND = 20;

constexpr auto LIFX_PROTOCOL = 1024;
#ifdef _WIN32
constexpr auto LIFX_HEADER_SIZE = sizeof(lifx::Header) - 1;
constexpr auto LIFX_NETWORK_HEADER_SIZE = sizeof(lifx::NetworkHeader);
#else
constexpr auto LIFX_HEADER_SIZE = sizeof(lifx::Header);
#endif

class LifxClient
{
  public:
    //! Result of processing the Linux client
    enum class RunResult
    {
      RUN_WAITING       = 0, //!< Waiting for actions to take
      RUN_ERROR         = 1, //!< An error occurred
      RUN_RECEIVED_DATA = 2, //!< Received data
      RUN_SENT_DATA     = 3, //!< Sent data
      RUN_SENT_LIMIT    = 4, //!< Sending limit has been reached
    };

    //! Callback template for received messages
    template<typename T> using LifxCallback =
      std::function<void(const Header header, const T& message)>;

    //! Constructor for LifxClient that optionally sets the source ID.
    //! This creates the socket in which all LIFX messages are sent and received.
    //! @param[in] sourceId Optional Source ID for all messages. Can be
    //! checked on received messages to see which LifxClient sent the request.
    LifxClient(uint32_t sourceId = 0);
    //! Default destructor. Deletes all callbacks and closes the created socket.
    virtual ~LifxClient();
    //! Broadcasts a message to all LIFX devices on the local network
    //! using the provided message.
    //! @tparam T The message type to broadcast.
    //! @param[in] message The message that will be broadcasted.
    template<typename T> uint8_t Broadcast(T&& message);
    //! Broadcasts a message to all LIFX devices on the local network
    //! using an empty/default payload.
    //! @tparam T The message type to broadcast.
    template<typename T> uint8_t Broadcast();
    //! Sends a message in the current network. If target is nullptr,
    //! the message will be broadcasted instead.
    //! @tparam T The message type to send.
    //! @param[in] message The message that will be sent.
    //! @param[in] target The target to send the message to. If this is
    //! nullptr, the message will be broadcasted on the current network instead.
    template<typename T> uint8_t Send(const T& message,
      const uint8_t target[8] = nullptr);
    //! Sends a empty/default payload of a message type in the current
    //! network. If target is nullptr, the message will be broadcasted instead.
    //! @tparam T The message type to send.
    //! @param[in] target The target to send the message to. If this is
    //! nullptr, the message will be broadcasted on the current network instead.
    template<typename T> uint8_t Send(const uint8_t target[8] = nullptr);
    //! Registers a @ref LifxCallback callback function for when
    //! a specific message type is received.
    //! @tparam T The message type to trigger the callback function for.
    //! @param[in] callback The callback to be triggered when the
    //! specified message type si received.
    template<typename T> void RegisterCallback(LifxCallback<T> callback);
    //! Runs the LifxClient's internal processing for one loop. Call this
    //! repeatedly to continue functionality.
    //! @param[in] seconds The number of seconds that the client may process for.
    //! @param[in] milliseconds The number of milliseconds in addition to
    //! the provided seconds that the client may process for.
    virtual RunResult RunOnce(long seconds = 0, long milliseconds = 1);
    //! Checks if there are any messages waiting in the client's queue to be sent.
    virtual bool WaitingToSend() const;
  protected:
    //! Internal callback template for received messages
    using LifxInternalCallback =
      std::function<void(const Header header, const void* data)>;

    //! Sends the buffer put together by the internal client system.
    //! @param[in] buffer The buffer to send over the network.
    virtual int SendBuffer(const std::vector<char>& buffer);
    //! Attempts to receive any of the provided types of messages
    //! @tparam T Variadic template of possible types to receive
    //! @param header The header that identifies the incoming message in the buffer
    //! @param buffer The buffer containing the raw message
    template<typename ... T> void ReceiveMessageTypes(const Header& header,
      const char* buffer);
    //! Tries to retrieve a message from a provided buffer
    //! based on the provided header.
    //! @tparam T The type of the message to retrieve from the buffer.
    //! @param[in] header The header received
    //! @param[in] buffer The buffer to retrieve the message from.
    template<typename T> void TryReceiveMessage(const Header& header,
      const char* buffer);
    //! Runs the callback registered to a message type.
    //! @tparam T The type of the message to run the callback for.
    //! @param[in] header The header of the received message.
    //! @param[in] msg The message payload.
    template<typename T> void RunCallback(const Header& header, const T& msg);

    //! Converts a @ref Header to a @ref NetworkHeader
    //! @param[in] h @ref Header to convert
    //! @returns A converted @ref NetworkHeader object
    static NetworkHeader ToNetwork(const Header& h);
    //! Converts a @ref NetworkHeader to a @ref Header
    //! @param[in] nh @ref NetworkHeader to convert
    //! @returns A converted @ref Header object
    static Header FromNetwork(const NetworkHeader& nh);

    //! Map that contains the callbacks that are waiting to be triggered.
    std::unordered_map<uint16_t, LifxInternalCallback> m_callbacks;
    //! Map that contains the buffers pending being sent.
    std::unordered_map<uint8_t, std::vector<char>> m_pendingSends;
    //! The source ID of the client; optionally provided in constructor.
    uint32_t m_sourceId;
};

template<typename T>
uint8_t LifxClient::Broadcast(T&& message)
{
  return std::move(Send<T>(std::forward<T>(message), nullptr));
}

template<typename T>
uint8_t LifxClient::Broadcast()
{
  return std::move(Send<T>(nullptr));
}

template<typename T>
uint8_t LifxClient::Send(const T& message, const uint8_t target[8])
{
  // Create header
  lifx::Header header = { };
  header.size = lifx::LIFX_HEADER_SIZE;
  header.origin = 0;
  header.tagged = std::remove_reference<T>::type::type == message::device::GetService::type ? 1 : 0;
  header.addressable = 1;
  header.protocol = LIFX_PROTOCOL;
  header.source = m_sourceId;
  for (auto&& i : { 0,1,2,3,4,5,6,7 })
  {
    header.target[i] = (target == nullptr) ? 0 : target[i];
  }
  header.ack_required = 0;
  header.res_required = std::remove_reference<T>::type::has_response ? 1 : 0;
  header.type = std::remove_reference<T>::type::type;
  // Generate a random sequence for each message
  uint8_t generatedSequence;
  do {
    static std::random_device randomDevice;
    static std::mt19937 mtRand(randomDevice());
    static std::uniform_int_distribution<short> uniformDistribution{ 1, UCHAR_MAX };
    generatedSequence = static_cast<uint8_t>(uniformDistribution(mtRand));
    header.sequence = generatedSequence;
  } while (m_pendingSends.find(header.sequence) != m_pendingSends.end());

  // Copy header & message to buffer
  auto messageSize = sizeof(T);
  std::vector<char> buffer(LIFX_HEADER_SIZE + messageSize, 0);
  auto nh = ToNetwork(header);
  memcpy(buffer.data(), &nh, LIFX_HEADER_SIZE);
  memcpy((buffer.data() + LIFX_HEADER_SIZE), &message, messageSize);

  // Queue the send
  m_pendingSends[generatedSequence] = buffer;

  return std::move(generatedSequence);
}

template<typename T>
uint8_t LifxClient::Send(const uint8_t target[8])
{
  return std::move(Send<T>({}, target));
}

template<typename T>
void LifxClient::RegisterCallback(LifxClient::LifxCallback<T> callback)
{
  m_callbacks[T::type] =
  [callback = std::move(callback)]
  (const Header header, const void* data)
  {
    if (header.type == T::type)
    {
      callback(header, *(static_cast<const T*>(data)));
    }
  };
}

template<typename T>
void LifxClient::TryReceiveMessage(const Header& header, const char* buffer)
{
  if (header.type != T::type)
    return;

  T msg;

  if (buffer != nullptr)
  {
    memset(&msg, 0, sizeof(T));
    memcpy(&msg, (buffer + LIFX_HEADER_SIZE), sizeof(T));
  }

  RunCallback(header, std::move(msg));
}

template<typename T>
void LifxClient::RunCallback(const Header& header, const T& msg)
{
  auto callbackIter = m_callbacks.find(T::type);
  if (callbackIter != m_callbacks.cend())
  {
    callbackIter->second(header, static_cast<const void*>(&msg));
  }
}

} // namespace lifx
