#ifndef __VSURFACEHANDLER__
#define __VSURFACEHANDLER__

#include <yarp/os/all.h>
#include <iCub/eventdriven/vBottle.h>
#include <iCub/eventdriven/vCodec.h>
#include <iCub/eventdriven/vWindow_basic.h>
#include <iCub/eventdriven/vWindow_adv.h>
#include <iCub/eventdriven/vFilters.h>
#include <deque>
#include <string>
#include <map>

namespace ev {

class queueAllocator : public yarp::os::BufferedPort<ev::vBottle>
{
private:

    std::deque<ev::vQueue *> qq;
    std::deque<yarp::os::Stamp> sq;
    yarp::os::Mutex m;
    yarp::os::Mutex dataready;

public:

    queueAllocator()
    {
        useCallback();
        setStrict();
    }

    ~queueAllocator()
    {
        m.lock();
        for(std::deque<ev::vQueue *>::iterator i = qq.begin(); i != qq.end(); i++)
            delete *i;
        qq.clear();
        m.unlock();
    }

    void onRead(ev::vBottle &inputbottle)
    {

        //make a new vQueue
        m.lock();
        qq.push_back(new vQueue);
        yarp::os::Stamp yarpstamp;
        getEnvelope(yarpstamp);
        sq.push_back(yarpstamp);
        m.unlock();
        //and decode the data
        inputbottle.addtoendof<ev::AddressEvent>(*(qq.back()));
        dataready.unlock();
    }

    ev::vQueue* getNextQ(yarp::os::Stamp &yarpstamp)
    {
        dataready.lock();
        if(qq.size() > 1) {
            yarpstamp = sq.front();
            return qq.front();
        }
        else
            return 0;

    }

    void scrapQ()
    {
        m.lock();
        delete qq.front();
        qq.pop_front();
        sq.pop_front();
        m.unlock();
    }

    void releaseDataLock()
    {
        dataready.unlock();
    }

};

class surfaceThread : public yarp::os::Thread
{
private:

    ev::temporalSurface surfaceLeft;
    ev::temporalSurface surfaceRight;

    queueAllocator allocatorCallback;

    yarp::os::Mutex m;
    yarp::os::Stamp yarpstamp;
    unsigned int ctime;

    int vcount;


public:

    surfaceThread()
    {
        vcount = 0;
        ctime = 0;
    }

    void configure(int height, int width)
    {
        surfaceLeft = ev::temporalSurface(width, height);
        surfaceRight = ev::temporalSurface(width, height);
    }

    bool open(std::string portname)
    {
        if(!allocatorCallback.open(portname))
            return false;
        start();
        return true;
    }

    void onStop()
    {
        allocatorCallback.close();
        allocatorCallback.releaseDataLock();
    }

    void run()
    {
        while(true) {

            ev::vQueue *q = 0;
            while(!q && !isStopping()) {
                q = allocatorCallback.getNextQ(yarpstamp);
            }
            if(isStopping()) break;

            for(ev::vQueue::iterator qi = q->begin(); qi != q->end(); qi++) {

                m.lock();

                vcount++;

                ctime = (*qi)->stamp;

                if((*qi)->getChannel() == 0)
                    surfaceLeft.fastAddEvent(*qi);
                else if((*qi)->getChannel() == 1)
                    surfaceRight.fastAddEvent(*qi);
                else
                    std::cout << "Unknown channel" << std::endl;

                m.unlock();

            }

            allocatorCallback.scrapQ();

        }

    }

    yarp::os::Stamp queryROI(ev::vQueue &fillq, int c, unsigned int t, int x, int y, int r)
    {

        //if(!vcount) return false;

        m.lock();
        if(c == 0)
            fillq = surfaceLeft.getSurf_Tlim(t, x, y, r);
        else
            fillq = surfaceRight.getSurf_Tlim(t, x, y, r);
        vcount = 0;
        m.unlock();
        return yarpstamp;
    }

    yarp::os::Stamp queryWindow(ev::vQueue &fillq, int c, unsigned int t)
    {

        m.lock();
        if(c == 0)
            fillq = surfaceLeft.getSurf_Tlim(t);
        else
            fillq = surfaceRight.getSurf_Tlim(t);
        vcount = 0;
        m.unlock();

        return yarpstamp;
    }

    unsigned int queryVTime()
    {
        return ctime;
    }

};

class hSurfThread : public yarp::os::Thread
{
private:

    int maxcpudelay; //maximum delay between v time and cpu time (in v time)
    int numEvts;

    queueAllocator allocatorCallback;
    historicalSurface surfaceleft;
    historicalSurface surfaceright;
    ev::vNoiseFilter filter;

    yarp::os::Mutex m;

    //current stamp to propagate
    yarp::os::Stamp ystamp;
    unsigned int vstamp;

    //synchronising value (add to it when stamps come in, subtract from it
    // when querying events).
    double cputime;
    int cpudelay;

public:

    hSurfThread()
    {
        numEvts = 36;
        cpudelay = 0;
        vstamp = 0;
        cputime = yarp::os::Time::now();
        maxcpudelay = 0.5 / vtsHelper::tsscaler;
    }

    void configure(int height, int width, double maxcpudelay)
    {
        //this->maxcpudelay = maxcpudelay / vtsHelper::tsscaler;
        this->maxcpudelay = vtsHelper::max_stamp * 0.25;
        surfaceleft.initialise(height, width);
        surfaceright.initialise(height, width);
        filter.initialise(width, height, 100000, 1);
    }

    void configure(int height, int width, double maxcpudelay, int numEvts)
    {
        //this->maxcpudelay = maxcpudelay / vtsHelper::tsscaler;
        this->numEvts = numEvts;
        this->maxcpudelay = vtsHelper::max_stamp * 0.25;
        surfaceleft.initialise(height, width);
        surfaceright.initialise(height, width);
        filter.initialise(width, height, 100000, 1);
    }

    bool open(std::string portname)
    {
        if(!allocatorCallback.open(portname))
            return false;

        start();
        return true;
    }

    void onStop()
    {
        allocatorCallback.close();
        allocatorCallback.releaseDataLock();
    }


    void run()
    {
        while(true) {

            ev::vQueue *q = 0;
            while(!q && !isStopping()) {
                q = allocatorCallback.getNextQ(ystamp);
            }
            if(isStopping()) break;

            for(ev::vQueue::iterator qi = q->begin(); qi != q->end(); qi++) {

                auto ae = is_event<AE>(*qi);
                if(!filter.check(ae->x, ae->y, ae->polarity, ae->channel, ae->stamp)) {
                    continue;
                }

                m.lock();

                int dt = (*qi)->stamp - vstamp;
                if(dt < 0) dt += vtsHelper::max_stamp;
                cpudelay += dt;
                vstamp = (*qi)->stamp;

                if((*qi)->getChannel() == 0)
                    surfaceleft.addEvent(*qi);
                else if((*qi)->getChannel() == 1)
                    surfaceright.addEvent(*qi);

                m.unlock();

            }

            allocatorCallback.scrapQ();

        }

    }

    vQueue queryROI(int channel, int numEvts, int r)
    {

        vQueue q;

        m.lock();
        double cpunow = yarp::os::Time::now();

        cpudelay -= (cpunow - cputime) * vtsHelper::vtsscaler * 1.1;
        cputime = cpunow;

        if(cpudelay < 0) cpudelay = 0;
        if(cpudelay > maxcpudelay) cpudelay = maxcpudelay;

        if(channel == 0)
            q = surfaceleft.getSurfaceN(cpudelay, numEvts, r);
        else
            q = surfaceright.getSurfaceN(cpudelay, numEvts, r);
        m.unlock();

        return q;
    }

    vQueue queryROI(int channel, unsigned int querySize, int x, int y, int r)
    {


        vQueue q;

        m.lock();
        double cpunow = yarp::os::Time::now();

        cpudelay -= (cpunow - cputime) * vtsHelper::vtsscaler * 1.02;
        cputime = cpunow;

        if(cpudelay < 0) cpudelay = 0;
        if(cpudelay > maxcpudelay) cpudelay = maxcpudelay;


        if(channel == 0)
            q = surfaceleft.getSurface(cpudelay, querySize, r, x, y);
        else
            q = surfaceright.getSurface(cpudelay, querySize, r, x, y);
        m.unlock();

        return q;
    }

    vQueue queryWindow(int channel, unsigned int querySize)
    {
        double cpunow = yarp::os::Time::now();

        vQueue q;

        m.lock();

        cpudelay -= (cpunow - cputime) * vtsHelper::vtsscaler * 1.02;
        cputime = cpunow;

        if(cpudelay < 0) cpudelay = 0;
        if(cpudelay > maxcpudelay) cpudelay = maxcpudelay;


        if(channel == 0)
            q = surfaceleft.getSurface(cpudelay, querySize);
        else
            q = surfaceright.getSurface(cpudelay, querySize);
        m.unlock();

        return q;
    }

    double queryDelay()
    {
        return cpudelay * vtsHelper::tsscaler;
    }

    yarp::os::Stamp queryYstamp()
    {
        return ystamp;
    }

    int queryVstamp()
    {
        m.lock();
        int modvstamp = vstamp - cpudelay;
        m.unlock();

        if(modvstamp < 0) modvstamp += vtsHelper::max_stamp;
        return modvstamp;

    }

};

class tWinThread : public yarp::os::Thread
{
private:

    queueAllocator allocatorCallback;
    vTempWindow windowleft;
    vTempWindow windowright;

    yarp::os::Mutex m;
    yarp::os::Stamp yarpstamp;
    unsigned int ctime;

public:

    tWinThread()
    {
        ctime = 0;
    }

    bool open(std::string portname)
    {
        if(!allocatorCallback.open(portname))
            return false;

        start();
        return true;
    }

    void onStop()
    {
        allocatorCallback.close();
        allocatorCallback.releaseDataLock();
    }

    void run()
    {
        while(true) {

            ev::vQueue *q = 0;
            while(!q && !isStopping()) {
                q = allocatorCallback.getNextQ(yarpstamp);
            }
            if(isStopping()) break;

            for(ev::vQueue::iterator qi = q->begin(); qi != q->end(); qi++) {

                m.lock();

                ctime = (*qi)->stamp;

                if((*qi)->getChannel() == 0)
                    windowleft.addEvent(*qi);
                else if((*qi)->getChannel() == 1)
                    windowright.addEvent(*qi);

                m.unlock();

            }

            allocatorCallback.scrapQ();

        }

    }

    vQueue queryWindow(int channel)
    {
        vQueue q;

        m.lock();
        if(channel == 0)
            q = windowleft.getWindow();
        else
            q = windowright.getWindow();
        m.unlock();

        return q;
    }

    void queryStamps(yarp::os::Stamp &yStamp, int &vStamp)
    {
        yStamp = yarpstamp;
        vStamp = ctime;
    }

};

class syncvstreams
{
private:

    std::map<std::string, ev::tWinThread> iPorts;
    //std::deque<tWinThread> iPorts;
    yarp::os::Stamp yStamp;
    int vStamp;
    //std::map<std::string, int> labelMap;

public:

    syncvstreams(void) {}

    bool open(std::string moduleName, std::string eventType)
    {
        //check already have an input of that type
        if(iPorts.count(eventType))
            return true;

        //otherwise open a new port
        if(!iPorts[eventType].open(moduleName + "/" + eventType + ":i"))
            return false;

        return true;
    }

    vQueue queryWindow(std::string vType, int channel)
    {
        yarp::os::Stamp ys;
        int vs;
        iPorts[vType].queryStamps(ys, vs);

        if(ys.getTime() > yStamp.getTime()) {
            yStamp = ys;
            vStamp = vs;
        }

        return iPorts[vType].queryWindow(channel);
    }

    void close()
    {
        std::map<std::string, ev::tWinThread>::iterator i;
        for(i = iPorts.begin(); i != iPorts.end(); i++)
            i->second.stop();
    }

    yarp::os::Stamp getystamp()
    {
        return yStamp;
    }

    int getvstamp()
    {
        return vStamp;
    }

};

}

#endif
