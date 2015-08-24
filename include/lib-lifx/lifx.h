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
#include <unordered_map>
#include <utility>

namespace lifx
{

// Constant definitions
constexpr unsigned int MAX_LIFX_PACKET_SIZE = 512;
constexpr uint8_t SERVICE_UDP = 1;
constexpr unsigned int MAX_MESSAGES_PER_SECOND = 20;

constexpr auto LIFX_PROTOCOL = 1024;
#ifdef _WIN32
constexpr auto LIFX_HEADER_SIZE = sizeof(lifx::Header) - 1;
constexpr auto LIFX_NETWORK_HEADER_SIZE = sizeof(lifx::NetworkHeader);
#else
constexpr auto LIFX_HEADER_SIZE = sizeof(lifx::Header);
#endif

NetworkHeader ToNetwork(const Header& h);
Header FromNetwork(const NetworkHeader& nh);

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
    LifxClient(int sourceId = 0);
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
    template<typename T> uint8_t Send(const T& message, const uint8_t target[8] = nullptr);
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
    RunResult RunOnce(long seconds = 0, long milliseconds = 1);
    //! Checks if there are any messages waiting in the client's queue to be sent.
    bool WaitingToSend() const;
  private:
    //! Sends the buffer put together by the internal client system.
    //! @param[in] buffer The buffer to send over the network.
    int SendBuffer(const std::vector<char>& buffer);
    //! Retrieves a message from a provided buffer.
    //! @tparam T The type of the message to retrieve from the buffer.
    //! @param[in] buffer The buffer to retrieve the message from.
    template<typename T> T ReceiveMessage(const char* buffer);
    //! Runs the callback registered to a message type.
    //! @tparam T The type of the message to run the callback for.
    //! @param[in] header The header of the received message.
    //! @param[in] msg The message payload.
    template<typename T> void RunCallback(const Header& header, const T& msg);

    //! Map that contains the callbacks that are waiting to be triggered.
    std::unordered_map<uint16_t, void*> m_callbacks;
    //! Map that contains the buffers pending being sent.
    std::unordered_map<uint8_t, std::vector<char>> m_pendingSends;
    //! The source ID of the client; optionally provided in constructor.
    int m_sourceId;
};

template<typename T>
uint8_t LifxClient::Broadcast(T&& message)
{
  return std::move(Send(std::forward<T>(message), nullptr));
}

template<typename T>
uint8_t LifxClient::Broadcast()
{
  return std::move(Send(nullptr));
}

template<typename T>
uint8_t LifxClient::Send(const T& message, const uint8_t target[8])
{
  // Create header
  lifx::Header header = { };
  header.size = lifx::LIFX_HEADER_SIZE;
  header.origin = 0;
  header.tagged = T::type == message::device::GetService::type ? 1 : 0;
  header.addressable = 1;
  header.protocol = LIFX_PROTOCOL;
  header.source = m_sourceId;
  for (auto&& i : { 0,1,2,3,4,5,6,7 })
  {
    header.target[i] = (target == nullptr) ? 0 : target[i];
  }
  header.ack_required = 0;
  header.res_required = T::has_response ? 1 : 0;
  header.type = T::type;
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
  LifxCallback<T>* cbPtr = new LifxCallback<T>(callback);
  m_callbacks[T::type] = static_cast<void*>(cbPtr);
}

template<typename T>
T LifxClient::ReceiveMessage(const char* buffer)
{
  T msg;

  if (buffer != nullptr)
  {
    memset(&msg, 0, sizeof(T));
    memcpy(&msg, (buffer + LIFX_HEADER_SIZE), sizeof(T));
  }

  return std::move(msg);
}

template<typename T>
void LifxClient::RunCallback(const Header& header, const T& msg)
{
  auto callbackIter = m_callbacks.find(T::type);
  if (callbackIter != m_callbacks.end() &&
    callbackIter->second != nullptr)
  {
    LifxCallback<T>* callback =
      static_cast<LifxCallback<T>*>(callbackIter->second);
    (*callback)(header, msg);
  }
}

} // namespace lifx
