
#include "ManipulationTracker.hpp"
#include "costs/RobotStateCost.hpp"
#include "costs/JointStateCost.hpp"
#include "costs/KinectFrameCost.hpp"
#include "costs/DynamicsCost.hpp"
#include "costs/GelsightCost.hpp"
#include "costs/AttachedApriltagCost.hpp"
#include "costs/OptotrakMarkerCost.hpp"
#include "costs/NonpenetratingObjectCost.hpp"
#include "yaml-cpp/yaml.h"
#include "common/common.hpp"
#include "unistd.h"

using namespace std;
using namespace Eigen;

int main(int argc, char** argv) {
  char * drc_path = std::getenv("DRC_BASE");
  if (!drc_path) {
    printf("Environment variable DRC_BASE is not set -- assuming global paths\n");
    drc_path = "";
  }

  if (argc != 2){
    printf("Use: runManipulationTrackerIRB140 <path to yaml config file>\n");
    return 0;
  }


  std::shared_ptr<lcm::LCM> lcm(new lcm::LCM());
  if (!lcm->good()) {
    throw std::runtime_error("LCM is not good");
  }

  string configFile(argv[1]);
  YAML::Node config = YAML::LoadFile(configFile);

  VectorXd x0_robot;
  std::shared_ptr<const RigidBodyTree<double>> robot = setupRobotFromConfig(config, x0_robot, string(drc_path), true, false);

  // initialize tracker itself
  ManipulationTracker estimator(robot, x0_robot, lcm, config, true);

  // and register all of the costs that we know how to handle
  if (config["costs"]){
    for (auto iter = config["costs"].begin(); iter != config["costs"].end(); iter++){
      std::string cost_type = (*iter)["type"].as<string>();
      if (cost_type == "RobotStateCost"){
        std::shared_ptr<RobotStateCost> cost(new RobotStateCost(robot, lcm, *iter));
        estimator.addCost(dynamic_pointer_cast<ManipulationTrackerCost, RobotStateCost>(cost));
      } else if (cost_type == "KinectFrameCost") {
        // demands a modifiable copy of the robot to do collision calls
        std::shared_ptr<KinectFrameCost> cost(new KinectFrameCost(setupRobotFromConfig(config, x0_robot, string(drc_path), true, false), lcm, *iter));
        estimator.addCost(dynamic_pointer_cast<ManipulationTrackerCost, KinectFrameCost>(cost));
      } else if (cost_type == "DynamicsCost") { 
        std::shared_ptr<DynamicsCost> cost(new DynamicsCost(robot, lcm, *iter));
        estimator.addCost(dynamic_pointer_cast<ManipulationTrackerCost, DynamicsCost>(cost));
      } else if (cost_type == "JointStateCost") { 
        std::shared_ptr<JointStateCost> cost(new JointStateCost(robot, lcm, *iter));
        estimator.addCost(dynamic_pointer_cast<ManipulationTrackerCost, JointStateCost>(cost));
      } else if (cost_type == "GelsightCost") { 
        // demands a modifiable copy of the robot
        std::shared_ptr<GelsightCost> cost(new GelsightCost(setupRobotFromConfig(config, x0_robot, string(drc_path), true, true), lcm, *iter));
        estimator.addCost(dynamic_pointer_cast<ManipulationTrackerCost, GelsightCost>(cost));
      } else if (cost_type == "AttachedApriltagCost") { 
        std::shared_ptr<AttachedApriltagCost> cost(new AttachedApriltagCost(robot, lcm, *iter));
        estimator.addCost(dynamic_pointer_cast<ManipulationTrackerCost, AttachedApriltagCost>(cost));
      } else if (cost_type == "OptotrakMarkerCost") { 
        std::shared_ptr<OptotrakMarkerCost> cost(new OptotrakMarkerCost(robot, lcm, *iter));
        estimator.addCost(dynamic_pointer_cast<ManipulationTrackerCost, OptotrakMarkerCost>(cost));
      } else if (cost_type == "NonpenetratingObjectCost") {
        // demands a modifiable copy of the robot to do collision calls
        // also requires a list of all other robots in the scene, minus those excluded in "penetrable"
        std::string object_str = (*iter)["nonpenetrating_robot"].as<std::string>();

        std::vector<int> collider_index_correspondences;
        std::vector<std::string> non_colliders = (*iter)["penetrable_robots"].as<std::vector<std::string>>(); // confirmed: this clones rep, can mutate new var without mutating parsed YAML
        non_colliders.push_back(object_str);

        std::vector<int> object_index_correspondences;
        std::vector<std::string> object_vec;
        object_vec.push_back(object_str);
        
        std::shared_ptr<NonpenetratingObjectCost> cost(new NonpenetratingObjectCost(
          setupRobotFromConfigSubset(config, x0_robot, string(drc_path), true, false, true, non_colliders, collider_index_correspondences), collider_index_correspondences,
          setupRobotFromConfigSubset(config, x0_robot, string(drc_path), true, false, false, object_vec, object_index_correspondences), object_index_correspondences,
          lcm, *iter));
        estimator.addCost(dynamic_pointer_cast<ManipulationTrackerCost, NonpenetratingObjectCost>(cost));
      } else {
        cout << "Got cost type " << cost_type << " but I don't know what to do with it!" << endl;
      }
    }
  }

  std::cout << "Manipulation Tracker Listening" << std::endl;

  double last_update_time = getUnixTime();
  double timestep = 0.01;
  if (config["timestep"])
    timestep = config["timestep"].as<double>();

  while(1){
    for (int i=0; i < 100; i++)
      lcm->handleTimeout(0);

    double dt = getUnixTime() - last_update_time;
    if (dt > timestep){
      last_update_time = getUnixTime();
      estimator.update();
      estimator.publish();
    } else {
      usleep(1000);
    }
  }

  return 0;
}
