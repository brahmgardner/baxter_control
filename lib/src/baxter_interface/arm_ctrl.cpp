#include "baxter_interface/arm_ctrl.h"
#include "baxter_control/ArmPos.h"
#include <pthread.h>

using namespace std;
using namespace geometry_msgs;
using namespace baxter_core_msgs;

ArmCtrl::ArmCtrl(string _name, string _limb, bool _no_robot) :
                 RobotInterface(_name, _limb, _no_robot),
                 action(""), sub_state("")
{
    std::string topic = "/"+getName()+"/state_"+_limb;
    state_pub = _n.advertise<baxter_control::ArmState>(topic,1);
    ROS_INFO("[%s] Created state publisher with name : %s", getLimb().c_str(), topic.c_str());

    std::string other_limb = getLimb() == "right" ? "left" : "right";

    topic = "/"+getName()+"/service_"+_limb;
    control_topic = _n.subscribe(topic, 1, &ArmCtrl::moveArmCb, this);
    // service = _n.advertiseService(topic, &ArmCtrl::serviceCb, this);
    ROS_INFO("[%s] Created service server with name  : %s", getLimb().c_str(), topic.c_str());

    topic = "/"+getName()+"/service_"+_limb+"_to_"+other_limb;
    service_other_limb = _n.advertiseService(topic, &ArmCtrl::serviceOtherLimbCb,this);
    ROS_INFO("[%s] Created service server with name  : %s", getLimb().c_str(), topic.c_str());

    // insertAction(ACTION_HOME,    &ArmCtrl::goHome);
    // insertAction(ACTION_RELEASE, &ArmCtrl::releaseObject);
    insertAction(MOVE,      &ArmCtrl::movePose);

    _n.param<bool>("internal_recovery",  internal_recovery, true);
    ROS_INFO("[%s] Internal_recovery flag set to %s", getLimb().c_str(),
                                internal_recovery==true?"true":"false");

}

void ArmCtrl::InternalThreadEntry()
{
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    _n.param<bool>("internal_recovery",  internal_recovery, true);

    std::string a =     getAction();
    int         s = int(getState());

    setState(WORKING);

    if (a == ACTION_HOME || a == ACTION_RELEASE)
    {
        if (callAction(a))   setState(DONE);
    }
    else if (s == START || s == ERROR ||
             s == DONE  || s == KILLED )
    {
        if (doAction(s, a))   setState(DONE);
        else                  setState(ERROR);
    }
    else
    {
        ROS_ERROR("[%s] Invalid Action %s in state %i", getLimb().c_str(), a.c_str(), s);
    }

    if (getState()==WORKING)
    {
        setState(ERROR);
    }

    if (getState()==ERROR)
    {
        ROS_ERROR("[%s] Action %s not successful! State %s %s", getLimb().c_str(), a.c_str(),
                                          string(getState()).c_str(), getSubState().c_str());
    }

    closeInternalThread();
    return;
}

void ArmCtrl::moveArmCb(const baxter_control::ArmPos::ConstPtr& msg)
{
    std::string action = msg->action;
    std::string dir    = msg->dir;
    std::string mode   = msg->mode;
    float dist    = msg->dist;
    int    obj    = msg->obj;

    ROS_INFO("[%s] Message request received. Action: %s object: %i", getLimb().c_str(),
                                                                   action.c_str(), obj);

    if (action == PROT_ACTION_LIST)
    {
        printActionDB();
        // res.success  = true;
        // res.response = actionDBToString();
        // return true;
        return;
    }

    if (is_no_robot())
    {
        setState(WORKING);
        ros::Duration(2.0).sleep();
        setState(DONE);
        // res.success = true;
        // return true;
        return;
    }

    // res.success = false;

    setDir(dir);
    setMode(mode);
    setDist(dist);
    setAction(action);
    setObjectID(obj);

    startInternalThread();
    ros::Duration(0.5).sleep();

    ros::Rate r(100);
    while( ros::ok() && ( int(getState()) != START   &&
                          int(getState()) != ERROR   &&
                          int(getState()) != DONE    &&
                          int(getState()) != PICK_UP   ))
    {
        if (ros::isShuttingDown())
        {
            setState(KILLED);
            // return true;
            return;
        }

        if (getState()==KILLED)
        {
            recoverFromError();
        }

        r.sleep();
    }

    // if ( int(getState()) == START   ||
    //      int(getState()) == DONE    ||
    //      int(getState()) == PICK_UP   )
    // {
    //     res.success = true;
    // }

    // ROS_INFO("[%s] Service reply with success: %s\n", getLimb().c_str(),
    //                                         res.success?"true":"false");
    // return true;
    return;
}

bool ArmCtrl::serviceOtherLimbCb(baxter_control::DoAction::Request  &req,
                                 baxter_control::DoAction::Response &res)
{
    res.success = false;
    res.response   = "not implemented";
    return true;
}

// bool ArmCtrl::serviceCb(baxter_control::DoAction::Request  &req,
//                         baxter_control::DoAction::Response &res)
// {
//     std::string action = req.action;
//     std::string dir    = req.dir;
//     std::string mode   = req.mode;
//     float dist    = req.dist;
//     int    obj    = req.obj;

//     ROS_INFO("[%s] Service request received. Action: %s object: %i", getLimb().c_str(),
//                                                                    action.c_str(), obj);

//     if (action == PROT_ACTION_LIST)
//     {
//         printActionDB();
//         res.success  = true;
//         res.response = actionDBToString();
//         return true;
//     }

//     if (is_no_robot())
//     {
//         setState(WORKING);
//         ros::Duration(2.0).sleep();
//         setState(DONE);
//         res.success = true;
//         return true;
//     }

//     res.success = false;

//     setDir(dir);
//     setMode(mode);
//     setDist(dist);
//     setAction(action);
//     setObjectID(obj);

//     startInternalThread();
//     ros::Duration(0.5).sleep();

//     ros::Rate r(100);
//     while( ros::ok() && ( int(getState()) != START   &&
//                           int(getState()) != ERROR   &&
//                           int(getState()) != DONE    &&
//                           int(getState()) != PICK_UP   ))
//     {
//         if (ros::isShuttingDown())
//         {
//             setState(KILLED);
//             return true;
//         }

//         if (getState()==KILLED)
//         {
//             recoverFromError();
//         }

//         r.sleep();
//     }

//     if ( int(getState()) == START   ||
//          int(getState()) == DONE    ||
//          int(getState()) == PICK_UP   )
//     {
//         res.success = true;
//     }

//     ROS_INFO("[%s] Service reply with success: %s\n", getLimb().c_str(),
//                                             res.success?"true":"false");
//     return true;
// }

bool ArmCtrl::notImplemented()
{
    ROS_ERROR("[%s] Action not implemented!", getLimb().c_str());
    return false;
}

bool ArmCtrl::insertObject(int id, const std::string &n)
{
    if (isObjectInDB(id))
    {
        ROS_WARN("[%s][object_db] Overwriting existing object %i with name %s",
                 getLimb().c_str(), id, n.c_str());
    }

    object_db.insert( std::make_pair( id, n ));
    return true;
}

bool ArmCtrl::removeObject(int id)
{
    if (isObjectInDB(id))
    {
        object_db.erase(id);
        return true;
    }

    return false;
}

string ArmCtrl::getObjectName(int id)
{
    if (isObjectInDB(id))
    {
        return object_db[id];
    }

    return "";
}

bool ArmCtrl::isObjectInDB(int id)
{
    if (object_db.find(id) != object_db.end()) return true;
    return false;
}

void ArmCtrl::printObjectDB()
{
    ROS_INFO("[%s] Available objects in the database : %s",
              getLimb().c_str(), objectDBToString().c_str());
}

string ArmCtrl::objectDBToString()
{
    string res = "";
    map<int, string>::iterator it;

    for ( it = object_db.begin(); it != object_db.end(); it++ )
    {
        res = res + it->second + ", ";
    }
    res = res.substr(0, res.size()-2); // Remove the last ", "
    return res;
}

bool ArmCtrl::insertAction(const std::string &a, ArmCtrl::f_action f)
{
    if (a == PROT_ACTION_LIST)
    {
        ROS_ERROR("[%s][action_db] Attempted to insert protected action key: %s",
                 getLimb().c_str(), a.c_str());
        return false;
    }

    if (isActionInDB(a, true)) // The action is in the db
    {
        ROS_WARN("[%s][action_db] Overwriting existing action with key %s",
                 getLimb().c_str(), a.c_str());
    }

    action_db.insert( std::make_pair( a, f ));
    return true;
}

bool ArmCtrl::removeAction(const std::string &a)
{
    if (isActionInDB(a)) // The action is in the db
    {
        action_db.erase(a);
        return true;
    }

    return false;
}

bool ArmCtrl::callAction(const std::string &a)
{
    if (isActionInDB(a)) // The action is in the db
    {
        f_action act = action_db[a];
        return (this->*act)();
    }

    return false;
}

bool ArmCtrl::doAction(int s, std::string a)
{
    if (isActionInDB(a))
    {
        if (callAction(a))         return true;
        else                recoverFromError();
    }
    else
    {
        ROS_ERROR("[%s] Invalid Action %s in state %i", getLimb().c_str(), a.c_str(), s);
    }

    return false;
}

bool ArmCtrl::isActionInDB(const std::string &a, bool insertAction)
{
    if (action_db.find(a) != action_db.end()) return true;

    if (!insertAction)
    {
        ROS_ERROR("[%s][action_db] Action %s is not in the database!",
                  getLimb().c_str(), a.c_str());
    }
    return false;
}

void ArmCtrl::printActionDB()
{
    ROS_INFO("[%s] Available actions in the database : %s",
              getLimb().c_str(), actionDBToString().c_str());
}

string ArmCtrl::actionDBToString()
{
    string res = "";
    map<string, f_action>::iterator it;

    for ( it = action_db.begin(); it != action_db.end(); it++ )
    {
        res = res + it->first + ", ";
    }
    res = res.substr(0, res.size()-2); // Remove the last ", "
    return res;
}

bool ArmCtrl::movePose()
{
    float dist = getDist();
    std::string dir = getDir();
    std::string mode = getMode();
    if (!moveArm(dir, dist, mode, false)) {
        return false;
    }
    return true;
}

bool ArmCtrl::moveArm(string dir, double dist, string mode, bool disable_coll_av)
{
    Point start = getPos();
    Point final = getPos();

    Quaternion ori = getOri();

    if      (dir == "backward") final.x -= dist;
    else if (dir == "forward")  final.x += dist;
    else if (dir == "right")    final.y -= dist;
    else if (dir == "left")     final.y += dist;
    else if (dir == "down")     final.z -= dist;
    else if (dir == "up")       final.z += dist;
    else                               return false;

    ros::Time t_start = ros::Time::now();

    bool finish = false;

    ros::Rate r(100);
    while(RobotInterface::ok())
    {
        // if (disable_coll_av)    suppressCollisionAv();

        double t_elap = (ros::Time::now() - t_start).toSec();

        double px = start.x;
        double py = start.y;
        double pz = start.z;

        if (!finish)
        {
            if (dir == "backward" | dir == "forward")
            {
                int sgn = dir=="backward"?-1:+1;
                px = px + sgn * PICK_UP_SPEED * t_elap;

                if (dir == "backward")
                {
                    if (px < final.x) finish = true;
                }
                else if (dir == "forward")
                {
                    if (px > final.x) finish = true;
                }
            }
            if (dir == "right" | dir == "left")
            {
                int sgn = dir=="right"?-1:+1;
                py = py + sgn * PICK_UP_SPEED * t_elap;

                if (dir == "right")
                {
                    if (py < final.y) finish = true;
                }
                else if (dir == "left")
                {
                    if (py > final.y) finish = true;
                }
            }
            if (dir == "down" | dir == "up")
            {
                int sgn = dir=="down"?-1:+1;
                pz = pz + sgn * PICK_UP_SPEED * t_elap;

                if (dir == "down")
                {
                    if (pz < final.z) finish = true;
                }
                else if (dir == "up")
                {
                    if (pz > final.z) finish = true;
                }
            }
        }
        else
        {
            px = final.x;
            py = final.y;
            pz = final.z;
        }

        double ox = ori.x;
        double oy = ori.y;
        double oz = ori.z;
        double ow = ori.w;

        vector<double> joint_angles;
        if (!computeIK(px, py, pz, ox, oy, oz, ow, joint_angles)) return false;

        if (!goToPoseNoCheck(joint_angles))   return false;

        if (isPositionReached(final.x, final.y, final.z, mode)) return true;

        r.sleep();
    }

    return false;
}

// bool ArmCtrl::hoverAboveTable(double height, string mode, bool disable_coll_av)
// {
//     if (getLimb() == "right")
//     {
//         return RobotInterface::goToPose(HOME_POS_R, height, VERTICAL_ORI_R,
//                                                      mode, disable_coll_av);
//     }
//     else if (getLimb() == "left")
//     {
//         return RobotInterface::goToPose(HOME_POS_L, height, VERTICAL_ORI_L,
//                                                      mode, disable_coll_av);
//     }
//     else return false;
// }

bool ArmCtrl::homePoseStrict(bool disable_coll_av)
{
    ROS_INFO("[%s] Going to home position strict..", getLimb().c_str());

    ros::Rate r(100);
    while(ros::ok())
    {
        if (disable_coll_av)    suppressCollisionAv();

        JointCommand joint_cmd;
        joint_cmd.mode = JointCommand::POSITION_MODE;
        setJointNames(joint_cmd);

        joint_cmd.command = home_conf.command;

        publish_joint_cmd(joint_cmd);

        r.sleep();

        if(isConfigurationReached(joint_cmd))
        {
            return true;
        }
    }
    ROS_INFO("[%s] Done", getLimb().c_str());
}

// void ArmCtrl::setHomeConf(double s0, double s1, double e0, double e1,
//                                      double w0, double w1, double w2)
// {
//     setJointCommands( s0, s1, e0, e1, w0, w1, w2, home_conf);
// }

bool ArmCtrl::goHome()
{
    bool res = homePoseStrict();
    return res;
}

void ArmCtrl::recoverFromError()
{
    if (internal_recovery == true)
    {
        goHome();
    }
}

void ArmCtrl::setState(int _state)
{
    if (_state == KILLED && getState() != WORKING)
    {
        ROS_WARN("[%s] Attempted to kill a non-working controller", getLimb().c_str());
        return;
    }

    RobotInterface::setState(_state);

    if (_state == DONE)
    {
        setSubState(getAction());
    }
    publishState();
}

void ArmCtrl::setAction(string _action)
{
    action = _action;
    publishState();
}

void ArmCtrl::setDir(string _dir)
{
    dir = _dir;
    publishState();
}

void ArmCtrl::setDist(float _dist)
{
    dist = _dist;
    publishState();
}

void ArmCtrl::setMode(string _mode)
{
    mode = _mode;
    publishState();
}

void ArmCtrl::publishState()
{
    baxter_control::ArmState msg;

    msg.state  = string(getState());
    msg.action = getAction();

    state_pub.publish(msg);
}

ArmCtrl::~ArmCtrl()
{
    killInternalThread();
}
