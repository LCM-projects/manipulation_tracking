#ifndef MANIPULATION_TRACKER_COST_H
#define MANIPULATION_TRACKER_COST_H

#include <Eigen/Dense>

class ManipulationTrackerCost {
  public:
    // handed references to Q, f matrices for the cost function to construct
    // returns a bool, which indicates whether this cost should be used this cycle
    virtual bool constructCost(Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>& Q, Eigen::Matrix<double, Eigen::Dynamic, 1>& f, double& K) { return false; }
};

#endif