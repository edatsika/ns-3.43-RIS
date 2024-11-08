/*
 * Copyright (c) 2012 The Georgia Institute of Technology
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Brian Swenson <bswenson3@gatech.edu>
 */

#include "Brite.h"

#include "ns3/applications-module.h"
#include "ns3/brite-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nix-vector-helper.h"
#include "ns3/point-to-point-module.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BriteExample");

int
main(int argc, char* argv[])
{
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_ALL);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_ALL);

    LogComponentEnable("BriteExample", LOG_LEVEL_ALL);

    // BRITE needs a configuration file to build its graph. By default, this
    // example will use the TD_ASBarabasi_RTWaxman.conf file. There are many others
    // which can be found in the BRITE/conf_files directory
    std::string confFile = "src/brite/examples/conf_files/TD_ASBarabasi_RTWaxman.conf";
    bool tracing = false;
    bool nix = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("confFile", "BRITE conf file", confFile);
    cmd.AddValue("tracing", "Enable or disable ascii tracing", tracing);
    cmd.AddValue("nix", "Enable or disable nix-vector routing", nix);

    cmd.Parse(argc, argv);

    nix = false;

    // Invoke the BriteTopologyHelper and pass in a BRITE
    // configuration file and a seed file. This will use
    // BRITE to build a graph from which we can build the ns-3 topology
    BriteTopologyHelper bth(confFile);
    bth.AssignStreams(3);

    PointToPointHelper p2p;

    InternetStackHelper stack;

    if (nix)
    {
        Ipv4NixVectorHelper nixRouting;
        stack.SetRoutingHelper(nixRouting);
    }

    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.252");

    bth.BuildBriteTopology(stack);
    bth.AssignIpv4Addresses(address);

    NS_LOG_INFO("Number of AS created " << bth.GetNAs());

    // The BRITE topology generator generates a topology of routers.  Here we create
    // two subnetworks which we attach to router leaf nodes generated by BRITE
    // Any NS3 topology may be used to attach to the BRITE leaf nodes but here we
    // use just one node

    NodeContainer client;
    NodeContainer server;

    client.Create(1);
    stack.Install(client);

    // install client node on last leaf node of AS 0
    int numLeafNodesInAsZero = bth.GetNLeafNodesForAs(0);
    client.Add(bth.GetLeafNodeForAs(0, numLeafNodesInAsZero - 1));

    server.Create(1);
    stack.Install(server);

    // install server node on last leaf node on AS 1
    int numLeafNodesInAsOne = bth.GetNLeafNodesForAs(1);
    server.Add(bth.GetLeafNodeForAs(1, numLeafNodesInAsOne - 1));

    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pClientDevices;
    NetDeviceContainer p2pServerDevices;

    p2pClientDevices = p2p.Install(client);
    p2pServerDevices = p2p.Install(server);

    address.SetBase("10.1.0.0", "255.255.0.0");
    Ipv4InterfaceContainer clientInterfaces;
    clientInterfaces = address.Assign(p2pClientDevices);

    address.SetBase("10.2.0.0", "255.255.0.0");
    Ipv4InterfaceContainer serverInterfaces;
    serverInterfaces = address.Assign(p2pServerDevices);

    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(server.Get(0));
    serverApps.Start(Seconds(1));
    serverApps.Stop(Seconds(5));

    UdpEchoClientHelper echoClient(serverInterfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(client.Get(0));
    clientApps.Start(Seconds(2));
    clientApps.Stop(Seconds(5));

    if (!nix)
    {
        Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    }

    if (tracing)
    {
        AsciiTraceHelper ascii;
        p2p.EnableAsciiAll(ascii.CreateFileStream("briteLeaves.tr"));
    }
    // Run the simulator
    Simulator::Stop(Seconds(6));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
