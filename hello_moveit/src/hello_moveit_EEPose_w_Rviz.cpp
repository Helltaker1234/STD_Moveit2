#include <memory>
// C++ 헤더

#include <thread>
// 

#include <rclcpp/rclcpp.hpp>
// ROS 헤더

// #include <moveit/move_group_interface/move_group_interface.hpp>
#include "moveit/move_group_interface/move_group_interface.h"
// Moveit 헤더

// #include <moveit_visual_tools/moveit_visual_tools.h>
#include "moveit_visual_tools/moveit_visual_tools.h"
// Visual Tools 헤더



int main(int argc, char * argv[])
{

  rclcpp::init(argc, argv);

  auto const node = std::make_shared<rclcpp::Node>(
    "hello_moveit",
    rclcpp::NodeOptions().automatically_declare_parameters_from_overrides(true)
  );

  auto const logger = rclcpp::get_logger("hello_moveit");


////////////////////////////////////////////////////////////////// ^^^ 기본 셋업
////////////////////////////////////////////////////////////////// 로봇 파라미터를 받아올 수 있는 Moveit2 전용 노드를 생성함.


  // [Spin up a SingleThreadedExecutor for MoveItVisualTools to interact with ROS]
  //
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);
  auto spinner = std::thread([&executor]() { executor.spin(); });



  using moveit::planning_interface::MoveGroupInterface;

  auto move_group_interface = MoveGroupInterface(node, "areum2_arm_l"/*"manipulator"*/);



  // [Construct and initialize MoveItVisualTools]
  //
  auto moveit_visual_tools = moveit_visual_tools::MoveItVisualTools
  { 
    node, 
    "base_link", 
    rviz_visual_tools::RVIZ_MARKER_TOPIC, 
    move_group_interface.getRobotModel()
  };

  // [기존의 Marker 흔적들은 지움]
  moveit_visual_tools.deleteAllMarkers();

  // [이거는 단순히 Rviz 와 통신하기 위한 RemoteController 임]
  moveit_visual_tools.loadRemoteControl();





  // [Create closures for visualization]
  //    lambda 함수 기반.
  //    추후 이들을 모두 함수화 할 것임.
  //    함수화하면 개별로 입출력 테스트 및 알아보기 쉽다는 장점이 있다.
  
  // - Rviz 상의 로봇의 위에다가 Text Label 을 띄움
  auto const draw_title = 
  [&moveit_visual_tools]
  (auto text) 
  {

    // lambda 안에 lambda
    auto const text_pose = 
    [] 
    {
      auto msg = Eigen::Isometry3d::Identity();
      msg.translation().z() = 1.0;  // Place text 1m above the base link
      return msg;
    }
    ();

    moveit_visual_tools.publishText(text_pose, text, rviz_visual_tools::WHITE, rviz_visual_tools::XLARGE);
  };

  // - 사용자가 Rviz 상에서 next 버튼을 누르기 전까지 프로세스를 Block 
  // - 디버깅 용
  auto const prompt = 
  [&moveit_visual_tools]
  (auto text) 
  {
    moveit_visual_tools.prompt(text);
  };

  // - tool path of a trajectory 를 그려줌.
  auto const draw_trajectory_tool_path = 
  [&moveit_visual_tools, jmg = move_group_interface.getRobotModel()->getJointModelGroup("areum2_arm_l")]
  (auto const trajectory) 
  {
    moveit_visual_tools.publishTrajectoryLine(trajectory, jmg);
  };



  auto const target_pose = 
  []
  {
    geometry_msgs::msg::Pose msg;
    msg.orientation.w = 1.0;
    msg.position.x = 0.2;
    msg.position.y = 0.2;
    msg.position.z = 0.4;
    return msg;
  }
  ();
  move_group_interface.setPoseTarget(target_pose);



  // [prompt 를 통해 사용자의 응답 대기]
  prompt("Press 'Next' in the RvizVisualToolsGui window to plan");
  draw_title("Planning");
  moveit_visual_tools.trigger();


  auto const [success, plan] = 
  [&move_group_interface]
  {
    moveit::planning_interface::MoveGroupInterface::Plan msg;
    auto const ok = static_cast<bool>(move_group_interface.plan(msg));
    return std::make_pair(ok, msg);
  }
  ();

  if(success) {

    draw_trajectory_tool_path(plan.trajectory_);
    moveit_visual_tools.trigger();
    prompt("Press 'Next' in the RvizVisualToolsGui window to execute");
    draw_title("Executing");
    moveit_visual_tools.trigger();

    move_group_interface.execute(plan);
  } else {

    draw_title("Planning Failed!");
    moveit_visual_tools.trigger();

    RCLCPP_ERROR(logger, "Planning failed!");

  }  


////////////////////////////////////////////////////////////////// ^^^ 정해진 EE Pose 로 이동. 
////////////////////////////////////////////////////////////////// 


  // Shutdown ROS
  rclcpp::shutdown();  // <--- This will cause the spin function in the thread to return
  spinner.join();  // <--- Join the thread before exiting
  return 0;
}