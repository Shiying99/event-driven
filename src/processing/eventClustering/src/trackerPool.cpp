/*
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences -
 * Istituto Italiano di Tecnologia
 * Author: Chiara Bartolozzi and Arren Glover
 * email:  chiara.bartolozzi@iit.it, arren.glover@iit.it
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

#include "trackerPool.h"

TrackerPool::TrackerPool()
{
    sig_x2_ = 25;
    sig_y2_ = 25;
    sig_xy_ = 0;
    fixed_shape_ = false;
    alpha_pos = 0.1;
    alpha_shape = 0.01;

    decay_tau = 10000;
    ts_last_reg_ = -1;
    nb_ev_regulate_ = 50;
    count_ = 0;

    Tact = 50;
    Tinact = 20;
    Tfree = 10;
    Tevent = 2;

    max_dist = 10;
    //trackers_ = new std::vector<BlobTracker>;
    probThr = 0.01;
    
    trackerLocked = false;

}

/******************************************************************************/
void TrackerPool::setInitialParams(double sig_x, double sig_y, double sig_xy,
                      double alpha_pos, double alpha_shape, bool fixed_shape)
{
    this->sig_x2_ = std::pow(sig_x, 2.0);
    this->sig_y2_ = std::pow(sig_y, 2.0);
    this->sig_xy_ = sig_xy;
    this->alpha_pos = alpha_pos;
    this->alpha_shape = alpha_shape;
    this->fixed_shape_ = fixed_shape;

}

/******************************************************************************/
void TrackerPool::setDecayParams(double decay_tau, double Tact, double Tinact,
                    double Tfree, double Tevent, int rate)
{
    this->decay_tau = decay_tau;
    this->Tact = Tact;
    this->Tinact = Tinact;
    this->Tfree = Tfree;
    this->Tevent = Tevent;
    this->nb_ev_regulate_ = rate;
}

/******************************************************************************/
void TrackerPool::setComparisonParams(double max_dist)
{
    this->max_dist = max_dist;
}

/******************************************************************************/
void TrackerPool::setClusterLimit(int limit)
{
    clusterLimit = limit;
}

/******************************************************************************/
void TrackerPool::setProbabilityThreshold(double pThr)
{
    probThr = pThr;
}

/******************************************************************************/
int TrackerPool::update(emorph::AddressEvent &event,
                        std::vector<emorph::ClusterEventGauss> &clEvts)
{
    //printf("trackerPool update");
    unsigned long int ev_t = unwrap(event.getStamp());
    int ev_x = event.getX();
    int ev_y = event.getY();

    double max_p = 0;
    int trackId = -1;

    double p;
    
    //the first event sets the beginning of the regulation cycle
    if(ts_last_reg_ < 0) ts_last_reg_ = ev_t;
    
    if (trackerLocked == true)
    {
        trackId = 0; // deve essere il trackId di quello locked // todo
        if(trackers_[trackId].dist2event(ev_x, ev_y) < max_dist){
            p = trackers_[trackId].compute_p(ev_x, ev_y);
        }
    } else
    {
        // We look for the tracker with the biggest p
       for(int ii=0; ii<trackers_.size(); ii++){
            // only among the Active and Inactive clusters
            if(trackers_[ii].isOn() && trackers_[ii].dist2event(ev_x, ev_y) < max_dist){
                p = trackers_[ii].compute_p(ev_x, ev_y);
                if(p > max_p || trackId == -1){
                    max_p = p;
                    trackId = ii;
                }
            }
        }
    }
    // If there was not any tracker close enough,
    // we take one of the Free trackers (to_reset_) and reset
    // it to the position of the event
    
    if(trackId == -1) {
    
        trackId = getNewTracker();
        
        if(trackId >= 0) {
            trackers_[trackId].initialisePosition(ev_x, ev_y);
            trackers_[trackId].clusterSpiked();
            trackers_[trackId].isNoLongerFree();
        }
    }
    
    // Otherwise, we update the one with the highest probability
    /* 
     If the tracker is locked there can be noise (so we do not want to update for low probability) but when we loose the tracker because of overt tracking, then we must be able to update the position of the cluster. We can assume that we always have some events from the cluster, or that the cluster is moving at constant velocity. In the latter case, even if we loose the tracker for a while, it keeps moving and the probability of the event to belong to the cluster will be quite high and above the threshold.
     */
    else if (p > probThr){
        bool spiked = trackers_[trackId].addActivity(ev_x, ev_y, ev_t, Tact, Tevent);
        if(spiked) {
           clEvts.push_back(makeEvent(trackId, event.getStamp()));
        }
    }

    //regulate the pool only each nb_ev_regulate_ events
    if(++count_ < nb_ev_regulate_) return trackId;

    //do the regulation
    int dt = ev_t - ts_last_reg_;
    ts_last_reg_ = ev_t;
    count_ = 0;

    for(int i = 0; i < trackers_.size(); i++){
        // We regulate the activity of each Active tracker, but not of the Locked tracker
        if(!(trackers_[i].isOn())||trackers_[i].isLocked()) continue;
        bool spiked = trackers_[i].decayActivity(dt, decay_tau,
                                                 Tinact, Tfree);
        if(spiked) clEvts.push_back(makeEvent(i, event.getStamp()));
    }

    return trackId;
}

/******************************************************************************/
int TrackerPool::getNewTracker()
{
    //printf("getNewTracker\n");
    //check to see if there is a free tracker already created
    for(int i = 0; i < trackers_.size(); i++) {
        //printf("i: %d %d",i,trackers_[i].isFree());
        if(trackers_[i].isFree()) {
            trackers_[i].initialiseShape(sig_x2_, sig_y2_, sig_xy_, alpha_pos,
                                        alpha_shape, fixed_shape_);
            return i;
        }
    }

    //else no free trackers

    //first check if we are under the limit
    if(clusterLimit < 0 || trackers_.size() < clusterLimit) {
        //printf("new tracker, trackId: %lu\n", trackers_.size());
        BlobTracker newtracker;
        newtracker.initialiseShape(sig_x2_, sig_y2_, sig_xy_,
                                   alpha_pos, alpha_shape, fixed_shape_);
        trackers_.push_back(newtracker);

        return trackers_.size()  - 1;
    }

    return -1;

}

/******************************************************************************/
emorph::ClusterEventGauss TrackerPool::makeEvent(int i, int ts)
{
    emorph::ClusterEventGauss clep;

    clep.setStamp(ts);
    if(trackers_[i].isActive()) {
        clep.setPolarity(1);
    } else {
        clep.setPolarity(0);
    }
    clep.setXCog(trackers_[i].get_x());
    clep.setYCog(trackers_[i].get_y());
    clep.setXSigma2(trackers_[i].get_sigx2());
    clep.setYSigma2(trackers_[i].get_sigy2());
    clep.setXYSigma(trackers_[i].get_sigxy());
    clep.setXVel(trackers_[i].get_vx());
    clep.setYVel(trackers_[i].get_vy());
    clep.setNumAE(trackers_[i].get_act());
    clep.setID(i);

    trackers_[i].clusterSpiked();
    return clep;
}

/******************************************************************************/
bool TrackerPool::lockCluster(int trackId)
{
    if (trackId < trackers_.size())
    {
        printf("locking trackId %d\n", trackId);
        trackers_[trackId].Lock();
        trackerLocked = true;
        return true;
    } else {
        printf("non existing cluster -- locking failed\n");
        return false;
    }
}

/******************************************************************************/
bool TrackerPool::unlockCluster(int trackId)
{
    if (trackId < trackers_.size())
    {
        printf("unlocking trackId %d\n", trackId);
        trackers_[trackId].unLock();
        trackerLocked = false;
        return true;
    } else {
        printf("non existing cluster -- unlocking failed\n");
        return false;
    }
}

/******************************************************************************/

//void TrackerPool::apply_rep_field(){
//    // Apply repulsion between filters
//    for(int ii=0; ii<max_nb_tr_; ii++){
//        for(int jj=ii+1; jj<max_nb_tr_; jj++){
//            // We compute the distance
//            int dx = trackers_[ii].get_x() - trackers_[jj].get_x();
//            int dy = trackers_[ii].get_y() - trackers_[jj].get_y();
//            double dist = sqrt(dx*dx + dy*dy);
//            //printf("distance = %f and d_rep = %d\n", dist, d_rep_);
            
//            // If the distance is smaller than the minimum, we apply the repulsion filter
//            if(dist<d_rep_){
//                // The repulsion field will be a liner function of the distance. If the distance is zero, then it is one. If
//                // it is equal to d_rep, it will be zero
//                //printf("Alpha rep = %f\n", alpha_rep_);
//                double delta = alpha_rep_*(1-dist/d_rep_);
//                //printf("distance = %f - delta = %f\n", dist, delta);
//                int dir_x = dx>0?1:-1;
//                int dir_y = dy>0?1:-1;
//                double act_sum = trackers_[ii].get_act()+trackers_[jj].get_act();
//                double f_i, f_j;
                
//                if(act_sum > 0){
//                    f_i = trackers_[jj].get_act()/act_sum;
//                    f_j = trackers_[ii].get_act()/act_sum;
//                }
//                else{
//                    f_i = 0.5;
//                    f_j = 0.5;
//                }
//                trackers_[ii].displace(f_i*delta*dir_x, f_i*delta*dir_y);
//                trackers_[jj].displace(-f_j*delta*dir_x, -f_j*delta*dir_y);
//            }
//        }
//    }
//}
