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

#include "cluster.h"

using namespace ev;

cluster::cluster()
{

    tlast_update = 0.0;
    maxsize = 50;
    vel.first = 0.0;
    vel.second = 0.0;

}

double cluster::dist2event(ev::event<LabelledAE> evt)
{
    auto laste = is_event<LabelledAE>(this->getLastEvent());
    double dx = evt->x - laste->x;
    double dy = evt->y - laste->y;
    double dist = sqrt(dx*dx+dy*dy);

    return dist;
}

void cluster::addEvent(ev::event<LabelledAE> evt)
{
    cluster_.push_back(evt);
    tlast_update = evt->stamp;

    //remove the first event added
    if(cluster_.size() > maxsize)
        cluster_.pop_front();
}

ev::event<> cluster::getLastEvent()
{
    if(!cluster_.size()) return NULL;
    return cluster_.back();
}

void cluster::fitLine()
{
    yarp::sig::Vector meanvec(3);
    yarp::sig::Matrix covariance(2, 2);

    unsigned n = cluster_.size();
    yarp::sig::Matrix data(n, 3);
    yarp::sig::Matrix centreddata(n, 3);

    yarp::sig::Matrix U;
    yarp::sig::Vector S;
    yarp::sig::Matrix V;

    int count = 0;
    meanvec[0] = 0.0;
    meanvec[1] = 0.0;
    meanvec[2] = 0.0;
    for(ev::vQueue::iterator qi = cluster_.begin(); qi != cluster_.end(); qi++)
    {
        auto cep = is_event<LabelledAE>(*qi);
        data[count][0] = cep->x;
        data[count][1] = cep->y;
        data[count][2] = cep->stamp;

        meanvec[0] += cep->x;
        meanvec[1] += cep->y;
        meanvec[2] += cep->stamp;

        count++;
    }
    meanvec[0] = meanvec[0]/n;
    meanvec[1] = meanvec[1]/n;
    meanvec[2] = meanvec[2]/n;

    //center data
    for(unsigned int i = 0; i < n; i++)
    {
        centreddata[i][0] = data[i][0] - meanvec[0];
        centreddata[i][1] = data[i][1] - meanvec[1];
        centreddata[i][2] = data[i][2] - meanvec[2];

//        std::cout << centreddata[i][0] << " " << centreddata[i][1] << " " << centreddata[i][2] << std::endl;
    }

    yarp::math::SVDJacobi(centreddata, U, S, V);

    yarp::sig::Vector v;
    v = V.getCol(0);
    std::cout << -v[0]/v[2] * 1000000 << " " << -v[1]/v[2] * 1000000 << std::endl;
    vel.first = -v[0] /v[2]; //U(0, 2); // / U(2, 2);
    vel.second = -v[1] /v[2]; //U(1, 2); // / U(2, 2);

    //return parameters of the line a/c, b/c
//    return vel;

}

unsigned int cluster::getLastUpdate()
{
    return tlast_update;
}

int cluster::getClusterSize()
{
    return cluster_.size();
}

double cluster::getVx()
{
    return vel.first;
}

double cluster::getVy()
{
    return vel.second;
}

//vQueue cluster::getCluster()
//{
//    return cluster_;
//}

//empty line to make gcc happy
