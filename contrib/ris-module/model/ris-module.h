#ifndef RIS_MODULE_H
#define RIS_MODULE_H

#include "ns3/propagation-loss-model.h"
#include "ns3/mobility-model.h"

namespace ns3 {

class RisPropagationLossModel : public PropagationLossModel {
public:
    static TypeId GetTypeId(void);
    virtual int64_t DoAssignStreams(int64_t stream);
    //virtual double DoCalcRxPower(double txPowerDbm, Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility) const;
    double CalculatePathLoss(Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility) const;
    virtual double DoCalcRxPower(double txPowerDbm, Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility) const override;
    double CalculateSnrWithRis(double txPowerDbm, Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility, size_t numElements, double noisePowerW) const;


//protected:
    //double CalculatePathLoss(Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility) const;
    //virtual double DoCalcRxPower(double txPowerDbm, Ptr<MobilityModel> senderMobility, Ptr<MobilityModel> receiverMobility) const;
};

} // namespace ns3

#endif // RIS_PROPAGATION_LOSS_MODEL_H

