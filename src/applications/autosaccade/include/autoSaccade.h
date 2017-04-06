/*
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author: Arren.Glover@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

/// \defgroup RobotIO RobotIO
/// \defgroup autosaccade autosaccade
/// \ingroup RobotIO
/// \brief automatically elicit iCub eye movement when event rate is low

#ifndef __ICUB_EVENTCLUSTERING_MOD_H__
#define __ICUB_EVENTCLUSTERING_MOD_H__

#include <yarp/os/all.h>
#include <iCub/eventdriven/all.h>
#include <yarp/dev/all.h>
#include <iCub/iKin/iKinFwd.h>
#include <yarp/math/Math.h>
using namespace yarp::math;

class EventBottleManager : public yarp::os::BufferedPort<ev::vBottle>
{
private:

    //for helping with timestamp wrap around
    ev::vtsHelper unwrapper;

    //rate counters
    yarp::os::Semaphore mutex;
    unsigned long int latestStamp;
    unsigned int vCount;
    bool isReading;
    ev::vQueue vQueue;
    
public:

    EventBottleManager();

    bool    open(const std::string &name);

    //this is the entry point to your main functionality
    void    onRead(ev::vBottle &bot);

    //the getting functions of the parent class
    unsigned long int getTime();
    unsigned long int popCount();
    ev::vQueue getEvents();
    bool start();
    bool stop();

};

class saccadeModule : public yarp::os::RFModule
{
private:
    //the remote procedure port
    yarp::os::RpcServer     rpcPort;

    //the event bottle input and output handler
    EventBottleManager      eventBottleManager;

//    yarp::os::Port eventCount;
    
    //timing parameters
    double checkPeriod;
    double minVpS;
    double maxVps;
    bool isSaccading;

    //robot control settings
    yarp::dev::PolyDriver mdriver;
    double min, max;
    
    yarp::dev::IControlLimits2   *ilim;
    yarp::dev::IPositionControl2 *ipos;
    yarp::dev::IControlMode2     *imod;
    
    yarp::os::Port vRate;
    
    //timestamps for comparison
    double prevStamp;
    double lastSaccadeTime;
    double saccadeTimeout;
    void performSaccade();

public:

    //the virtual functions that need to be overloaded
    virtual bool configure(yarp::os::ResourceFinder &rf);
    virtual bool interruptModule();
    virtual bool close();
    virtual bool respond(const yarp::os::Bottle &command,
                         yarp::os::Bottle &reply);
    virtual double getPeriod();
    virtual bool updateModule();

};


#endif
//empty line to make gcc happy
