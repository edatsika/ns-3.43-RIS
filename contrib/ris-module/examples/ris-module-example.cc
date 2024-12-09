#include "ns3/core-module.h"
#include "ns3/ris-module-helper.h"

/**
 * \file
 *
 * Explain here what the example does.
 */

using namespace ns3;

int main(int argc, char *argv[])
{
    // Only run the simulation if it's not already running
    bool runTestSimulation = true; // Set this based on a condition or command line argument

    if (runTestSimulation)
    {
        // Run the simulation code here
        Simulator::Run();
        Simulator::Destroy();
    }
    else
    {
        NS_LOG_UNCOND("Simulation is skipped for testing purposes.");
    }

    return 0;
}