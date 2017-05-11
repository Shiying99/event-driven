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

#ifndef __QUEUESET__
#define __QUEUESET__

#include <iCub/eventdriven/all.h>
#include <yarp/sig/all.h>
#include <fstream>
#include <localQueue.h>

using namespace ev;

class queueSet {

private:

    std::vector< std::vector <localQueue> > queues;
    int width;
    int height;
    unsigned int qlen;
    int windowRad;

public:

    queueSet() {}
    void initialise(int width, int height, unsigned int qlen, int windowRad)
    {
        this->width = width;
        this->height = height;
        this->qlen = qlen;
        this->windowRad = windowRad;

        //create a queue for each pixel
        queues.resize(height);
        for(int y = 0; y < height; y++) {
            queues[y].resize(width);
            for(int x = 0; x < width; x++) {
                queues[y][x].initialise(qlen, windowRad * 2 + 1, x, y);
            }
        }
    }

    void add(ev::event<AE> v)
    {

        //add the event to the correct queue and its neighborings
        for(int dx = -windowRad; dx <= windowRad; dx++) {
            for(int dy = -windowRad; dy <= windowRad; dy++) {
                int ix = v->x + dx;
                int iy = v->y + dy;
                if(ix < 0 || ix >= width || iy < 0 || iy >= height)
                    continue;

                queues[iy][ix].addEvent(v);
            }
        }
    }

    vQueue getQueue(ev::event<AE> v)
    {
        return queues[v->y][v->x].getSurface();
    }

 };


#endif
//empty line to make gcc happy
