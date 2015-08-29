/////
// test.cpp
//! @file LIFX Unit Tests
/////

#include <lib-lifx/lifx.h>

#include <gtest/gtest.h>

#include <array>
#include <mutex>
#include <functional>

namespace
{

void TestCallback(const lifx::Header,
  const lifx::message::device::EchoRequest&) { }

class TestLifxClient : public lifx::LifxClient
{
  public:
    void SetSourceId(uint32_t num = 0)
    {
      m_sourceId = std::move(num);
    }

    uint32_t GetSourceId() const
    {
      return m_sourceId;
    }

    lifx::Header GetPendingSendHeader(uint8_t gn)
    {
      lifx::Header header {};
      auto iter = m_pendingSends.find(gn);
      if (iter == m_pendingSends.end())
        return header;

      lifx::NetworkHeader nh;
      memcpy(&nh, iter->second.data(), lifx::LIFX_HEADER_SIZE);

      header = FromNetwork(nh);

      EXPECT_EQ(header.sequence, gn);

      return header;
    }

    template<typename T> T GetPendingSendMessage(uint8_t gn, lifx::Header& header)
    {
      EXPECT_EQ(header.sequence, gn);
      EXPECT_EQ(std::remove_reference<T>::type::type, header.type);

      T message{};
      auto iter = m_pendingSends.find(gn);
      if (iter == m_pendingSends.end())
        return message;

      memcpy(&message, (iter->second.data() + lifx::LIFX_HEADER_SIZE), sizeof(T));

      return message;
    }

    const char* GetPendingSendBuffer(uint8_t gn)
    {
      auto iter = m_pendingSends.find(gn);
      if (iter == m_pendingSends.end())
        return nullptr;

      return iter->second.data();
    }

    void TestMemberCallback(const lifx::Header,
      const lifx::message::device::EchoRequest&) { }

    RunResult RunOnce(long seconds = 0, long milliseconds = 1) override
    {
      if (m_errorState)
      {
        m_errorState = !m_errorState;
        return RunResult::RUN_ERROR;
      }

      if (m_sendsLimited)
      {
        m_sendsLimited = !m_sendsLimited;
        return RunResult::RUN_SENT_LIMIT;
      }

      if (m_pretendReceive)
      {
        m_pretendReceive = !m_pretendReceive;
        return RunResult::RUN_RECEIVED_DATA;
      }

      return LifxClient::RunOnce(seconds, milliseconds);
    }

    int SendBuffer(const std::vector<char>& buffer) override
    {
      // Don't actually send anything
      return buffer.size();
    }

    template<typename ... T> void ReceiveMessageTypes(const lifx::Header& header,
      const char* buffer)
    {
      LifxClient::ReceiveMessageTypes<T...>(header, buffer);
    }

    template<typename T> void TryReceiveMessage(const lifx::Header& header,
      const char* buffer)
    {
      LifxClient::TryReceiveMessage<T>(header, buffer);
    }

    bool m_errorState;
    bool m_sendsLimited;
    bool m_pretendReceive;
};

class TestClient
  : public testing::Test
{
  protected:
    TestClient()
    {
      m_client = std::unique_ptr<TestLifxClient>(new TestLifxClient());
    }

    std::unique_ptr<TestLifxClient> m_client;
    std::array<uint8_t, 8> m_sendTarget { { 0, 1, 2, 3, 4, 5, 6, 7 } };
};

TEST_F(TestClient, TestSourceID)
{
  // Test setting the source ID
  constexpr unsigned int sourceId = 123;
  m_client->SetSourceId(sourceId);
  ASSERT_EQ(sourceId, m_client->GetSourceId());

  // Test a generated message
  auto gn = m_client->Send<lifx::message::device::EchoRequest>();
  lifx::Header header = m_client->GetPendingSendHeader(gn);
  ASSERT_EQ(sourceId, header.source);
}

TEST_F(TestClient, TestBlankSourceID)
{
  auto gn = m_client->Send<lifx::message::device::EchoRequest>();
  lifx::Header header = m_client->GetPendingSendHeader(gn);
  ASSERT_EQ(static_cast<unsigned int>(0), header.source);
}

TEST_F(TestClient, TestBroadcastEmptyMessage)
{
  auto gn = m_client->Broadcast<lifx::message::device::EchoRequest>();
  lifx::Header header = m_client->GetPendingSendHeader(gn);
  ASSERT_EQ(lifx::message::device::EchoRequest::type, header.type);
}

TEST_F(TestClient, TestBroadcastActualMessage)
{
  constexpr uint64_t echo = 123456;

  lifx::message::device::EchoRequest echoMsg = { echo };
  auto gn = m_client->Broadcast(echoMsg);
  lifx::Header header = m_client->GetPendingSendHeader(gn);
  lifx::message::device::EchoRequest hopefullyEchoMsg =
    m_client->GetPendingSendMessage<lifx::message::device::EchoRequest>(gn, header);
  ASSERT_EQ(echo, hopefullyEchoMsg.payload);
}

TEST_F(TestClient, TestBroadcastBadMessage)
{
  // The following bad message won't even compile
  // confirming that bad messages can't be broadcasted
  /*struct { } derp;
  m_client->Broadcast(derp);*/
}

TEST_F(TestClient, TestSendEmptyMessage)
{
  auto gn = m_client->Send<lifx::message::device::EchoRequest>(
    {}, m_sendTarget.data());
  lifx::Header header = m_client->GetPendingSendHeader(gn);
  for (auto&& t : {0,1,2,3,4,5,6,7})
  {
    ASSERT_EQ(m_sendTarget[t], header.target[t]);
  }
  ASSERT_EQ(lifx::message::device::EchoRequest::type, header.type);
}

TEST_F(TestClient, TestSendActualMessage)
{
  constexpr uint64_t echo = 123456;

  lifx::message::device::EchoRequest echoMsg = { echo };
  auto gn = m_client->Send(echoMsg, m_sendTarget.data());
  lifx::Header header = m_client->GetPendingSendHeader(gn);
  lifx::message::device::EchoRequest hopefullyEchoMsg =
    m_client->GetPendingSendMessage<lifx::message::device::EchoRequest>(gn, header);
  for (auto&& t : {0,1,2,3,4,5,6,7})
  {
    ASSERT_EQ(m_sendTarget[t], header.target[t]);
  }
  ASSERT_EQ(echo, hopefullyEchoMsg.payload);
}

TEST_F(TestClient, TestSendBadMessage)
{
  // The following bad message won't even compile
  // confirming that bad messages can't be sent
  /*struct { } derp;
  m_client->Send(derp);*/
}

TEST_F(TestClient, RegisterACallbackFunction)
{
  m_client->RegisterCallback<lifx::message::device::EchoRequest>(TestCallback);
}

TEST_F(TestClient, RegisterACallbackBind)
{
  m_client->RegisterCallback<lifx::message::device::EchoRequest>(std::bind(
    &TestLifxClient::TestMemberCallback, m_client.get(),
    std::placeholders::_1, std::placeholders::_2));
}

TEST_F(TestClient, RegisterACallbackLambda)
{
  m_client->RegisterCallback<lifx::message::device::EchoRequest>([]
    (const lifx::Header, const lifx::message::device::EchoRequest&)
    {});
}

TEST_F(TestClient, RegisterACallbackNull)
{
  // The following three function calls won't even compile
  // So we are guaranteed that we cannot register a NULL callback
  /*m_client->RegisterCallback(nullptr);
  m_client->RegisterCallback(NULL);
  m_client->RegisterCallback(0);*/
}

TEST_F(TestClient, RegisterABadCallback)
{
  // The following lambda callback won't even compile
  // So we are guaranteed that we cannot register invalid callbacks
  /*m_client->RegisterCallback<lifx::message::device::EchoRequest>([](){});*/
}

TEST_F(TestClient, RunOnce)
{
  ASSERT_EQ(TestLifxClient::RunResult::RUN_WAITING, m_client->RunOnce());

  m_client->m_errorState = true;
  ASSERT_EQ(TestLifxClient::RunResult::RUN_ERROR, m_client->RunOnce());

  m_client->m_sendsLimited = true;
  ASSERT_EQ(TestLifxClient::RunResult::RUN_SENT_LIMIT, m_client->RunOnce());

  m_client->m_pretendReceive = true;
  ASSERT_EQ(TestLifxClient::RunResult::RUN_RECEIVED_DATA, m_client->RunOnce());

  m_client->Send<lifx::message::device::EchoRequest>();
  ASSERT_EQ(TestLifxClient::RunResult::RUN_SENT_DATA, m_client->RunOnce());
}

TEST_F(TestClient, WaitingToSend)
{
  m_client->Broadcast<lifx::message::device::EchoRequest>();
  ASSERT_EQ(true, m_client->WaitingToSend());
}

TEST_F(TestClient, TryReceiveMessageSuccess)
{
  constexpr uint64_t payload = 123;
  auto gn = m_client->Send<lifx::message::device::EchoRequest>({payload});

  const char* buffer = m_client->GetPendingSendBuffer(gn);
  lifx::Header header = m_client->GetPendingSendHeader(gn);

  std::mutex recvMutex;
  m_client->RegisterCallback<lifx::message::device::EchoRequest>(
    [&recvMutex, &payload]
    (const lifx::Header, const lifx::message::device::EchoRequest& echo)
    {
      recvMutex.unlock();
      ASSERT_EQ(payload, echo.payload);
    });

  recvMutex.lock();
  m_client->TryReceiveMessage<lifx::message::device::EchoRequest>
    (header, buffer);

  recvMutex.lock();
  recvMutex.unlock();
}

TEST_F(TestClient, TryReceiveMessageFailure)
{
  auto gn = m_client->Send<lifx::message::device::EchoRequest>({});

  const char* buffer = m_client->GetPendingSendBuffer(gn);
  lifx::Header header = m_client->GetPendingSendHeader(gn);
  m_client->RegisterCallback<lifx::message::device::EchoRequest>([]
    (const lifx::Header, const lifx::message::device::EchoRequest&)
    {
      ASSERT_TRUE(false);
    });

  m_client->TryReceiveMessage<lifx::message::device::EchoResponse>
    (header, buffer);
}

} // local namespace

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
