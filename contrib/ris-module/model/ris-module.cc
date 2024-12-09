#include <cmath>
#include <complex>
#include <vector>
#include "ris-module.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("RisPropagationLossModel");
NS_OBJECT_ENSURE_REGISTERED(RisPropagationLossModel);

TypeId RisPropagationLossModel::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::RisPropagationLossModel")
        .SetParent<PropagationLossModel>()
        .AddConstructor<RisPropagationLossModel>();
    return tid;
}

// Long-distance path loss model P0​+10⋅n⋅log10​(d/d0​)
double RisPropagationLossModel::CalculatePathLoss(Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility) const
{
    Vector senderPos = senderMobility->GetPosition();
    Vector receiverPos = receiverMobility->GetPosition();

    double distance = std::sqrt(std::pow(receiverPos.x - senderPos.x, 2) +
                                std::pow(receiverPos.y - senderPos.y, 2) +
                                std::pow(receiverPos.z - senderPos.z, 2));

    if (distance <= 0.0) {
        NS_LOG_WARN("Distance is zero or too small, returning maximum path loss.");
        return std::numeric_limits<double>::max();
    }

    double d0 = 1.0; // Reference distance in meters
    double pathLossExponent = 2.7; // Example value, adjust based on environment
    double pathLossDbAtD0 = 40.0; // Example reference loss in dB

    double pathLossDb = pathLossDbAtD0 + 10 * pathLossExponent * std::log10(distance / d0);

    NS_LOG_DEBUG("Distance: " << distance << " meters, Path Loss: " << pathLossDb << " dB");

    return pathLossDb;
}


// Simple path loss model
/*double RisPropagationLossModel::DoCalcRxPower(double txPowerDbm, Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility) const {
    // Calculate path loss based on existing logic
    double pathLossDb = CalculatePathLoss(senderMobility, receiverMobility);

    // Convert transmitted power to linear scale (watts)
    double txPowerW = std::pow(10, txPowerDbm / 10.0);

    // Calculate received power in watts
    double rxPowerW = txPowerW / std::pow(10, pathLossDb / 10.0);

    // Add SNR calculation: assume a noise power in watts
    double noisePowerW = 1e-9; // Example noise power, adjust as needed
    double snr = rxPowerW / noisePowerW;

    // Convert SNR to dB
    double snrDb = 10 * std::log10(snr);

    NS_LOG_DEBUG("Tx Power: " << txPowerDbm << " dBm, Path Loss: " << pathLossDb << " dB, Rx Power: " << 10 * std::log10(rxPowerW) << " dBm, SNR: " << snrDb << " dB");

    return snrDb; // Return SNR in dB instead of received power
}*/

/*double RisPropagationLossModel::CalculateSnrWithRis(double txPowerDbm, Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility, size_t numElements, double noisePowerW) const {
    // Default path loss calculation
    double pathLossDb = CalculatePathLoss(senderMobility, receiverMobility);

    // Convert tx power to linear scale
    double txPowerW = std::pow(10, txPowerDbm / 10.0);

    // Calculate RIS-enhanced channel gain (example, customize as needed)
    double channelGain = numElements * std::pow(10, -pathLossDb / 20.0); // Simplified
    double absChannelGainSq = channelGain * channelGain;

    // Calculate SNR
    double snr = (txPowerW * absChannelGainSq) / noisePowerW;

    // Convert SNR to dB
    double snrDb = 10 * std::log10(snr);

    NS_LOG_DEBUG("Tx Power: " << txPowerDbm << " dBm, Path Loss: " << pathLossDb
                  << " dB, Channel Gain: " << channelGain << ", Noise Power: "
                  << noisePowerW << " W, SNR: " << snrDb << " dB");

    return snrDb; // Return SNR in dB
}*/

double RisPropagationLossModel::CalculateSnrWithRis(double txPowerDbm, Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility, size_t numElements, double noisePowerW) const {
    // Create vectors for h_km (1xN), g_km (Nx1), and theta_km (Nx1)
    std::vector<std::complex<double>> h_km(numElements);
    std::vector<std::complex<double>> g_km(numElements);
    std::vector<std::complex<double>> theta_km(numElements);

    // Populate vectors with random complex numbers (example: real and imaginary parts in [0, 1])
    for (size_t i = 0; i < numElements; ++i) {
        h_km[i] = {static_cast<double>(rand()) / RAND_MAX, static_cast<double>(rand()) / RAND_MAX};
        g_km[i] = {static_cast<double>(rand()) / RAND_MAX, static_cast<double>(rand()) / RAND_MAX};
        theta_km[i] = {static_cast<double>(rand()) / RAND_MAX, static_cast<double>(rand()) / RAND_MAX};
    }

    // Calculate the equivalent channel gain | h_km * theta_km * g_km |^2
    std::complex<double> channelGain = 0.0;
    for (size_t i = 0; i < numElements; ++i) {
        channelGain += h_km[i] * theta_km[i] * g_km[i];
    }
    double absChannelGainSq = std::norm(channelGain);

    // Convert transmitted power from dBm to linear scale (watts)
    double txPowerW = std::pow(10, txPowerDbm / 10.0);

    // Calculate SNR
    double snr = (txPowerW * absChannelGainSq) / noisePowerW;

    // Convert SNR to dB
    double snrDb = 10 * std::log10(snr);

    NS_LOG_DEBUG("Tx Power: " << txPowerDbm << " dBm, Noise Power: " << noisePowerW
                  << " W, |h_km * theta_km * g_km|^2: " << absChannelGainSq
                  << ", SNR: " << snrDb << " dB");

    return snrDb;
}


double RisPropagationLossModel::DoCalcRxPower(double txPowerDbm, Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility) const {
    // Default path loss calculation
    double pathLossDb = CalculatePathLoss(senderMobility, receiverMobility);

    // Convert transmitted power to linear scale
    double txPowerW = std::pow(10, txPowerDbm / 10.0);

    // Calculate received power in linear scale (watts)
    double rxPowerW = txPowerW / std::pow(10, pathLossDb / 10.0);

    // Convert received power to dBm
    double rxPowerDbm = 10 * std::log10(rxPowerW);

    NS_LOG_DEBUG("Tx Power: " << txPowerDbm << " dBm, Path Loss: " << pathLossDb
                  << " dB, Rx Power: " << rxPowerDbm << " dBm");

    return rxPowerDbm; // Default received power
}




int64_t RisPropagationLossModel::DoAssignStreams(int64_t stream) {
    return 0;
}

} // namespace ns3
