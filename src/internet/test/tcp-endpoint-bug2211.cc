/*
 * Copyright (c) 2015 Alexander Krotov <ilabdsf@yandex.ru>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/test.h"

#include <iostream>

using namespace ns3;

/**
 * @ingroup internet-test
 *
 * @brief Test for bug 2211.
 *
 * https://www.nsnam.org/bugzilla/show_bug.cgi?id=2211
 *
 * NOTE: It is a valgrind testcase, it contains no ASSERTs.
 *
 * Test creates one node and sets up two TCP sockets on the loopback
 * with default parameters: CWND = 1, MTU = 536.
 * Sender socket sends 3 segments.
 * When first segment is acknowledged, cwnd is raised to 2.
 * Then, two segments are sent and arrive into receive buffer.
 * Until bugfix, the following happened:
 * Ipv4EndPoint::ForwardUp was called for both second and third segment.
 * These calls scheduled two Ipv4EndPoint::DoForwardUp events.
 * To demonstrate the bug, test case closes the receiver socket after
 * receiving the second segment. As a result, Ipv4EndPoint is destroyed.
 * However, Ipv4EndPoint::DoForwardUp is already scheduled for third segment.
 * It is a use-after-free bug.
 */
class TcpEndPointBug2211Test : public TestCase
{
  public:
    /**
     * Constructor.
     * @param desc Test description.
     * @param ipVersion True to use IPv6.
     */
    TcpEndPointBug2211Test(std::string desc, bool ipVersion);

    /**
     * @brief Receive a packet.
     * @param socket The receiving socket.
     */
    void Recv(Ptr<Socket> socket);
    /**
     * @brief Handle an incoming connection.
     * @param s The receiving socket.
     * @param from The other node IP address.
     */
    void HandleAccept(Ptr<Socket> s, const Address& from);
    /**
     * @brief Handle a connection establishment.
     * @param socket The receiving socket.
     */
    void HandleConnect(Ptr<Socket> socket);
    void DoRun() override;

  private:
    bool m_v6; //!< True to use IPv6.
};

void
TcpEndPointBug2211Test::Recv(Ptr<Socket> socket)
{
    if (socket->GetRxAvailable() == 536 * 2)
    {
        socket->Close();
    }
}

void
TcpEndPointBug2211Test::HandleAccept(Ptr<Socket> s, const Address& from)
{
    s->SetRecvCallback(MakeCallback(&TcpEndPointBug2211Test::Recv, this));
}

void
TcpEndPointBug2211Test::HandleConnect(Ptr<Socket> socket)
{
    socket->Send(Create<Packet>(536));
    socket->Send(Create<Packet>(536));
    socket->Send(Create<Packet>(536));
    socket->Close();
}

TcpEndPointBug2211Test::TcpEndPointBug2211Test(std::string desc, bool ipVersion)
    : TestCase(desc)
{
    m_v6 = ipVersion;
}

void
TcpEndPointBug2211Test::DoRun()
{
    Ptr<Node> node = CreateObject<Node>();

    InternetStackHelper internet;
    internet.Install(node);

    TypeId tid = TcpSocketFactory::GetTypeId();
    Ptr<Socket> sink = Socket::CreateSocket(node, tid);
    if (!m_v6)
    {
        sink->Bind(InetSocketAddress(Ipv4Address::GetAny(), 9));
    }
    else
    {
        sink->Bind(Inet6SocketAddress(Ipv6Address::GetAny(), 9));
    }
    sink->Listen();
    sink->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                            MakeCallback(&TcpEndPointBug2211Test::HandleAccept, this));

    Ptr<Socket> source = Socket::CreateSocket(node, tid);
    source->Bind();
    source->SetConnectCallback(MakeCallback(&TcpEndPointBug2211Test::HandleConnect, this),
                               MakeNullCallback<void, Ptr<Socket>>());
    if (!m_v6)
    {
        source->Connect(InetSocketAddress(Ipv4Address::GetLoopback(), 9));
    }
    else
    {
        source->Connect(Inet6SocketAddress(Ipv6Address::GetLoopback(), 9));
    }

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief TestSuite for bug 2211 - It must be used with valgrind.
 */
class TcpEndpointBug2211TestSuite : public TestSuite
{
  public:
    TcpEndpointBug2211TestSuite()
        : TestSuite("tcp-endpoint-bug2211-test", Type::UNIT)
    {
        AddTestCase(new TcpEndPointBug2211Test("Bug 2211 testcase IPv4", false),
                    TestCase::Duration::QUICK);
        AddTestCase(new TcpEndPointBug2211Test("Bug 2211 testcase IPv6", true),
                    TestCase::Duration::QUICK);
    }
};

static TcpEndpointBug2211TestSuite
    g_TcpEndPoint2211TestSuite; //!< Static variable for test initialization
