#include "vCornerRTThread.h"

using namespace ev;

vCornerThread::vCornerThread(unsigned int height, unsigned int width, std::string name, bool strict, int qlen,
                             double temporalsize, int windowRad, int sobelsize, double sigma, double thresh, int nthreads)
{
    this->height = height;
    this->width = width;
    this->name = name;
    this->strict = strict;
    this->qlen = qlen;
    this->temporalsize = temporalsize / ev::vtsHelper::tsscaler;
    this->windowRad = windowRad;
    this->sobelsize = sobelsize;
    this->sigma = sigma;
    this->thresh = thresh;
    this->nthreads = nthreads;

    std::cout << "Creating surfaces..." << std::endl;
//    surfaceleft.initialise(height, width);
//    surfaceright.initialise(height, width);
    surfaceleft  = new temporalSurface(width, height, this->temporalsize);
    surfaceright = new temporalSurface(width, height, this->temporalsize);

//    int gaussiansize = 2*windowRad + 2 - sobelsize;
//    convolution.configure(sobelsize, gaussiansize);
//    convolution.setSobelFilters();
//    convolution.setGaussianFilter(sigma);

    std::cout << "Using a " << sobelsize << "x" << sobelsize << " filter ";
    std::cout << "and a " << 2*windowRad + 1 << "x" << 2*windowRad + 1 << " spatial window" << std::endl;

    for(int i = 0; i < nthreads; i ++) {
        computeThreads.push_back(new vComputeThread(sobelsize, windowRad, sigma, thresh, qlen, &outthread, &semaphore));
        computeThreads[i]->start();
    }
    std::cout << "Using " << nthreads << " threads for computation " << std::endl;

    this->cpudelay = 0.005;
    this->prevstamp = 0;
    this->t1 = this->t2 = 0.0; // yarp::os::Time::now();
    this->k = 0;

//    std::cout << this->setPriority(10, 2) << " ";

}

bool vCornerThread::threadInit()
{

    if(!allocatorCallback.open("/" + name + "/vBottle:i")) {
    std::cout << "could not open vBottleIn port " << std::endl;
        return false;
    }

    if(!outthread.open("/" + name + "/vBottle:o")) {
        std::cout << "could not open vBottleOut port" << std::endl;
        return false;
    }
    if(!outthread.start())
        return false;

//    if(!vBottleOut.open("/" + name + "/vBottle:o")) {
//        std::cout << "could not open vBottleOut port" << std::endl;
//        return false;
//    }

    std::string debugPortName = "/" + name + "/score:o";
    if(!debugPort.open(debugPortName)) {
        std::cout << "could not open debug port" << std::endl;
        return false;
    }

    std::cout << "Thread initialised" << std::endl;
    return true;
}


void vCornerThread::onStop()
{
    allocatorCallback.close();
    allocatorCallback.releaseDataLock();

    for(int i = 0; i < nthreads; i++)
        delete computeThreads[i];

    delete surfaceleft;
    delete surfaceright;
}


//void vCornerThread::run()
//{
//    double timeInit;
//    double stampInit;
//    bool stampChecked = false;

////    ev::temporalSurface *cSurf = 0;
//    while(true) {

//       // double t3 = yarp::os::Time::now();

//        ev::vQueue *q = 0;
//        while(!q && !isStopping()) {
//            q = allocatorCallback.getNextQ(yarpstamp);
//        }
//        if(isStopping()) break;

////       // cpudelay -= yarp::os::Time::now() - t3;

//        //we look for a free thread
//        for(int k = 0; k < nthreads; k++) {
//            if(!computeThreads[k]->isRunning()) {
//                std::cout << k << std::endl;
//                computeThreads[k]->setData(*q, *cSurf, yarpstamp);
//                computeThreads[k]->start();
//                break;
//            }
//        }

//        for(ev::vQueue::iterator qi = q->begin(); qi != q->end(); qi++) {

//            auto ae = ev::is_event<ev::AE>(*qi);
//            double dt = ae->stamp - prevstamp;

//            if (!stampChecked){
//                stampInit = ae->stamp * vtsHelper::tsscaler;
//                timeInit = yarp::os::Time::now();
//                stampChecked = true;
//            }
//            if(dt < 0.0) stampInit -= vtsHelper::max_stamp * vtsHelper::tsscaler;

//            if(dt < 0.0) dt += vtsHelper::max_stamp;
//            prevstamp = ae->stamp;

//            cSurf->fastAddEvent(*qi);

//        }

////        std::cout << yarp::os::Time::now() - timeInit << " " <<prevstamp * vtsHelper::tsscaler - stampInit << " " << (yarp::os::Time::now() - timeInit) - (prevstamp * vtsHelper::tsscaler - stampInit) << std::endl;

//        if(debugPort.getOutputCount()) {
//            yarp::os::Bottle &scorebottleout = debugPort.prepare();
//            scorebottleout.clear();
//            scorebottleout.addDouble((yarp::os::Time::now() - timeInit) - (prevstamp * vtsHelper::tsscaler - stampInit));
////            scorebottleout.addDouble((double)countProcessed/q->size());
////            scorebottleout.addDouble(cpudelay);
////            scorebottleout.addInt(evtcnt);
//            debugPort.write();
//        }

//        allocatorCallback.scrapQ();
////        std::cout << allocatorCallback.scrapQ() << std::endl;

//    }

//}

//void vCornerThread::run()
//{

//    //    ev::temporalSurface *cSurf = 0;
//    while(true) {

//        // double t3 = yarp::os::Time::now();

//        ev::vQueue *q = 0;
//        while(!q && !isStopping()) {
//            q = allocatorCallback.getNextQ(yarpstamp);
//        }
//        if(isStopping()) break;

//        // cpudelay -= yarp::os::Time::now() - t3;

//        int countProcessed = 0;
//        ev::vQueue patch;
//        for(ev::vQueue::iterator qi = q->begin(); qi != q->end(); qi++) {

//            auto ae = ev::is_event<ev::AE>(*qi);
//            double dt = ae->stamp - prevstamp;

//            if(dt < 0.0) dt += vtsHelper::max_stamp;
//            cpudelay -= dt * vtsHelper::tsscaler;
//            prevstamp = ae->stamp;

//            //            //we need to filter the input as we will less likely process noise
//            //            if(!spfilter.check(ae->x, ae->y, ae->polarity, ae->channel, ae->stamp))
//            //                continue;

//            //            ev::historicalSurface *cSurf;
//            ev::temporalSurface *cSurf;
//            if(ae->getChannel() == 0)
//                cSurf = surfaceleft;
//            //                cSurf = &surfaceleft;
//            else
//                cSurf = surfaceright;
//            //                cSurf = &surfaceright;
//            //            cSurf->addEvent(*qi);
//            cSurf->fastAddEvent(*qi);

//            if(cpudelay < 0.0) cpudelay = 0.0;

//            if(cpudelay <= 0.1) {
//                t1 = yarp::os::Time::now();
//                // we look for a free thread
//                for(int k = 0; k < nthreads; k++) {
//                    if(!computeThreads[k]->isRunning()) {
//                        computeThreads[k]->setData(cSurf, yarpstamp);
//                        computeThreads[k]->start();
////                        std::cout << k << std::endl;
//                        countProcessed++;
//                        break;
//                    }
//                }

//                ////                patch.clear();
//                ////                cSurf->getSurf(patch, windowRad);
//                ////                if(detectcorner(patch, ae->x, ae->y)) {
//                ////                    auto ce = ev::make_event<LabelledAE>(ae);
//                ////                    ce->ID = 1;
//                ////                    outthread.pushevent(ce, yarpstamp);
//                ////                }
//                ////                countProcessed++;

//                //time it took to process
//                //                cpudelay = 0.0;
//                cpudelay += yarp::os::Time::now() - t1;
//                //                t1 = yarp::os::Time::now();

//            }

//        }

//        if(debugPort.getOutputCount()) {
//            yarp::os::Bottle &scorebottleout = debugPort.prepare();
//            scorebottleout.clear();
//            scorebottleout.addDouble((double)countProcessed/q->size());
//            scorebottleout.addDouble(cpudelay);
//            debugPort.write();
//        }

//        allocatorCallback.scrapQ();
//        //        std::cout << allocatorCallback.scrapQ() << std::endl;

//    }

//}

//void vCornerThread::run()
//{
//    while(true) {

//        ev::vQueue *q = 0;
//        while(!q && !isStopping()) {
//            q = allocatorCallback.getNextQ(yarpstamp);
//        }
//        if(isStopping()) break;

//        int countProcessed = 0;
//        for(ev::vQueue::iterator qi = q->begin(); qi != q->end(); qi++) {

//            auto ae = ev::is_event<ev::AE>(*qi);
//            double dt = ae->stamp - prevstamp;
//            if(dt < 0.0) dt += vtsHelper::max_stamp;
//            cpudelay -= dt * vtsHelper::tsscaler;
//            prevstamp = ae->stamp;

//            ev::temporalSurface *cSurf;
//            if(ae->getChannel() == 0)
//                cSurf = surfaceleft;
//            else
//                cSurf = surfaceright;
//            cSurf->fastAddEvent(*qi);

//            if(cpudelay < 0.0) cpudelay = 0.0;

//            if(cpudelay <= 0.1) {
//                t1 = yarp::os::Time::now();

//                computeThreads[k]->setData(cSurf, yarpstamp);
////                std::cout << k << std::endl;
//                countProcessed++;
//                k++;

//                if(k == nthreads) k = 0;

//                //time it took to process
//                cpudelay += (yarp::os::Time::now() - t1); // * 0.8;
//            }
//        }

//        if(debugPort.getOutputCount()) {
//            yarp::os::Bottle &scorebottleout = debugPort.prepare();
//            scorebottleout.clear();
//            scorebottleout.addDouble((double)countProcessed/q->size());
//            scorebottleout.addDouble(cpudelay);
//            debugPort.write();
//        }

//        allocatorCallback.scrapQ();

//    }

//}

void vCornerThread::run()
{
    //    double timeInit;
    //    double stampInit;
    //    bool stampChecked = false;

    //    double time = 0.0;
    //    double dtneg = 0.0;
    //    double stamp1;

    while(true) {

        ev::vQueue *q = 0;
        while(!q && !isStopping()) {
            q = allocatorCallback.getNextQ(yarpstamp);
        }
        if(isStopping()) break;

        //yarp::os::Time::delay(0.0005);

        int countProcessed = 0;
        ev::vQueue patch;
        for(ev::vQueue::iterator qi = q->begin(); qi != q->end(); qi++) {

            auto ae = ev::is_event<ev::AE>(*qi);
            //            double dt = ae->stamp - prevstamp;

            //            if(dt < 0.0) dt += vtsHelper::max_stamp;
            //            cpudelay -= dt * vtsHelper::tsscaler;
            //            prevstamp = ae->stamp;

            //        if(q->size()) {
            //            auto ae = ev::is_event<ev::AE>((q->back()));
            //            std::cout << ae->stamp << std::endl;

            //            if(ae->stamp < vtsHelper::max_stamp) {
            //                double dt;
            //                if (!stampChecked){
            //                    dt = 0;
            //                } else {
            //                    dt = ae->stamp - prevstamp;
            //                }
            //                if(dt < 0.0) {
            //                    dtneg = dt * vtsHelper::tsscaler;
            //                    dt += vtsHelper::max_stamp;
            //                    //                    std::cout << ae->stamp * vtsHelper::tsscaler << " " << prevstamp * vtsHelper::tsscaler << " " << dt << std::endl;
            //                }
            //                //            cpudelay -= dt * vtsHelper::tsscaler;
            //                prevstamp = ae->stamp;

            //                if (!stampChecked){
            //                    stampInit = ae->stamp * vtsHelper::tsscaler;
            //                    timeInit = yarp::os::Time::now();
            //                    stampChecked = true;
            //                }
            //                if(dt < 0.0) stampInit -= vtsHelper::max_stamp * vtsHelper::tsscaler;

            ev::temporalSurface *cSurf;
            if(ae->getChannel() == 0)
                cSurf = surfaceleft;
            else
                cSurf = surfaceright;
            semaphore.lock();
            cSurf->fastAddEvent(*qi);
            semaphore.unlock();

            //            if(cpudelay < 0.0) cpudelay = 0.0;

            //            if(cpudelay <= 0.1) {
            //                t1 = yarp::os::Time::now();

            for(int k = 0; k < nthreads; k++) {

                //assign a task to a thread that is not managing a task
                //                                    if(!computeThreads[k]->taskassigned) {
                if(computeThreads[k]->isSuspended()) {
                    //                        patch.clear();
                    //                    cSurf->getSurf(patch, windowRad);

                    computeThreads[k]->assignTask(cSurf, &yarpstamp);
//                    computeThreads[k]->assignTask(&patch, &yarpstamp);

                    //                        computeThreads[k]->assignTask(cSurf, yarpstamp);
                    countProcessed++;
                    break;
                }
                //                }

                //                //time it took to process
                //                cpudelay += (yarp::os::Time::now() - t1);
            }
            //        }

            //                time += dt;
            //                stamp1 = unwrapper(prevstamp) * vtsHelper::tsscaler;
            //                //        std::cout << stamp1 << std::endl;
            //                //        std::cout << yarp::os::Time::now() - timeInit << " " << stamp1 - stampInit << " " << (yarp::os::Time::now() - timeInit) - (stamp1 - stampInit) << std::endl;

            //            }
        }

        if(debugPort.getOutputCount()) {
            yarp::os::Bottle &scorebottleout = debugPort.prepare();
            scorebottleout.clear();
            //            scorebottleout.addDouble((yarp::os::Time::now() - timeInit) - time * vtsHelper::tsscaler); //(stamp1 - stampInit));
            //            scorebottleout.addDouble(dtneg);
            //            scorebottleout.addInt(q->size());
            scorebottleout.addDouble((double)countProcessed/q->size());
            //            scorebottleout.addDouble(cpudelay);
            debugPort.write();
        }

        allocatorCallback.scrapQ();

    }

}

//bool vCornerThread::detectcorner(ev::vQueue patch, int x, int y)
//{

//    //set the final response to be centred on the current event
//    convolution.setResponseCenter(x, y);

//    //update filter response
//    for(unsigned int i = 0; i < patch.size(); i++)
//    {
//        //events the patch
//        auto vi = ev::is_event<ev::AE>(patch[i]);
//        convolution.applysobel(vi);

//    }
//    convolution.applygaussian();
//    double score = convolution.getScore();

////    std::cout << score << std::endl;

//    //reset responses
//    convolution.reset();

//    //if score > thresh tag ae as ce
//    return score > thresh;

//}

/*////////////////////////////////////////////////////////////////////////////*/
//threaded computation
/*////////////////////////////////////////////////////////////////////////////*/
vComputeThread::vComputeThread(int sobelsize, int windowRad, double sigma, double thresh, unsigned int qlen, collectorPort *outthread, yarp::os::Mutex *semaphore)
{
    this->sobelsize = sobelsize;
    this->windowRad = windowRad;
    this->sigma = sigma;
    this->thresh = thresh;
    this->qlen = qlen;
    int gaussiansize = 2*windowRad + 2 - sobelsize;
    convolution.configure(sobelsize, gaussiansize);
    convolution.setSobelFilters();
    convolution.setGaussianFilter(sigma);
    this->outthread = outthread;
    this->semaphore = semaphore;

    mutex = new yarp::os::Semaphore(0);
    suspended = true;
//    taskassigned = false;

//    dataready.lock();

}

//void vComputeThread::assignTask(vQueue *cPatch, yarp::os::Stamp *ystamp)
void vComputeThread::assignTask(temporalSurface *cSurf, yarp::os::Stamp *ystamp)
//void vComputeThread::assignTask(temporalSurface *cSurf, yarp::os::Stamp ystamp)
{
    cSurf_p = cSurf;
//    cPatch_p = cPatch;
    ystamp_p = ystamp;
    wakeup();
//    taskassigned = true;

//    patch.clear();
//    cSurf->getSurf(patch, windowRad);
//    aep = is_event<AE>(cSurf->getMostRecent());
//    this->ystamp = ystamp;
//    taskassigned = true;
}

void vComputeThread::suspend()
{
    suspended = true;
}

void vComputeThread::wakeup()
{
    suspended = false;
    mutex->post();
}

bool vComputeThread::isSuspended()
{
    return suspended;
}

void vComputeThread::run()
{
//    double tstart = yarp::os::Time::now();
//    double counter = 0.0;
    while(true) {

        //if no task is assigned, wait
//        if(!taskassigned) {
        if(suspended) {
//            yarp::os::Time::delay(0.00001);
            mutex->wait();
        }
        else {

            patch.clear();
            semaphore->lock();
            cSurf_p->getSurf(patch, windowRad);
            semaphore->unlock();

            auto aep = is_event<AE>(cSurf_p->getMostRecent());
//            auto aep = is_event<AE>(cPatch_p->back());
            if(detectcorner(aep->x, aep->y)) {
                auto ce = make_event<LabelledAE>(aep);
                ce->ID = 1;
                outthread->pushevent(ce, *ystamp_p);
//                outthread->pushevent(ce, ystamp);
            }
            suspend();

//            taskassigned = false;
//            counter += (yarp::os::Time::now() - tstarttask);
//            double total = yarp::os::Time::now() - tstart;
//            std::cout << this->getKey() << " " << (counter/total) * 100 << std::endl;

//            std::cout << " task completed " << this->getKey() << std::endl;
        }

    }
}

void vComputeThread::onStop()
{
//    stopping = true;
    wakeup();
//    join();
//    ((ThreadImpl*)implementation)->close();
//    return true;
}

//void vComputeThread::setData(temporalSurface *cSurf, yarp::os::Stamp ystamp)
////void vComputeThread::setData(historicalSurface *cSurf, yarp::os::Stamp ystamp)
//{
////    patch.clear();
////    cSurf->getSurfaceN(patch, 0, qlen, windowRad);

//    patch.clear();
////    patch = cSurf->getSurf(windowRad); // cSurf->getSurf_Clim(qlen, windowRad);
//    cSurf->getSurf(patch, windowRad);
//    aep = is_event<AE>(cSurf->getMostRecent());
//    this->ystamp = ystamp;
//}

//void vComputeThread::run()
//{

//    if(patch.size() == 0) return;

////    auto aep = is_event<AE>(patch.front());
//    if(detectcorner(aep->x, aep->y)) {
//        auto ce = make_event<LabelledAE>(aep);
//        ce->ID = 1;
//        outthread->pushevent(ce, ystamp);
//    }

//}

//void vComputeThread::setData(temporalSurface *cSurf, yarp::os::Stamp ystamp)
//{
//    processing.lock();

//    patch.clear();
//    cSurf->getSurf(patch, windowRad);
//    aep = is_event<AE>(cSurf->getMostRecent());
//    this->ystamp = ystamp;

//    processing.unlock();
//    dataready.unlock();
//}

//void vComputeThread::run()
//{
//    while(true) {
//        dataready.lock();
//        processing.lock();

//        if(detectcorner(aep->x, aep->y)) {
//            auto ce = make_event<LabelledAE>(aep);
//            ce->ID = 1;
//            outthread->pushevent(ce, ystamp);
//        }
//        processing.unlock();
//    }

//}

bool vComputeThread::detectcorner(int x, int y)
{

    if(patch.size() == 0) return false;

    //set the final response to be centred on the current event
    convolution.setResponseCenter(x, y);

    //update filter response
//    std::cout << "computing before " << this->getKey() << std::endl;
    for(unsigned int i = 0; i < patch.size(); i++)
    {
        //events the patch
        auto vi = is_event<AE>(patch[i]);
        convolution.applysobel(vi);

    }
    convolution.applygaussian();
    double score = convolution.getScore();

//    std::cout << "computing " << this->getKey() << std::endl;

//    std::cout << score << std::endl;

    //reset responses
    convolution.reset();

    //if score > thresh tag ae as ce
    return score > thresh;

}
