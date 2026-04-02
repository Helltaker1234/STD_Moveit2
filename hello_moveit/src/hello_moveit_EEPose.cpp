#include <memory>
// C++ 헤더

#include <rclcpp/rclcpp.hpp>
// ROS 헤더

// #include <moveit/move_group_interface/move_group_interface.hpp>
#include "moveit/move_group_interface/move_group_interface.h"
// Moveit 헤더



int main(int argc, char * argv[])
{
  // Initialize ROS and create the Node
  //
  // ros2 시스템 시작.
  // 이 함수가 호출되어야 ROS 통신이 가능.
  rclcpp::init(argc, argv);



  // ROS2 노드 자료형을 가진 (스마트)포인터 변수를, auto 자료형(컴파일러가 알아서 자료형 지정) node에 대입.
  // 
  // 
  // automatically_declare_parameters_from_overrides
  // 이 옵션을 true로 켜야만, 런치(Launch) 파일에서 넘겨주는 로봇 설정 파라미터들을 
  // 이 노드가 자동으로 자신의 파라미터로 선언하고 받아들일 수 있음.
  //
  // 그러므로 이 노드가 실행되기 전, move_group 노드가 이미 있어야 한다.
  // 그래야 robot_description 토픽을 move_group 노드로 부터 받을 수 있다.
  auto const node = std::make_shared<rclcpp::Node>(
    "hello_moveit",
    rclcpp::NodeOptions().automatically_declare_parameters_from_overrides(true)
  );


  // Create a ROS logger
  auto const logger = rclcpp::get_logger("hello_moveit");


////////////////////////////////////////////////////////////////// ^^^ 기본 셋업
////////////////////////////////////////////////////////////////// 로봇 파라미터를 받아올 수 있는 Moveit2 전용 노드를 생성함.


  // Create the MoveIt MoveGroup Interface
  // 컴파일러에게 "이제 이 다음 부터 MoveGroupInterface 로 쓴건 다
  // moveit::planning_interface::MoveGroupInterface 소속이다잉" 라고 말해주는 것.
  using moveit::planning_interface::MoveGroupInterface;


  // [현재 생성한 노드에 "Action Client인" MoveGroupInterface 생성]
  // 
  // {move_group_interface} <--Action--> {move_group}
  //
  // 두번째 인수: "manipulator" - robot description 에 정의되어 있는 the group of joints. 
  // auto move_group_interface = MoveGroupInterface(node, "panda_arm"/*"manipulator"*/);
  auto move_group_interface = MoveGroupInterface(node, "areum2_arm_l"/*"manipulator"*/);


  // Vel, Acc 스케일 지정
  move_group_interface.setMaxVelocityScalingFactor(0.9);
  move_group_interface.setMaxAccelerationScalingFactor(0);
  // move_group_interface.setPlannerId()


  // [Set a target Pose]
  // The starting pose is implicitly the position published by the joint state publisher
  // (Can be changed using "MoveGroupInterface::setStartState* family of functions")
  //
  // IIFE 형식
  // Lambdas for constructing the message type target_pose and planning
  auto const target_pose = []{
    geometry_msgs::msg::Pose msg;
    msg.orientation.w = 1.0;
    msg.position.x = 0.2;
    msg.position.y = 0.2;
    msg.position.z = 0.4;
    return msg;
  }();
  move_group_interface.setPoseTarget(target_pose);



  // [Create a plan to that target pose]
  // - 주어진 Path planner (OMPL 등) 로 Path 생성
  // - Trajectory 까지 plan() 함수에서 생성됨.
  auto const [success, plan] = [&move_group_interface]{
    moveit::planning_interface::MoveGroupInterface::Plan msg;
    auto const ok = static_cast<bool>(move_group_interface.plan(msg));
    return std::make_pair(ok, msg);
  }();

  // [Execute the plan]
  if(success) {
    move_group_interface.execute(plan);
    // - Trajectory 를 펌웨어단으로 명령을 보냄.
  } else {
    RCLCPP_ERROR(logger, "Planning failed!");
  }  


////////////////////////////////////////////////////////////////// ^^^ 정해진 EE Pose 로 이동. 
////////////////////////////////////////////////////////////////// 


  // Shutdown ROS
  rclcpp::shutdown();
  return 0;
}