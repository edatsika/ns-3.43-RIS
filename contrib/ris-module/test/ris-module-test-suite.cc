#include <cstdlib> // For std::getenv
#include <string>  // For std::stod

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-module.h"
#include "ns3/ris-module.h"

#include "ns3/test.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RisUplinkSimulation");
// Define the number of users and RIS nodes as constants
const uint32_t numUsers = 5;  // Example: 5 users
const uint32_t numRIS = 3;    // Example: 5 RIS nodes
const uint32_t numElements = 32; // Example: 32 elements per RIS

const uint32_t channelWidth = 20;   // Valid ChannelWidth in MHz for 802.11ax
const double totalDuration = 10.0; // Total simulation duration in seconds
const double slotDuration = totalDuration / numUsers; // Each user's time slot

// Bandwidth per RIS in Hz (adjust this value as needed)
const double txPowerDbm = 20; //P_tx in dBm per user
const double kBoltzmann = 1.38e-23; // Boltzmann constant in J/K
const double roomTemperature = 290.0; // Room temperature in Kelvin
double bandwidthPerRIS = 5e6; // Example: 5 MHz per RIS
double noisePowerW = kBoltzmann * roomTemperature * bandwidthPerRIS;


std::vector<double> timeSlots(numUsers, slotDuration); 
// SNR values for each user (to be calculated, initialized here for testing)
std::vector<double> snrValues(numUsers, 0.0); // Initialize to 0, will be updated based on path loss


class RisModuleTestCase1 : public TestCase
{
public:
    RisModuleTestCase1(double txPowerDbm); // Constructor with txPowerDbm
    virtual ~RisModuleTestCase1();

private:
    void DoRun() override;
    double m_txPowerDbm; // Store Tx power
};

// Constructor Implementation
RisModuleTestCase1::RisModuleTestCase1(double txPowerDbm)
    : TestCase("RisModule test case (simulates RIS uplink scenario)"),
      m_txPowerDbm(txPowerDbm) // Initialize member variable
{
}

RisModuleTestCase1::~RisModuleTestCase1()
{
}

void RisModuleTestCase1::DoRun()
{
    NS_LOG_UNCOND("Starting the RIS Uplink Simulation with Throughput Estimation");
    /*uint32_t seed = static_cast<uint32_t>(Simulator::Now().GetSeconds());  // Use time-based seed
    /SeedManager::SetSeed(seed);
    /SeedManager::SetRun(1);  // This can also be varied between runs */
    uint32_t seed = static_cast<uint32_t>(time(0)); // Seed based on system time
    SeedManager::SetSeed(seed);                    
    SeedManager::SetRun(1);                        // Vary between runs for different random streams

    NodeContainer userNodes;
    userNodes.Create(numUsers);

    NodeContainer risNodes;
    risNodes.Create(numRIS);

    NodeContainer bsNode;
    bsNode.Create(1);

    InternetStackHelper internet;
    internet.Install(userNodes);
    internet.Install(bsNode);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));

    // Static users
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); 
    mobility.Install(userNodes);
    mobility.Install(bsNode);

    // Random positions for RIS nodes
    MobilityHelper risMobility;
    Ptr<UniformRandomVariable> xRand = CreateObject<UniformRandomVariable>();
    xRand->SetAttribute("Min", DoubleValue(0.0));
    xRand->SetAttribute("Max", DoubleValue(100.0));
    Ptr<UniformRandomVariable> yRand = CreateObject<UniformRandomVariable>();
    yRand->SetAttribute("Min", DoubleValue(0.0));
    yRand->SetAttribute("Max", DoubleValue(100.0));
    Ptr<UniformRandomVariable> zRand = CreateObject<UniformRandomVariable>();
    zRand->SetAttribute("Min", DoubleValue(0.0));
    zRand->SetAttribute("Max", DoubleValue(10.0));

    // Static RISs
    risMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    risMobility.Install(risNodes);

    for (uint32_t i = 0; i < numRIS; ++i) {
        Ptr<MobilityModel> mobilityModel = risNodes.Get(i)->GetObject<MobilityModel>();
        if (mobilityModel == nullptr) {
            NS_LOG_ERROR("No mobility model assigned to RIS node " << i);
            continue;
        }

        // Random position generation
        Ptr<UniformRandomVariable> randVar = CreateObject<UniformRandomVariable>();
        randVar->SetStream(i);  // Ensure unique stream per RIS node

        double xPos = randVar->GetValue(0.0, 100.0);
        double yPos = randVar->GetValue(0.0, 100.0);
        double zPos = randVar->GetValue(0.0, 10.0);

        mobilityModel->SetPosition(Vector(xPos, yPos, zPos));

        // Log assigned positions
        Vector pos = mobilityModel->GetPosition();
        NS_LOG_UNCOND("RIS " << i << " assigned position: " << pos);
    }


    /*for (uint32_t i = 0; i < numRIS; ++i) {
        Ptr<MobilityModel> mobilityModel = risNodes.Get(i)->GetObject<MobilityModel>();
        if (mobilityModel == nullptr) {
            NS_LOG_ERROR("No mobility model assigned to RIS node " << i);
        } else {
            // Create unique random positions for each RIS
            Ptr<UniformRandomVariable> xRand = CreateObject<UniformRandomVariable>();
            xRand->SetAttribute("Min", DoubleValue(0.0));
            xRand->SetAttribute("Max", DoubleValue(100.0));
            xRand->SetStream(i);  // Ensure unique RNG stream per RIS node
            double xPos = xRand->GetValue();

            Ptr<UniformRandomVariable> yRand = CreateObject<UniformRandomVariable>();
            yRand->SetAttribute("Min", DoubleValue(0.0));
            yRand->SetAttribute("Max", DoubleValue(100.0));
            yRand->SetStream(i);  // Unique stream for Y position
            double yPos = yRand->GetValue();

            Ptr<UniformRandomVariable> zRand = CreateObject<UniformRandomVariable>();
            zRand->SetAttribute("Min", DoubleValue(0.0));
            zRand->SetAttribute("Max", DoubleValue(10.0));
            zRand->SetStream(i);  // Unique stream for Z position
            double zPos = zRand->GetValue();

            mobilityModel->SetPosition(Vector(xPos, yPos, zPos));

            // Log the assigned position
            Vector pos = mobilityModel->GetPosition();
            NS_LOG_UNCOND("RIS " << i << " assigned position: " << pos);
        }
    }*/

    /*
    Ptr<MobilityModel> mobilityModel = risNodes.Get(0)->GetObject<MobilityModel>();
    NS_ASSERT(mobilityModel != nullptr && "RIS node 0 has no mobility model");
    for (uint32_t i = 0; i < risNodes.GetN(); ++i) {
        Ptr<MobilityModel> mobilityModel = risNodes.Get(i)->GetObject<MobilityModel>();
        if (mobilityModel) {
            NS_LOG_UNCOND("RIS node " << i << " has mobility model initialized.");
            mobilityModel->SetPosition(Vector(xRand->GetValue(), yRand->GetValue(), zRand->GetValue()));
            // Log the assigned position
            Vector pos = mobilityModel->GetPosition();
            NS_LOG_UNCOND("RIS " << i << " assigned position: " << pos);
        } else {
            NS_LOG_ERROR("MobilityModel not found for RIS node " << i);
            Simulator::Destroy();
            return;
        }
    }*/


    // User-RIS association according to RIS model
    Ptr<RisPropagationLossModel> risModel = CreateObject<RisPropagationLossModel>();
    if (risModel) {
        NS_LOG_UNCOND("RIS propagation model created successfully.");
    } else {
        NS_LOG_ERROR("RIS propagation model creation failed.");
        Simulator::Destroy();
        return;
    }

    // Associate each user with the RIS providing the highest SNR
    std::map<Ptr<Node>, Ptr<Node>> userToRISMap;
    for (uint32_t i = 0; i < numUsers; ++i) {
        Ptr<Node> user = userNodes.Get(i);

        double bestSnr = -std::numeric_limits<double>::infinity();
        Ptr<Node> bestRis = nullptr;

        for (uint32_t j = 0; j < numRIS; ++j) {
            Ptr<Node> ris = risNodes.Get(j);

            // Calculate SNR for the current user and RIS
            double snrDb = risModel->CalculateSnrWithRis(
                m_txPowerDbm, // Use the instance's Tx power instead of txPowerDbm, 
                user->GetObject<MobilityModel>(), 
                ris->GetObject<MobilityModel>(), 
                numElements, 
                noisePowerW
            );

            NS_LOG_UNCOND("User " << i << ", RIS " << j << ", Tx Power " << txPowerDbm 
                                  << " dBm, SNR: " << snrDb << " dB");

            if (snrDb > bestSnr) {
                bestSnr = snrDb;
                bestRis = ris;
            }
        }
        // Store the RIS with the best SNR for the current user
        userToRISMap[user] = bestRis;
        snrValues[i] = bestSnr; // Store the best SNR value for throughput calculation
    }


    // Print user-RIS mapping
    NS_LOG_UNCOND("User to RIS Mapping:");
    for (uint32_t i = 0; i < numUsers; ++i) {
        Ptr<Node> user = userNodes.Get(i);
        Ptr<Node> assignedRIS = userToRISMap[user];
    
        if (assignedRIS != nullptr) {
            // Find the index of the assigned RIS
            uint32_t risIndex = 0;
            for (uint32_t j = 0; j < numRIS; ++j) {
                if (risNodes.Get(j) == assignedRIS) {
                    risIndex = j;
                    break;
                }
            }
        
            NS_LOG_UNCOND("User " << i << " is served by RIS " << risIndex 
                              << " with SNR: " << snrValues[i] << " dB");
        } else {
            NS_LOG_UNCOND("User " << i << " is not assigned to any RIS.");
        }
    }

    //YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    //channel.AddPropagationLoss("ns3::RisPropagationLossModel");

    /*YansWifiPhyHelper phy = YansWifiPhyHelper();
    phy.SetChannel(CreateObject<YansWifiChannel>());*/
    //Sets up the channel for use in future objects created by the helper.
    //It doesn't create an instance of WifiPhy immediately or associate the channel with anything directly.
    // If you don't use phyHelper.Create<>() to create a WifiPhy object, the actual channel object might never 
    //be associated with a WifiPhy

    // Create the YansWifiChannelHelper
    //YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
    // Create the channel and set it in the YansWifiPhyHelper
    //YansWifiPhyHelper phy = YansWifiPhyHelper();
    //phy.SetChannel(channelHelper.Create());
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper();
    //channel.AddPropagationLoss("ns3::RisPropagationLossModel");
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);
    WifiMacHelper mac;
    Ssid ssid = Ssid("RIS-Network");

    // Users
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer userDevices = wifi.Install(phy, mac, userNodes);

    // BS
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer bsDevice = wifi.Install(phy, mac, bsNode);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer userInterfaces = ipv4.Assign(userDevices);
    Ipv4InterfaceContainer bsInterface = ipv4.Assign(bsDevice);

    // UDP server at BS
    UdpServerHelper udpServer(9); // Port No. 9
    ApplicationContainer serverApp = udpServer.Install(bsNode.Get(0));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(totalDuration));

    // UDP clients are installed on the user nodes to send traffic to the BS
    UdpClientHelper udpClient(bsInterface.GetAddress(0), 9); 
    udpClient.SetAttribute("MaxPackets", UintegerValue(100)); // Max number of packets send by each user
    udpClient.SetAttribute("PacketSize", UintegerValue(1024));

    // Simulate TDMA where each user is served by 1 RIS and uses the total bandwidth
    for (uint32_t i = 0; i < numUsers; ++i) {
        Ptr<Node> user = userNodes.Get(i);
        // Each user transmits during a unique slot using clientApp.Start and clientApp.Stop
        ApplicationContainer clientApp = udpClient.Install(user);
        clientApp.Start(Seconds(i * slotDuration));
        clientApp.Stop(Seconds((i + 1) * slotDuration));
    }


    FlowMonitorHelper flowMonitorHelper;
    Ptr<FlowMonitor> monitor = flowMonitorHelper.InstallAll();

    Simulator::Stop(Seconds(totalDuration));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonitorHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    if (stats.empty())
    {
        NS_LOG_ERROR("No flow stats available, verify traffic transmission.");
    }
    else
    {
        for (const auto &flow : stats)
        {
            Ipv4FlowClassifier::FiveTuple tuple = classifier->FindFlow(flow.first);
            double throughput = (flow.second.rxBytes * 8.0) / 
                                (flow.second.timeLastRxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds()) / 1e6; // Mbps
            NS_LOG_UNCOND("Flow from " << tuple.sourceAddress << " to " << tuple.destinationAddress
                                       << ": Throughput = " << throughput << " Mbps");
        }
    }



    double systemSumRate = 0.0;
    for (uint32_t i = 0; i < numUsers; ++i) {
        double rate = (timeSlots[i] / totalDuration) * bandwidthPerRIS * std::log2(1 + snrValues[i]);
        systemSumRate += rate;
        NS_LOG_UNCOND("Rate for User " << i << ": " << rate << " bps");
    }

    NS_LOG_UNCOND("System Sum Rate: " << systemSumRate / 1e6 << " Mbps");

    NS_LOG_UNCOND("SNR Values:");
    for (uint32_t i = 0; i < numUsers; ++i) {
        NS_LOG_UNCOND("User " << i << " SNR: " << snrValues[i] << " dB");
    }

    Simulator::Destroy();
    NS_LOG_UNCOND("Simulation Completed");
}



class RisModuleTestSuite : public TestSuite
{
public:
    RisModuleTestSuite();
};

RisModuleTestSuite::RisModuleTestSuite()
    : TestSuite("ris-module", Type::UNIT)
{
    double txPowerDbm = 20.0;

    const char* envTxPower = std::getenv("TX_POWER_DBM");
    if (envTxPower != nullptr)
    {
        try
        {
            txPowerDbm = std::stod(envTxPower); // Convert string to double
        }
        catch (const std::exception& e)
        {
            NS_LOG_ERROR("Error parsing TX_POWER_DBM: " << e.what());
        }
    }

    NS_LOG_UNCOND("Using Tx Power: " << txPowerDbm << " dBm from environment variable TX_POWER_DBM");
    AddTestCase(new RisModuleTestCase1(txPowerDbm), Duration::QUICK);
}


static RisModuleTestSuite srisModuleTestSuite;
