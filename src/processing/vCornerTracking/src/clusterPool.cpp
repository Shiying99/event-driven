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

#include "clusterPool.h"

using namespace ev;

clusterPool::clusterPool(double mindistance, double maxdistance, double trefresh, int maxsize, int minevts)
{
    this->mindistance = mindistance; //px
    this->maxdistance = maxdistance;
    this->trefresh = trefresh; //s
    firstevent = true;
    this->maxsize = maxsize; //number of events
    this->minevts = minevts;
}

std::pair <double, double> clusterPool::update(ev::event<ev::LabelledAE> evt, double currt)
{
//    double dist = 0.0, mindist = mindistance;
//    int clusterID = -1;
//    std::pair <double, double> clustervel;

//    //if it's the first event, we create the first cluster
//    if(firstevent) {
//        firstevent = false;
//        createNewCluster(evt, currt);
//    }

//    //compute the distance from the current event to each cluster
//    for(unsigned int i = 0; i < pool.size(); i++) {
//        dist = pool[i].dist2event(evt);

//        //compute the minimum distance from all clusters
//        if(dist < mindist) {
//            mindist = dist;
//            clusterID = i;
//            pool[clusterID].addEvent(evt, currt);
////            std::cout << "updating " << clusterID << " at time " << currt << std::endl;
//        }
//    }

//    clustervel.first = 0.0;
//    clustervel.second = 0.0;
//    //if the minimum distance is less than a predefined threshold
//    if(clusterID >= 0) {

//        if(pool[clusterID].getClusterSize() > minevts) {
//            //            std::cout << "fitting line to cluster " << clusterID << " to event " << evt->x << " " << evt->y << " " << evt->stamp << std::endl;
//            pool[clusterID].fitLine();
//            clustervel.first = pool[clusterID].getVx() * 1000000.0;
//            clustervel.second = pool[clusterID].getVy() * 1000000.0;
//        }

//        //        std::cout << pool[clusterID].getVx() << " " << pool[clusterID].getVy() << std::endl;

//    } else {

//        //create new cluster
//        createNewCluster(evt, currt);
//        clusterID = pool.size() - 1; // pool.size() + 1;
////        std::cout << "creating new cluster " << clusterID << std::endl;
//    }

//    //check for old clusters
//    for(unsigned int i = 0; i < pool.size(); i++) {
////        std::cout << "for cluster " << i << " " << " last update at " << pool[i].getLastUpdate() << " " << currt - pool[i].getLastUpdate() << std::endl;
//        if( (currt - pool[i].getLastUpdate()) > trefresh) {
////            std::cout << " killing cluster " << i << " " << pool.size() << std::endl;
//            killOldCluster(i);
//        }
//    }
////    std::cout << std::endl;

//    //return the velocity of the current cluster
//    return clustervel;

}

std::pair <double, double> clusterPool::updateNew(ev::event<ev::LabelledAE> evt, double currt)
{
    int clusterID = -1;
    std::pair <double, double> clustervel;
    clustervel.first = 0.0;
    clustervel.second = 0.0;

    //if it's the first event, we create the first cluster
    if(firstevent) {
        firstevent = false;
        createNewCluster(evt, currt);
    }
    else {

        //for each created cluster
        for(unsigned int i = 0; i < pool.size(); i++) {

//            std::cout << "test " << std::endl;
            //if the event is within the triangle add it
            //CHECK IF THIS HAPPENS FOR MORE CLUSTERS
            if(pool[i].isInTriangle(evt, currt)) {
//                std::cout << "adding evt " << evt->x << " " << evt->y << " to cluster " << i << std::endl;
                clusterID = i;
                pool[clusterID].addEvent(evt, currt);
            }

        }

        if(clusterID >= 0) {

            //start tracking when there are enough events in the cluster
            //and there the cluster moved enough
            if(pool[clusterID].getClusterSize() > minevts && pool[clusterID].getSpatialDist(evt) > mindistance) {
//                std::cout << "start tracking cluster " << clusterID << std::endl;
                pool[clusterID].fitLine();
                clustervel.first = pool[clusterID].getVx() * 1000000.0;
                clustervel.second = pool[clusterID].getVy() * 1000000.0;
            }

        } else {
            //create new cluster
//            std::cout << "create new cluster " << std::endl;
            createNewCluster(evt, currt);
            clusterID = pool.size() - 1;
        }

        //kill the cluster if it moved too much
//        std::cout << "cluster ID " << clusterID << std::endl;
//        std::cout << "distance " << pool[clusterID].getSpatialDist(evt) << std::endl;
        if(pool[clusterID].getSpatialDist(evt) > maxdistance) {
//            std::cout << "killing cluster " << clusterID << std::endl;
            killOldCluster(clusterID);
        }
    }

    return clustervel;

}

void clusterPool::createNewCluster(ev::event<ev::LabelledAE> evt, double currt)
{
    cluster newcluster;
    newcluster.initialise(maxsize);
    newcluster.addEvent(evt, currt);
    pool.push_back(newcluster);
}

void clusterPool::killOldCluster(int clusterID)
{
//    std::cout << "KILL!" << std::endl;
    pool.erase(pool.begin() + clusterID);
//    std::cout << "killed " << std::endl;
}



//std::vector <cluster> clusterPool::getPool()
//{
//    return pool;
//}

//empty line to make gcc happy
