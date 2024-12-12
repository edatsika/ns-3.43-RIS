#include <cstdlib> // For std::getenv
#include <string>  // For std::stod
#include <fstream>
#include <sstream>
#include <unordered_map>


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


class RisModuleTestCase1 : public TestCase
{
public:
    RisModuleTestCase1(); 
    virtual ~RisModuleTestCase1();

private:
    void DoRun() override;
};

// Constructor Implementation
RisModuleTestCase1::RisModuleTestCase1()
    : TestCase("RisModule test case (simulates RIS uplink scenario)")
{
    
}

RisModuleTestCase1::~RisModuleTestCase1()
{
}

// Utility function to read key-value pairs from a file
std::unordered_map<std::string, std::string> ReadConfigFile(const std::string& filename) {
    std::unordered_map<std::string, std::string> config;
    std::ifstream file(filename);

    // Verify file access
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open configuration file: " << filename << std::endl;
        return config; // Return an empty map
    }

    std::cout << "Successfully opened configuration file: " << filename << std::endl;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            config[key] = value;
            std::cout << "Read configuration: " << key << " = " << value << std::endl; // Log each entry
        }
    }

    file.close();
    return config;
}

void RisModuleTestCase1::DoRun()
{
    NS_LOG_COMPONENT_DEFINE("RisUplinkSimulation");
    NS_LOG_UNCOND("Starting the RIS Uplink Simulation with Throughput Estimation");
    //std::string filename = "config.txt";
    std::string filename = "./contrib/ris-module/test/config.txt"; 
    std::unordered_map<std::string, std::string> config = ReadConfigFile(filename);
    // use default values below if not set in the file

    // NS-3 parameters
    uint32_t maxPackets = config.count("MAX_PACKETS") ? std::stoi(config["MAX_PACKETS"]) : 1000;
    uint32_t packetSize = config.count("PACKET_SIZE") ? std::stoi(config["PACKET_SIZE"]) : 1024;
    uint32_t channelWidth = config.count("CHANNEL_WIDTH") ? std::stoi(config["CHANNEL_WIDTH"]) : 20; // Valid ChannelWidth in MHz for 802.11ax
    double interval = config.count("INTERVAL") ? std::stod(config["INTERVAL"]) : 0.01;
    double totalDuration = config.count("TOTAL_DURATION") ? std::stod(config["TOTAL_DURATION"]) : 10.0;

    // UL scenario parameters
    uint32_t numUsers = config.count("NUM_USERS") ? std::stoi(config["NUM_USERS"]) : 5;
    uint32_t numRIS = config.count("NUM_RIS") ? std::stoi(config["NUM_RIS"]) : 3;
    uint32_t numElements = config.count("NUM_ELEMENTS") ? std::stoi(config["NUM_ELEMENTS"]) : 32; // Number of elements per RIS
    double txPowerDbm = config.count("TX_POWER_DBM") ? std::stod(config["TX_POWER_DBM"]) : 20;

    double slotDuration = totalDuration / numUsers; // Each user's time slot

    // Bandwidth per RIS in Hz (adjust this value as needed)
    //const double txPowerDbm = 20; //P_tx in dBm per user
    const double kBoltzmann = 1.38e-23; // Boltzmann constant in J/K
    const double roomTemperature = 290.0; // Room temperature in Kelvin
    double bandwidthPerRIS = channelWidth*1e6; // Example: 20 MHz per RIS
    double noisePowerW = kBoltzmann * roomTemperature * bandwidthPerRIS;


    std::vector<double> timeSlots(numUsers, slotDuration); 
    // SNR values for each user (to be calculated, initialized here for testing)
    std::vector<double> snrValues(numUsers, 0.0); // Initialize to 0, will be updated based on path loss


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
                txPowerDbm, // Use the instance's Tx power instead of txPowerDbm, 
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

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper();
    phy.Set("TxPowerStart", DoubleValue(txPowerDbm)); // Minimum transmission power in dBm
    phy.Set("TxPowerEnd", DoubleValue(txPowerDbm));   // Maximum transmission power in dBm
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
    udpClient.SetAttribute("MaxPackets", UintegerValue(maxPackets));
    udpClient.SetAttribute("PacketSize", UintegerValue(packetSize));
    udpClient.SetAttribute("Interval", TimeValue(Seconds(interval)));
    /*Traffic rate = (PacketSize * 8) / Interval
             = (1024 bytes * 8 bits/byte) / 0.1 seconds
             = 81,920 bps or ~0.08192 Mbps*/

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
    NS_LOG_UNCOND("P_TX: " << txPowerDbm << " dBm");

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
    AddTestCase(new RisModuleTestCase1(), Duration::QUICK);
}


static RisModuleTestSuite srisModuleTestSuite;
