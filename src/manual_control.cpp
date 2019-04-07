#include "ros/ros.h"
#include "geometry_msgs/Twist.h"
#include "std_msgs/Bool.h"
#include "isc_joy/xinput.h"

#include <string>

ros::Publisher manualPub;
ros::Publisher driveModeSignalPub;

bool flipForwardBackward = false;
bool flipLeftRight = false;
bool startButtonDown = false;

double speedMultiplier;
double turnMultiplier;

bool enableLogging;

/* This fires every time a button is pressed/released
and when an axis changes (even if it doesn't leave the
deadzone). */
void joystickCallback(const isc_joy::xinput::ConstPtr& joy){

	bool enableDriving = joy->LB; //the dead man's switch

	//toggle flipping controls
	if(joy->Y && !enableDriving) flipForwardBackward = !flipForwardBackward;
	if(joy->X && !enableDriving) flipLeftRight = !flipLeftRight;

	float joySpeed = 0.0, joyTurn = 0.0;
	joySpeed = joy->LeftStick_UD * (flipForwardBackward ? -1.0 : 1.0);// * speedMultiplier;
	joyTurn = joy->LeftStick_LR * (flipLeftRight ? -1.0 : 1.0) * turnMultiplier;

	//disable joystick axes
	if(joy->A) joySpeed = 0;
	if(joy->B) joyTurn = 0;

	geometry_msgs::Twist msg;
	msg.linear.x = enableDriving ? joySpeed : 0;
	msg.angular.z = enableDriving ? joyTurn : 0;
	manualPub.publish(msg);

	// send drive mode signal to robot state controller
	std_msgs::Bool driveModeSignal;
	if (joy->Start && !startButtonDown) { //The Start button has been pressed
		startButtonDown = true;
		driveModeSignal->data = false;
		driveModeSignalPub.publish(driveModeSignal);
	} else if (!joy->Start && startButtonDown) { // start button released
		startButtonDown = false;
		driveModeSignal->data = true;
		driveModeSignalPub.publish(driveModeSignal);
	} else { // some other button was pressed
		driveModeSignal->data = false;
		driveModeSignalPub.publish(driveModeSignal);
	}

	ROS_INFO_COND(enableLogging, "Manual Control: %s linear.x=%f angular.z=%f", joy->LB ? "on" : "off", msg.linear.x, msg.angular.z);
}

int main(int argc, char **argv){
	ros::init(argc, argv, "manual_control");

	ros::NodeHandle n;

	n.param("manual_control_enable_logging", enableLogging, false);
	n.param("manual_control_speed_multiplier", speedMultiplier, 1.0);
	n.param("manual_control_turn_multiplier", turnMultiplier, 0.5);

	manualPub = n.advertise<geometry_msgs::Twist>("manual_control", 5);
	driveModeSignalPub = n.advertise<std_msgs::Bool>("/signal/drive_mode", 5);

	ros::Subscriber joystickSub = n.subscribe("joystick/xinput", 5, joystickCallback);

	ros::spin();

	return 0;
}
