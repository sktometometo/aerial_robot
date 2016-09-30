#ifndef OPTICAL_FLOW_MODULE_H
#define OPTICAL_FLOW_MODULE_H

#include <ros/ros.h>
#include <aerial_robot_base/basic_state_estimation.h>
#include <aerial_robot_base/sensor/sensor_base_plugin.h>
#include <aerial_robot_base/States.h>
#include <px_comm/OpticalFlow.h>
#include <std_msgs/Int32.h>

namespace sensor_plugin
{
  class OpticalFlow :public sensor_base_plugin::SensorBase
  {
  public:
      void initialize(ros::NodeHandle nh, ros::NodeHandle nhp, BasicEstimator* estimator)
      {
        nh_ = ros::NodeHandle(nh, "optical_flow");
        nhp_ = ros::NodeHandle(nhp, "optical_flow");
        estimator_ = estimator;
        baseRosParamInit();
        rosParamInit();

        optical_flow_pub_ = nh_.advertise<aerial_robot_base::States>("data",10);
        optical_flow_sub_ = nh_.subscribe<px_comm::OpticalFlow>("opt_flow", 1, &OpticalFlow::opticalFlowCallback, this, ros::TransportHints().tcpNoDelay());

        raw_pos_z_ = 0;
        prev_raw_pos_z_ = 0;
        pos_z_ = 0;
        raw_vel_z_ = 0;
        vel_z_ = 0;

        raw_vel_x_ = 0;
        filtered_vel_x_ = 0;

        raw_vel_y_ = 0;
        filtered_vel_y_ = 0;

        //debug, but can be constant;
        sensor_fusion_flag_ = false; 
        }

    ~OpticalFlow() {}

    OpticalFlow() {}
  private:
    ros::Publisher optical_flow_pub_;
    ros::Subscriber optical_flow_sub_;
    ros::Time optical_flow_stamp_;

    double start_upper_thre_;
    double start_lower_thre_;
    double start_vel_;

    double x_axis_direction_;
    double y_axis_direction_;

    double sonar_noise_sigma_, level_vel_noise_sigma_;
    double z_upper_thre_;

    float raw_pos_z_;
    float prev_raw_pos_z_;
    float pos_z_;
    float raw_vel_z_;
    float vel_z_;

    float raw_vel_x_;
    float filtered_vel_x_;

    float raw_vel_y_;
    float filtered_vel_y_;

    bool sensor_fusion_flag_;

    px_comm::OpticalFlow optical_flow_msg_;

    void opticalFlowCallback(const px_comm::OpticalFlowConstPtr & optical_flow_msg)
    {

      static int init_flag = true;

      static bool stop_flag = true;
      static double previous_secs;
      static double special_increment = 0;
      optical_flow_msg_ = *optical_flow_msg;
      double current_secs = optical_flow_msg_.header.stamp.toSec();

      //**** Global Sensor Fusion Flag Check
      if(!estimator_->getSensorFusionFlag())
        {
          //this is for the repeat mode
          init_flag = true;

          //this is for the halt mode
          for(vector<int>::iterator  it = estimate_indices_.begin(); it != estimate_indices_.end(); ++it )
            {
              estimator_->getFuserEgomotion(*it)->setMeasureFlag(false);
              estimator_->getFuserEgomotion(*it)->resetState();
            }
          for(vector<int>::iterator  it = experiment_indices_.begin(); it != experiment_indices_.end(); ++it )
            {
              estimator_->getFuserExperiment(*it)->setMeasureFlag(false);
              estimator_->getFuserExperiment(*it)->resetState();
            }
          return;
        }

      //**** Optical flow: initialize(enable) the measuring flag
      if(init_flag)
        {
          if(estimate_mode_ & (1 << EGOMOTION_ESTIMATION_MODE))
            {
              for(int i = 0; i < estimator_->getFuserEgomotionNo(); i++)
                {
                  if((estimator_->getFuserEgomotionId(i) & (1 << BasicEstimator::Z_W)))
                    estimator_->getFuserEgomotion(i)->setMeasureFlag();
                }
            }
          if(estimate_mode_ & (1 << EXPERIMENT_MODE))
            {
              for(int i = 0; i < estimator_->getFuserExperimentNo(); i++)
                {
                  if((estimator_->getFuserExperimentId(i) & (1 << BasicEstimator::Z_W)))

                    estimator_->getFuserExperiment(i)->setMeasureFlag();
                }
            }

          /* enable the non-descending mode  */
          estimator_->setUnDescendMode(true);

          init_flag = false;
        }

      /* final landing moment */
      if(estimator_->getLandedFlag())
        {
          for(vector<int>::iterator  it = estimate_indices_.begin(); it != estimate_indices_.end(); ++it )
            {
              estimator_->getFuserEgomotion(*it)->setMeasureFlag(false);
              estimator_->getFuserEgomotion(*it)->resetState();
            }
          for(vector<int>::iterator  it = experiment_indices_.begin(); it != experiment_indices_.end(); ++it )
            {
              estimator_->getFuserExperiment(*it)->setMeasureFlag(false);
              estimator_->getFuserExperiment(*it)->resetState();
            }
        }


      //**** updateh the height
      raw_pos_z_ = optical_flow_msg_.ground_distance;

      //  start sensor fusion condition
      if(!sensor_fusion_flag_)
        {
          if(raw_pos_z_ < start_upper_thre_ && raw_pos_z_ > start_lower_thre_ &&
             prev_raw_pos_z_ < start_lower_thre_ &&
             prev_raw_pos_z_ > (start_lower_thre_ - 0.1) )
            {//pose init
              //TODO: start flag fresh arm, or use air pressure => refined
              ROS_ERROR("optical flow: start sensor fusion, prev_raw_pos_z_ : %f", prev_raw_pos_z_);

              /* release the non-descending mode */
              estimator_->setUnDescendMode(false);

              if(estimate_mode_ & (1 << EGOMOTION_ESTIMATION_MODE))
                {
                  for(int i = 0; i < estimator_->getFuserEgomotionNo(); i++)
                    {
                      Eigen::Matrix<double, 1, 1> temp = Eigen::MatrixXd::Zero(1, 1);
                      if((estimator_->getFuserEgomotionId(i) & (1 << BasicEstimator::X_W)) || (estimator_->getFuserEgomotionId(i) & (1 << BasicEstimator::X_B)))
                        {
                          estimate_indices_.push_back(i);
                          temp(0,0) = level_vel_noise_sigma_;
                          estimator_->getFuserEgomotion(i)->setInitState(x_axis_direction_ * optical_flow_msg_.flow_x /1000.0, 1);
                          estimator_->getFuserEgomotion(i)->setMeasureSigma(temp);
                          estimator_->getFuserEgomotion(i)->setMeasureFlag();
                        }
                      else if((estimator_->getFuserEgomotionId(i) & (1 << BasicEstimator::Y_W)) || (estimator_->getFuserEgomotionId(i) & (1 << BasicEstimator::Y_B)))
                        {
                          estimate_indices_.push_back(i);
                          temp(0,0) = level_vel_noise_sigma_;
                          estimator_->getFuserEgomotion(i)->setInitState(y_axis_direction_ * optical_flow_msg_.flow_y /1000.0 ,1);
                          estimator_->getFuserEgomotion(i)->setMeasureSigma(temp);
                          estimator_->getFuserEgomotion(i)->setMeasureFlag();
                        }
                      else if((estimator_->getFuserEgomotionId(i) & (1 << BasicEstimator::Z_W)))
                        {
                          estimate_indices_.push_back(i);
                          estimator_->getFuserEgomotion(i)->setInitState(optical_flow_msg_.ground_distance, 0);
                          temp(0,0) = sonar_noise_sigma_;
                          estimator_->getFuserEgomotion(i)->setMeasureSigma(temp);
                          estimator_->getFuserEgomotion(i)->setMeasureFlag();
                        }
                    }
                }

              if(estimate_mode_ & (1 << EXPERIMENT_MODE))
                {
                  for(int i = 0; i < estimator_->getFuserExperimentNo(); i++)
                    {
                      Eigen::Matrix<double, 1, 1> temp = Eigen::MatrixXd::Zero(1, 1); 
                      if((estimator_->getFuserExperimentId(i) & (1 << BasicEstimator::X_W)) || (estimator_->getFuserExperimentId(i) & (1 << BasicEstimator::X_B)))
                        {
                          experiment_indices_.push_back(i);
                          temp(0,0) = level_vel_noise_sigma_;
                          estimator_->getFuserExperiment(i)->setMeasureSigma(temp);
                          estimator_->getFuserExperiment(i)->setInitState(x_axis_direction_ * optical_flow_msg_.flow_x /1000.0, 1);
                          estimator_->getFuserExperiment(i)->setMeasureFlag();
                        }
                      else if((estimator_->getFuserExperimentId(i) & (1 << BasicEstimator::Y_W)) || (estimator_->getFuserExperimentId(i) & (1 << BasicEstimator::Y_B)))
                        {
                          experiment_indices_.push_back(i);
                          temp(0,0) = level_vel_noise_sigma_;
                          estimator_->getFuserExperiment(i)->setMeasureSigma(temp);
                          estimator_->getFuserExperiment(i)->setInitState(y_axis_direction_ * optical_flow_msg_.flow_y /1000.0 ,1);
                          estimator_->getFuserExperiment(i)->setMeasureFlag();
                        }
                      else if((estimator_->getFuserExperimentId(i) & (1 << BasicEstimator::Z_W)))
                        {
                          experiment_indices_.push_back(i);
                          estimator_->getFuserExperiment(i)->setInitState(optical_flow_msg_.ground_distance, 0);
                          temp(0,0) = sonar_noise_sigma_;
                          estimator_->getFuserExperiment(i)->setMeasureSigma(temp);
                          estimator_->getFuserExperiment(i)->setMeasureFlag();
                        }
                    }
                }
              sensor_fusion_flag_ = true;
            }
        }
      else
        {
          if(estimator_->getLandingMode() &&
             prev_raw_pos_z_ < start_upper_thre_ &&
             prev_raw_pos_z_ > start_lower_thre_ &&
             raw_pos_z_ < start_lower_thre_ && raw_pos_z_ > (start_lower_thre_ - 0.1))
            {//special process(1) for landing, this is not very good, as far, 1/10 fail
              sensor_fusion_flag_ = false;
              ROS_ERROR("optical flow sensor: stop sensor fusion");
              return;
            }

          //**** 高さ方向情報の更新
          raw_vel_z_ = (raw_pos_z_ - prev_raw_pos_z_) / (current_secs - previous_secs);

          //**** 速度情報の更新,ボードの向き
          raw_vel_x_ = x_axis_direction_ * optical_flow_msg_.velocity_x; 
          raw_vel_y_ = y_axis_direction_ * optical_flow_msg_.velocity_y; 

          filtered_vel_x_ = x_axis_direction_ * optical_flow_msg_.flow_x /1000.0;
          filtered_vel_y_ = y_axis_direction_ * optical_flow_msg_.flow_y /1000.0;

          estimateProcess();

          //publish
          aerial_robot_base::States opt_data;
          opt_data.header.stamp = optical_flow_msg_.header.stamp;

          aerial_robot_base::State x_state;
          x_state.id = "x";
          x_state.raw_vel = raw_vel_x_;
          x_state.vel = filtered_vel_x_;

          aerial_robot_base::State y_state;
          y_state.id = "y";
          y_state.raw_vel = raw_vel_y_;
          y_state.vel = filtered_vel_y_;

          aerial_robot_base::State z_state;
          z_state.id = "z";
          z_state.raw_pos = raw_pos_z_;
          z_state.raw_vel = raw_vel_z_;


          Eigen::Matrix<double,2,1> kf_x_state;
          Eigen::Matrix<double,2,1> kf_y_state;
          Eigen::Matrix<double,2,1> kf_z_state;
          Eigen::Matrix<double,3,1> kfb_x_state;
          Eigen::Matrix<double,3,1> kfb_y_state;
          Eigen::Matrix<double,3,1> kfb_z_state;

          for(vector<int>::iterator  it = estimate_indices_.begin(); it != estimate_indices_.end(); ++it )
            {
              if(estimator_->getFuserEgomotionPluginName(*it) == "kalman_filter/kf_pose_vel_acc_bias")
                {
                  if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::X_B))) 
                    {
                      kfb_x_state = estimator_->getFuserEgomotion(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::X_B, 1, kfb_x_state(1,0));
                      estimator_->setEEState(BasicEstimator::X_B, 0, kfb_x_state(0,0));
                    }
                  if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::X_W))) 
                    {
                      kfb_x_state = estimator_->getFuserEgomotion(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::X_W, 1, kfb_x_state(1,0));
                      estimator_->setEEState(BasicEstimator::X_W, 0, kfb_x_state(0,0));
                    }
                  if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::Y_B))) 
                    {
                      kfb_y_state = estimator_->getFuserEgomotion(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::Y_B, 1, kfb_y_state(1,0));
                      estimator_->setEEState(BasicEstimator::Y_B, 0, kfb_y_state(0,0));
                    }
                  if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::Y_W))) 
                    {
                      kfb_y_state = estimator_->getFuserEgomotion(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::Y_W, 1, kfb_y_state(1,0));
                      estimator_->setEEState(BasicEstimator::Y_W, 0, kfb_y_state(0,0));
                    }
                  if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::Z_W))) 
                    {
                      kfb_z_state = estimator_->getFuserEgomotion(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::Z_W, 0, kfb_z_state(0,0));
                      estimator_->setEEState(BasicEstimator::Z_W, 1, kfb_z_state(1,0));
                    }
                }
              if(estimator_->getFuserEgomotionPluginName(*it) == "kalman_filter/kf_pose_vel_acc")
                {
                  if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::X_B))) 
                    {
                      kf_x_state = estimator_->getFuserEgomotion(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::X_B, 1, kf_x_state(1,0));
                      estimator_->setEEState(BasicEstimator::X_B, 0, kf_x_state(0,0));
                    }
                  if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::X_W))) 
                    {
                      kf_x_state = estimator_->getFuserEgomotion(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::X_W, 1, kf_x_state(1,0));
                      estimator_->setEEState(BasicEstimator::X_W, 0, kf_x_state(0,0));
                    }
                  if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::Y_B))) 
                    {
                      kf_y_state = estimator_->getFuserEgomotion(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::Y_B, 1, kf_y_state(1,0));
                      estimator_->setEEState(BasicEstimator::X_W, 0, kf_x_state(0,0));
                    }
                  if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::Y_W))) 
                    {
                      kf_y_state = estimator_->getFuserEgomotion(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::Y_W, 1, kf_y_state(1,0));
                      estimator_->setEEState(BasicEstimator::Y_W, 0, kf_y_state(0,0));
                    }
                  if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::Z_W))) 
                    {
                      kf_z_state = estimator_->getFuserEgomotion(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::Z_W, 0, kf_z_state(0,0));
                      estimator_->setEEState(BasicEstimator::Z_W, 1, kf_z_state(1,0));
                    }
                }
            }
          for(vector<int>::iterator  it = experiment_indices_.begin(); it != experiment_indices_.end(); ++it )
            {
              if(estimator_->getFuserExperimentPluginName(*it) == "kalman_filter/kf_pose_vel_acc_bias")
                {
                  if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::X_B))) 
                    {
                      kfb_x_state = estimator_->getFuserExperiment(*it)->getEstimateState();
                      estimator_->setEXState(BasicEstimator::X_B, 1, kfb_x_state(1,0));
                    }
                  if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::X_W))) 
                    {
                      kfb_x_state = estimator_->getFuserExperiment(*it)->getEstimateState();
                      estimator_->setEXState(BasicEstimator::X_W, 1, kfb_x_state(1,0));
                    }
                  if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::Y_B))) 
                    {
                      kfb_y_state = estimator_->getFuserExperiment(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::Y_B, 1, kfb_y_state(1,0));
                    }
                  if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::Y_W))) 
                    {
                      kfb_y_state = estimator_->getFuserExperiment(*it)->getEstimateState();
                      estimator_->setEEState(BasicEstimator::Y_W, 1, kfb_y_state(1,0));
                    }
                }
              if(estimator_->getFuserExperimentPluginName(*it) == "kalman_filter/kf_pose_vel_acc")
                {
                  if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::X_B))) 
                    {
                      kf_x_state = estimator_->getFuserExperiment(*it)->getEstimateState();
                      estimator_->setEXState(BasicEstimator::X_B, 1, kf_x_state(1,0));
                    }
                  if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::X_W))) 
                    {
                      kf_x_state = estimator_->getFuserExperiment(*it)->getEstimateState();
                      estimator_->setEXState(BasicEstimator::X_W, 1, kf_x_state(1,0));
                    }
                  if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::Y_B))) 
                    {
                      kf_y_state = estimator_->getFuserExperiment(*it)->getEstimateState();
                      estimator_->setEXState(BasicEstimator::Y_B, 1, kf_y_state(1,0));
                    }
                  if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::Y_W))) 
                    {
                      kf_y_state = estimator_->getFuserExperiment(*it)->getEstimateState();
                      estimator_->setEXState(BasicEstimator::Y_W, 1, kf_y_state(1,0));
                    }
                  if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::Z_W))) 
                    {
                      kf_z_state = estimator_->getFuserExperiment(*it)->getEstimateState();
                      estimator_->setEXState(BasicEstimator::Z_W, 0, kf_z_state(0,0));
                      estimator_->setEXState(BasicEstimator::Z_W, 1, kf_z_state(1,0));
                    }
                }
            }

          x_state.kf_pos = kf_x_state(0, 0);
          x_state.kf_vel = kf_x_state(1, 0);
          x_state.kfb_pos = kfb_x_state(0, 0);
          x_state.kfb_vel = kfb_x_state(1, 0);
          x_state.kfb_bias = kfb_x_state(2, 0);

          y_state.kf_pos = kf_y_state(0, 0);
          y_state.kf_vel = kf_y_state(1, 0);
          y_state.kfb_pos = kfb_y_state(0, 0);
          y_state.kfb_vel = kfb_y_state(1, 0);
          y_state.kfb_bias = kfb_y_state(2, 0);

          z_state.kf_pos = kf_z_state(0, 0);
          z_state.kf_vel = kf_z_state(1, 0);
          z_state.kfb_pos = kfb_z_state(0, 0);
          z_state.kfb_vel = kfb_z_state(1, 0);


          opt_data.states.push_back(x_state);
          opt_data.states.push_back(y_state);
          opt_data.states.push_back(z_state);

          optical_flow_pub_.publish(opt_data);

        }

      //更新
      previous_secs = current_secs;
      prev_raw_pos_z_ = raw_pos_z_;
    }

    void estimateProcess()
    {
      if(!estimate_flag_) return;

      Eigen::Matrix<double, 1, 1> sigma_temp = Eigen::MatrixXd::Zero(1, 1); 
      Eigen::Matrix<double, 1, 1> temp = Eigen::MatrixXd::Zero(1, 1); 

      for(vector<int>::iterator  it = estimate_indices_.begin(); it != estimate_indices_.end(); ++it )
        {
          if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::X_W)) || (estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::X_B)))
            {
              sigma_temp(0,0) = level_vel_noise_sigma_;
              estimator_->getFuserEgomotion(*it)->setMeasureSigma(sigma_temp);
              estimator_->getFuserEgomotion(*it)->setCorrectMode(1);
              if(optical_flow_msg_.quality == 0 || raw_vel_x_ == 0 || raw_pos_z_ > 2.5)
                temp(0, 0) = filtered_vel_x_;
              else
                temp(0, 0) = raw_vel_x_;
              estimator_->getFuserEgomotion(*it)->correction(temp);
            }
          else if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::Y_W)) || (estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::Y_B)))
            {
              sigma_temp(0,0) = level_vel_noise_sigma_;
              estimator_->getFuserEgomotion(*it)->setMeasureSigma(sigma_temp);
              estimator_->getFuserEgomotion(*it)->setCorrectMode(1);

              if(optical_flow_msg_.quality == 0 || raw_vel_y_ == 0 || raw_pos_z_ > 2.5)
                temp(0, 0) = filtered_vel_y_;
              else
                temp(0, 0) = raw_vel_y_;
              estimator_->getFuserEgomotion(*it)->correction(temp);
            }
          else if((estimator_->getFuserEgomotionId(*it) & (1 << BasicEstimator::Z_W)))
            {
              sigma_temp(0,0) = sonar_noise_sigma_;
              estimator_->getFuserEgomotion(*it)->setMeasureSigma(sigma_temp);
              if(raw_pos_z_ != prev_raw_pos_z_ && raw_pos_z_ < z_upper_thre_ && raw_pos_z_ > (start_lower_thre_ + 0.02))
                {//TODO: the condition is too rough
                  temp(0, 0) = raw_pos_z_;
                  estimator_->getFuserEgomotion(*it)->correction(temp);
                }
            }
        }
      for(vector<int>::iterator  it = experiment_indices_.begin(); it != experiment_indices_.end(); ++it )
        {
          if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::X_W)) || (estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::X_B)))
            {
              sigma_temp(0,0) = level_vel_noise_sigma_;
              estimator_->getFuserExperiment(*it)->setMeasureSigma(sigma_temp);
              estimator_->getFuserExperiment(*it)->setCorrectMode(1);

              if(optical_flow_msg_.quality == 0 || raw_vel_x_ == 0 || raw_pos_z_ > 2.5)
                temp(0, 0) = filtered_vel_x_;
              else
                temp(0, 0) = raw_vel_x_;
              estimator_->getFuserExperiment(*it)->correction(temp);
            }
          else if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::Y_W)) || (estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::Y_B)))
            {
              sigma_temp(0,0) = level_vel_noise_sigma_;
              estimator_->getFuserExperiment(*it)->setMeasureSigma(sigma_temp);
              estimator_->getFuserExperiment(*it)->setCorrectMode(1);

              if(optical_flow_msg_.quality == 0 || raw_vel_y_ == 0 || raw_pos_z_ > 2.5)
                temp(0, 0) = filtered_vel_y_;
              else
                temp(0, 0) = raw_vel_y_;
              estimator_->getFuserExperiment(*it)->correction(temp);
            }
          else if((estimator_->getFuserExperimentId(*it) & (1 << BasicEstimator::Z_W)))
            {
              sigma_temp(0,0) = sonar_noise_sigma_;
              estimator_->getFuserExperiment(*it)->setMeasureSigma(sigma_temp);

              if(raw_pos_z_ != prev_raw_pos_z_ && raw_pos_z_ < 2.5) //100Hz
                temp(0, 0) = raw_pos_z_;
              estimator_->getFuserExperiment(*it)->correction(temp);
            }
        }
    }

    void rosParamInit()
    {

      nhp_.param("level_vel_noise_sigma", level_vel_noise_sigma_, 0.01 );
      printf("level vel noise sigma  is %f\n", level_vel_noise_sigma_);

      nhp_.param("sonar_noise_sigma", sonar_noise_sigma_, 0.01 );
      printf("sonar noise sigma  is %f\n", sonar_noise_sigma_);

      if (!nhp_.getParam ("start_upper_thre", start_upper_thre_))
        start_upper_thre_ = 0;
      printf("%s: start_upper_thre_ is %.3f\n", nhp_.getNamespace().c_str(), start_upper_thre_);

      if (!nhp_.getParam ("z_upper_thre", z_upper_thre_))
        z_upper_thre_ = 2.5;
      printf("%s: z_upper_thre_ is %.3f\n", nhp_.getNamespace().c_str(), z_upper_thre_);


      if (!nhp_.getParam ("start_lower_thre", start_lower_thre_))
        start_lower_thre_ = 0;
      printf("%s: start_lower_thre_ is %.3f\n", nhp_.getNamespace().c_str(), start_lower_thre_);
      if (!nhp_.getParam ("start_vel", start_vel_))
        start_vel_ = 0;
      printf("%s: start_vel_ is %.3f\n", nhp_.getNamespace().c_str(), start_vel_);

      std::string ns = nhp_.getNamespace();
      if (!nhp_.getParam ("x_axis_direction", x_axis_direction_))
        x_axis_direction_ = 1.0;
      printf("%s: x_axis direction_ is %.3f\n", ns.c_str(), x_axis_direction_);

      if (!nhp_.getParam ("y_axis_direction", y_axis_direction_))
        y_axis_direction_ = 1.0; //-1 is default
      printf("%s: y_axis direction_ is %.3f\n", ns.c_str(), y_axis_direction_);

    }
  };

};
#endif



