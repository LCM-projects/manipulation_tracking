#ifndef ATTACHED_APRILTAG_COST_H
#define ATTACHED_APRILTAG_COST_H

#include <stdexcept>
#include <iostream>
#include "ManipulationTrackerCost.hpp"
#include "drake/multibody/rigid_body_tree.h"
#include <lcm/lcm-cpp.hpp>
#include <memory>
#include <mutex>
#include "yaml-cpp/yaml.h"
#include <bot_lcmgl_client/lcmgl.h>
#include <bot_frames/bot_frames.h>
#include <bot_param/param_client.h>
#include "lcmtypes/bot_core/rigid_transform_t.hpp"


#include <lcmtypes/bot_core/rigid_transform_t.hpp>

class AttachedApriltagCost : public ManipulationTrackerCost {
public:
  AttachedApriltagCost(std::shared_ptr<const RigidBodyTree<double> > robot_, std::shared_ptr<lcm::LCM> lcm_, YAML::Node config);
  ~AttachedApriltagCost() {};

  void initBotConfig(const char* filename);
  int get_trans_with_utime(std::string from_frame, std::string to_frame,
                               long long utime, Eigen::Isometry3d & mat);
  
  bool constructCost(ManipulationTracker * tracker, const Eigen::Matrix<double, Eigen::Dynamic, 1> x_old, Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>& Q, Eigen::Matrix<double, Eigen::Dynamic, 1>& f, double& K);

  void handleTagDetectionMsg(const lcm::ReceiveBuffer* rbuf,
                           const std::string& chan,
                           const bot_core::rigid_transform_t* msg);
  void handleCameraOffsetMsg(const lcm::ReceiveBuffer* rbuf,
                           const std::string& chan,
                           const bot_core::rigid_transform_t* msg);

private:
  std::string robot_name = "";
  struct ApriltagAttachment {
    int body_id;
    int list_id;
    Eigen::Transform<double, 3, Eigen::Isometry> body_transform;
    Eigen::Transform<double, 3, Eigen::Isometry> last_transform;
    double last_received;
    lcm::Subscription * detection_sub;
  };
  std::map<int, ApriltagAttachment> attachedApriltags;
  std::map<std::string, int> channelToApriltagIndex;

  std::string state_channelname = "";
  double localization_var = 0.01;
  double transform_var = 1.0;
  double timeout_time = 0.5;
  bool verbose = false;
  bool verbose_lcmgl = false;
  bool world_frame_ = false;

  bot_lcmgl_t* lcmgl_tag_ = NULL;
  BotParam* botparam_ = NULL;
  BotFrames* botframes_ = NULL;

  std::mutex camera_offset_mutex;
  Eigen::Isometry3d kinect2robot;
  
  std::shared_ptr<lcm::LCM> lcm;
  std::shared_ptr<const RigidBodyTree<double> > robot;
  int nq;

  std::mutex detectionsMutex;

  double lastReceivedTime;
};

#endif
