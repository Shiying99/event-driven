// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/* 
 * Copyright (C) 2010 RobotCub Consortium, European Commission FP6 Project IST-004370
 * Authors: Rea Francesco, Charles Clercq
 * email:   francesco.rea@iit.it, charles.clercq@iit.it
 * website: www.robotcub.org 
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

/**
 * @file device2yarp.h
 * @brief Definition of the ratethread that receives events from DVS camera
 */

#ifndef _DEVICE2YARP_H
#define _DEVICE2YARP_H

//yarp include
#include <yarp/os/RateThread.h>
#include <yarp/os/BufferedPort.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <stdint.h>

#include <iCub/eventBuffer.h>
//#include "sending_buffer.h"
#include "config.h"

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t


/*
struct aer {
    u32 timestamp;
    u32 address;
};
*/
struct aer {
    u16 timestamp;
    u16 address;
    };


class device2yarp : public yarp::os::RateThread {
public:
    device2yarp(std::string deviceNumber, bool save, std::string filename);
    ~device2yarp();
    virtual void run();


    /**
    * function used to set the name of the port device where biases are sent
    * @param name name of the port of the device
    */
    void setDeviceName(std::string name);

private:

    yarp::os::BufferedPort<eventBuffer> port;
    FILE* raw;

    u64 seqTime;
    u64 ec;
    u32 monBufEvents;
    u32 monBufSize_b;

    int file_desc,len,sz;
    unsigned char buffer_msg[64];
    short enabled;
    char buffer[SIZE_OF_DATA];
    struct aer *pmon;
    int err;
    unsigned int timestamp;
    short cartX, cartY, polarity;

    unsigned int xmask;
    unsigned int ymask;
    int yshift;
    int xshift;
    int polshift;
    int polmask;
    int retinalSize;
    std::string portDeviceName;

    bool save;

    std::stringstream str_buf;
};

#endif //_DEVICE2YARP_H

//----- end-of-file --- ( next line intentionally left blank ) ------------------

