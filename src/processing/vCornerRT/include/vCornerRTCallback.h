/*
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author: Valentina Vasco
 * email:  valentina.vasco@iit.it
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

/// \defgroup Modules Modules
/// \defgroup vCorner vCorner
/// \ingroup Modules
/// \brief detects corner events using the Harris method

#ifndef __VCORNERCALLBACK__
#define __VCORNERCALLBACK__

#include <yarp/os/all.h>
#include <yarp/sig/all.h>
#include <yarp/math/Math.h>
#include <iCub/eventdriven/all.h>
#include <iCub/eventdriven/vtsHelper.h>
#include <filters.h>
#include <fstream>
#include <math.h>
#include <vCornerRTCallback.h>
#include <iomanip>

class vCornerCallback : public yarp::os::BufferedPort<ev::vBottle>
{
private:

    bool strictness;

    //output port for the vBottle with the new events computed by the module
    yarp::os::BufferedPort<ev::vBottle> outPort;
    yarp::os::BufferedPort<yarp::os::Bottle> debugPort;

    //data structures
    ev::temporalSurface *surfaceleft;
    ev::temporalSurface *surfaceright;

    //parameters
    int height;
    int width;
    unsigned int qlen;
    int temporalsize;
    int windowRad;
    double thresh;

    //for synchronization between adding events and processing
    double cpudelay;
    int prevstamp;
    double t1;
    double t2;

    int circle3[16][2] =
    {
        {0, 3},
        {1, 3},
        {2, 2},
        {3, 1},
        {3, 0},
        {3, -1},
        {2, -2},
        {1, -3},
        {0, -3},
        {-1, -3},
        {-2, -2},
        {-3, -1},
        {-3, 0},
        {-3, 1},
        {-2, 2},
        {-1, 3}
    };

    int circle4[20][2] =
    {
        {0, 4},
        {1, 4},
        {2, 3},
        {3, 2},
        {4, 1},
        {4, 0},
        {4, -1},
        {3, -2},
        {2, -3},
        {1, -4},
        {0, -4},
        {-1, -4},
        {-2, -3},
        {-3, -2},
        {-4, -1},
        {-4, 0},
        {-4, 1},
        {-3, 2},
        {-2, 3},
        {-1, 4}
    };

    std::ofstream outfile;
    ev::vtsHelper unwrapper;

    filters convolution;
    bool detectcorner(const ev::vQueue subsurf, int x, int y);
    bool detectcornerfast(unsigned int patch3[16], unsigned int patch4[20]);

public:

    vCornerCallback(int height, int width, int filterSize, int windowRad, double temporalsize,
                    double sigma, int qlen, double thresh);

    bool    open(const std::string moduleName, bool strictness = false);
    void    close();
    void    interrupt();

    //this is the entry point to your main functionality
    void    onRead(ev::vBottle &bot);

};

#endif
//empty line to make gcc happy

