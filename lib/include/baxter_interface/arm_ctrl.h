#ifndef __ARM_CONTROLLER_H__
#define __ARM_CONTROLLER_H__

#include <map>

#include <robot_interface/robot_interface.h>

#include "baxter_control/DoAction.h"
#include "baxter_control/ArmState.h"
#include "baxter_control/ArmPos.h"

class ArmCtrl : public RobotInterface, public ROSThread
{
private:
    std::string  sub_state;

    std::string     action;
    std::string        dir;
    std::string       mode;
    float             dist;
    int          marker_id;
    int          object_id;
    int        update_flag;
    int       reached_flag;

    // Flag to know if the robot will recover from an error
    // or will wait the external planner to take care of that
    bool internal_recovery;

    ros::ServiceServer service;
    ros::ServiceServer service_other_limb;

    ros::Publisher     state_pub;

    ros::Subscriber    control_topic;

    geometry_msgs::Point        _desired_pos;

    baxter_core_msgs::JointCommand home_conf;

protected:

    /**
     * Pointer to the action prototype function, which does not take any
     * input argument and returns true/false if success/failure
     */
    typedef bool(ArmCtrl::*f_action)();

    /**
     * Action database, which pairs a string key, corresponding to the action name,
     * with its relative action, which is an f_action.
     *
     * Please be aware that, by default, if the user calls an action with the wrong
     * key or an action that is not available, the code will segfault. By C++
     * standard: operator[] returns (*((insert(make_pair(x, T()))).first)).second
     * Which means that if we are having a map of pointers to functions, a wrong key
     * will segfault the software. A layer of protection has been put in place to
     * avoid accessing a non-existing key.
     */
    std::map <std::string, f_action> action_db;

    /**
     * Object database, which pairs an integer key, corresponding to the marker ID
     * placed on the object and read by ARuco, with a string that describes the object
     * itself in human terms.
     */
    std::map<int, std::string> object_db;

    /**
     * Provides basic functionalities for the object, such as a goHome and releaseObject.
     * For deeper, class-specific specialization, please modify doAction() instead.
     */
    void InternalThreadEntry();

    /**
     * Recovers from errors during execution. It provides a basic interface,
     * but it is advised to specialize this function in the ArmCtrl's children.
     */
    void recoverFromError();

    /**
     * Hovers above table at a specific x-y position.
     * @param  height the z-axis value of the end-effector position
     * @return        true/false if success/failure
     */
    bool hoverAboveTable(double height, std::string mode="loose",
                                    bool disable_coll_av = false);

    /**
     * Home position with a specific joint configuration. This has
     * been introduced in order to force the arms to go to the home configuration
     * in always the same exact way, in order to clean the seed configuration in
     * case of subsequent inverse kinematics requests.
     *
     * @param  disable_coll_av if to disable the collision avoidance while
     *                         performing the action or not
     * @return                 true/false if success/failure
     */
    bool homePoseStrict(bool disable_coll_av = false);

    void setHomeConf(double s0, double s1, double e0, double e1,
                                     double w0, double w1, double w2);

    /**
     * Hovers above the table with a specific joint configuration. This has
     * been introduced in order to force the arms to go to the home configuration
     * in always the same exact way, in order to clean the seed configuration in
     * case of subsequent inverse kinematics requests.
     *
     * @param  disable_coll_av if to disable the collision avoidance while
     *                         performing the action or not
     * @return                 true/false if success/failure
     */
    bool hoverAboveTableStrict(bool disable_coll_av = false);

    /**
     * Goes to the home position
     * @return        true/false if success/failure
     */
    bool goHome();

    /**
     * Moves arm in a direction requested by the user, relative to the current
     * end-effector position
     *
     * @param dir  the direction of motion (left right up down forward backward)
     * @param dist the distance from the end-effector starting point
     *
     * @return true/false if success/failure
     */
    bool moveArm(std::string dir, double dist, std::string mode = "loose",
                                             bool disable_coll_av = false);

    bool movePose();

    /**
     * Placeholder for an action that has not been implemented (yet)
     *
     * @return false always
     */
    bool notImplemented();

    /**
     * Adds an object to the object database
     * @param  id the id of the object as read by ARuco
     * @param  n  its name as a string
     * @return    true/false if the insertion was successful or not
     */
    bool insertObject(int id, const std::string &n);

    /**
     * Removes an object from the database. If the object is not in the
     * database, the return value will be false.
     *
     * @param   id the object to be removed
     * @return     true/false if the removal was successful or not
     */
    bool removeObject(int id);

    /**
     * Gets an object from the object database
     *
     * @param    id the requested object
     * @return      the associated string
     *              (empty string if object is not there)
     */
    std::string getObjectName(int id);

    /**
     * Checks if an object is available in the database
     * @param  id the action to check for
     * @param  insertAction flag to know if the method has been called
     *                      inside insertAction (it only removes the
     *                      ROS_ERROR if the action is not in the DB)
     * @return   true/false if the action is available in the database
     */
    bool isObjectInDB(int id);

    /**
     * Prints the object database to screen.
     */
    void printObjectDB();

    /**
     * Converts the action database to a string.
     * @return the list of allowed actions, separated by a comma.
     */
    std::string objectDBToString();

    /**
     * Adds an action to the action database
     *
     * @param   a the action to be removed
     * @param   f a pointer to the action, in the form bool action()
     * @return    true/false if the insertion was successful or not
    /**
     * Adds an action to the action database
     *
     * @param   a the action to be removed
     * @param   f a pointer to the action, in the form bool action()
     * @return    true/false if the insertion was successful or not
     */
    bool insertAction(const std::string &a, ArmCtrl::f_action f);

    /**
     * Removes an action from the database. If the action is not in the
     * database, the return value will be false.
     *
     * @param   a the action to be removed
     * @return    true/false if the removal was successful or not
     */
    bool removeAction(const std::string &a);

    /**
     * Calls an action from the action database
     *
     * @param    a the action to take
     * @return     true/false if the action called was successful or failed
     */
    bool callAction(const std::string &a);

    /**
     * This function wraps the arm-specific and task-specific actions.
     * For this reason, it has been implemented as virtual because it depends on
     * the child class.
     *
     * @param  s the state of the system BEFORE starting the action (when this
     *           method is called the state has been already updated to WORKING,
     *           so there is no way for the controller to recover it a part from
     *           this)
     * @param  a the action to do
     * @return   true/false if success/failure
     */
    virtual bool doAction(int s, std::string a);

    /**
     * Checks if an action is available in the database
     * @param             a the action to check for
     * @param  insertAction flag to know if the method has been called
     *                      inside insertAction (it only removes the
     *                      ROS_ERROR if the action is not in the DB)
     * @return   true/false if the action is available in the database
     */
    bool isActionInDB(const std::string &a, bool insertAction=false);

    /**
     * Prints the action database to screen.
     */
    void printActionDB();

    /**
     * Converts the action database to a string.
     * @return the list of allowed actions, separated by a comma.
     */
    std::string actionDBToString();

    /**
     * This function wraps the arm-specific and task-specific actions.
     * For this reason, it has been implemented as virtual because it depends on
     * the child class.
     *
     * @param  s the state of the system BEFORE starting the action (when this
     *           method is called the state has been already updated to WORKING,
     *           so there is no way for the controller to recover it a part from
     *           this)
     * @param  a the action to do
     * @return   true/false if success/failure
     */

    float vector_norm(geometry_msgs::Point x);
    geometry_msgs::Point vector_difference(geometry_msgs::Point x0, geometry_msgs::Point x1);

public:
    /**
     * Constructor
     */
    ArmCtrl(std::string _name, std::string _limb, bool _no_robot = false);

    /*
     * Destructor
     */
    ~ArmCtrl();

    /**
     * Callback for the service that requests actions
     * @param  req the action request
     * @param  res the action response (res.success either true or false)
     * @return     true always :)
     */
    bool serviceCb(baxter_control::DoAction::Request  &req,
                   baxter_control::DoAction::Response &res);

    void setInitDesiredPose();

    /**
     * Callback for the service that lets the two limbs interact
     * @param  req the action request
     * @param  res the action response (res.success either true or false)
     * @return     true always :)
     */
    virtual bool serviceOtherLimbCb(baxter_control::DoAction::Request  &req,
                                    baxter_control::DoAction::Response &res);

    void moveArmCb(const baxter_control::ArmPos::ConstPtr& msg);
    float ComputeStepSize(float start, float finish, float frequency);
    void updateDesiredPoseCb(const baxter_control::ArmPos::ConstPtr& msg);

    void publishState();

    /* Self-explaining "setters" */
    void setSubState(std::string _state) { sub_state =  _state; };
    void setMarkerID(int _id)            { marker_id =     _id; };
    virtual void setObjectID(int _obj)   { object_id =    _obj; };
    void setAction(std::string _action);
    void setDir(std::string _dir);
    void setMode(std::string _mode);
    void setDist(float dist);

    void setState(int _state);

    /* Self-explaining "getters" */
    std::string getSubState() { return sub_state; };
    std::string getAction()   { return    action; };
    std::string getDir()      { return       dir; };
    std::string getMode()     { return      mode; };
    float       getDist()     { return      dist; };
    int         getMarkerID() { return marker_id; };
    int         getObjectID() { return object_id; };
    std::string getObjName();
    geometry_msgs::Point        getDesiredPos();
};

#endif
