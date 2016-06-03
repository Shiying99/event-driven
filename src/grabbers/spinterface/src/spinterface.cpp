/*
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author: Chiara Bartolozzi
 * email:  chiara.bartolozzi@iit.it
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

#include "spinterface.h"
#include <map>
#include <list>
#include <math.h>

/******************************************************************************/
// vSpinInterface
/******************************************************************************/
bool vSpinInterface::configure(yarp::os::ResourceFinder &rf)
{
    //set the name of the module
    std::string moduleName =
            rf.check("name", yarp::os::Value("spinterface")).asString();
    setName(moduleName.c_str());


    /* create the thread and pass pointers to the module parameters */
    spinnio::EIEIOSender   *spinSender;
    spinnio::EIEIOReceiver *spinReceiver;

    // Create Spin sender and receiver objects


    initSpin(17895, 12346, "192.168.240.253",
                       "/home/vvasco/dev/SpiNNaker/PyNNExamples/examples/icub_test_spintest/application_generated_data_files/latest/input_output_database.db",
             &spinReceiver, &spinSender);



//    int spinPort = 17895;
//    int sendPort = 12346;
//    std::string ip = "192.168.1.1";
//    std::string databasefile =  "/home/ubuntu/VVV/TestNetwork/application_generated_data_files/latest/input_output_database.db";

//    spinReceiver  = new spinnio::EIEIOReceiver(spinPort,(char*)ip.c_str(), true, (char*)databasefile.c_str());

//    std::map<int,int> *keymap = spinReceiver->getIDKeyMap();

//    spinSender = new spinnio::EIEIOSender(sendPort,(char*)ip.c_str(), keymap);

//    spinReceiver->start();
//    spinSender->start();
//    spinSender->enableSendQueue();


    // Start the output manager thread
    //bool oSuccess = outputManager.initThread(moduleName, spinReceiver);
    //outputManager.start();
    //outputManager.run();

    //inputManager.attachEIEIOSender(spinSender);
    ioManager.attachEIEIOmodules(spinSender, spinReceiver);
    return ioManager.open(moduleName);
    //bool iSuccess = inputManager.open(moduleName);



    //return oSuccess && iSuccess;

}

/******************************************************************************/
void vSpinInterface::initSpin(int spinPort, int sendPort, std::string ip,
              std::string databasefile, spinnio::EIEIOReceiver **eir,
                              spinnio::EIEIOSender **eis)
{
    (*eir)  = new spinnio::EIEIOReceiver(spinPort,(char*)ip.c_str(), true,
                                      (char*)databasefile.c_str());

    std::map<int,int> *keymap = (*eir)->getIDKeyMap();

    (*eis) = new spinnio::EIEIOSender(sendPort,(char*)ip.c_str(), keymap);

//    (*eis) = new spinnio::EIEIOSender(sendPort,(char*)ip.c_str(), true,
//                                      (char*)databasefile.c_str());

//    std::map<int,int> *keymap = (*eis)->getIDKeyMap();

//    (*eir) = new spinnio::EIEIOReceiver(spinPort,(char*)ip.c_str(), keymap);

    (*eir)->start();
    (*eis)->start();
    (*eis)->enableSendQueue();

}

/******************************************************************************/
bool vSpinInterface::interruptModule()
{
    outputManager.stop();
    inputManager.interrupt();
    yarp::os::RFModule::interruptModule();
    return true;
}

/******************************************************************************/
bool vSpinInterface::close()
{
    outputManager.threadRelease();
    inputManager.close();
    yarp::os::RFModule::close();
    return true;
}

/******************************************************************************/
bool vSpinInterface::updateModule()
{
    return true;
}

/******************************************************************************/
double vSpinInterface::getPeriod()
{
    return 1;
}

/******************************************************************************/
// YARP - SPIN - INPUT
/******************************************************************************/
YARPspinI::YARPspinI()
{

    //here we should initialise the module
    height = 128;
    width = 128;
    downsamplefactor = 1; // 2;
    eventsin.open("eventssenttospinnaker.txt");


}

/******************************************************************************/
bool YARPspinI::open(const std::string &name)
{
    //and open the input port

    this->useCallback();

    std::string inPortName = "/" + name + "/vBottle:i";
    return yarp::os::BufferedPort<emorph::vBottle>::open(inPortName);
}

/******************************************************************************/
void YARPspinI::close()
{
    spinSender->closeSendSocket();
    delete spinSender;
    yarp::os::BufferedPort<emorph::vBottle>::close();

    //remember to also deallocate any memory allocated by this class
}

/******************************************************************************/
void YARPspinI::interrupt()
{
    yarp::os::BufferedPort<emorph::vBottle>::interrupt();

}

/**********************************************************/
void YARPspinI::attachEIEIOSender(spinnio::EIEIOSender* spinSenderPtr)
{
   spinSender = spinSenderPtr;
}

/**********************************************************/
void YARPspinI::onRead(emorph::vBottle &bot)
{
    //create event queue
    emorph::vQueue q = bot.getAll();


    for(emorph::vQueue::iterator qi = q.begin(); qi != q.end(); qi++)
    {

        emorph::AddressEvent *v = (*qi)->getAs<emorph::AddressEvent>();
        if(!v) continue;
        if(v->getChannel()) continue;

        int neuronID = (v->getY() >> downsamplefactor) *
                (width / pow(downsamplefactor, 2.0)) +
                (v->getX() >> downsamplefactor);

        eventsin << (int)(v->getX()) << " " << (int)(v->getY()) << " " << neuronID << " "
                 << (int)(v->getX() >> downsamplefactor) << " " << (int)(v->getY() >> downsamplefactor) << std::endl;

        int pixels = 16;

//        for(int i = (width - pixels)/2; i <= (width + pixels)/2; i++)
//            if(neuronID > i - (width - pixels)/2 && neuronID < i + (width - pixels)/2)

        for(int i = 0; i <= pixels; i++) {
            if(neuronID > i*width && neuronID < i*width + pixels)
            {
//                std::cout << "\n";
//                std::cout << "neuronID " << neuronID << std::endl;
//                std::cout << "Send event x " << (int)(v->getX()) << " y " << (int)(v->getY()) << std::endl;
                spinSender->addSpikeToSendQueue(neuronID);
            }
        }

    }

}

/******************************************************************************/
// YARP - SPIN - OUTPUT
/******************************************************************************/
YARPspinO::YARPspinO() : yarp::os::RateThread(1)
{

  // Do we need anything here?
    width = 128;
    height = 128;
    downsamplefactor = 1; // 2;
    eventsout.open("eventsfromspinnaker.txt");

}

/**********************************************************/

bool YARPspinO::initThread(std::string moduleName, spinnio::EIEIOReceiver *spinReceiverPtr)
{
  // Output thread initialisation
    spinReceiver = spinReceiverPtr;
    std::string outPortName = "/" + moduleName + "/vBottle:o";
    return vBottleOut.open(outPortName);
}


/**********************************************************/

void YARPspinO::run()
{

    //get the data
    int recvQueueSize = spinReceiver->getRecvQueueSize();
    if (recvQueueSize <= 0) return;
    //std::cout << "Size: " << recvQueueSize << std::endl;

    //there is data to process so prepare out outbottle port
    emorph::vBottle &outbottle = vBottleOut.prepare();
    outbottle.clear();

    emorph::AddressEvent ae;
    emorph::NeuronIDEvent ne;

    //convert the data to readable packets
    std::list<std::pair<int, int> > spikepacket =
            spinReceiver->getNextSpikePacket();

    //iterate over all spikes and add them to the bottle
    std::list<std::pair<int, int> >::iterator i;

    int rowsize = width >> downsamplefactor;
    for(i = spikepacket.begin(); i != spikepacket.end(); i++) {
        std::pair<int,int> spikeEvent = *i;
        ae.setStamp(spikeEvent.first);
//        ae.setPolarity(0);
        int y = (spikeEvent.second / rowsize) << downsamplefactor;
        int x = (spikeEvent.second % rowsize) << downsamplefactor;
        ae.setY(y);
        ae.setX(x);
//        cout << "x " << x << " y " << y << endl;
//        outbottle.addEvent(ae);
        ne.setStamp(spikeEvent.first);
        ne.setID(spikeEvent.second);

        eventsout << x << " " << y << " " << ae.getStamp() << " " << spikeEvent.second << std::endl;
        outbottle.addEvent(ne);

    }
    std::cout << std::endl;

    //send the bottle on
    vBottleOut.write();


}

/**********************************************************/

void YARPspinO::threadRelease()
{
  // Clean up thread resources
  spinReceiver->closeRecvSocket();

}

/******************************************************************************/
// YARP - SPIN - INPUT/OUTPUT
/******************************************************************************/
YARPspinIO::YARPspinIO()
{

    //here we should initialise the module
    height = 128;
    width = 128;
    downsamplefactor = 4;

    spinSender = 0;
    spinReceiver = 0;
    //eventsin.open("eventssenttospinnaker.txt");

}

/******************************************************************************/
bool YARPspinIO::open(const std::string &name)
{
    //and open the input port

    this->useCallback();

    if(!yarp::os::BufferedPort<emorph::vBottle>::open("/" + name + "/vBottle:i"))
        return false;
    if(!vBottleOut.open("/" + name + "/vBottle:o"))
        return false;

    return true;
}

/******************************************************************************/
void YARPspinIO::close()
{
    if(spinReceiver) spinReceiver->closeRecvSocket();
    if(spinSender) spinSender->closeSendSocket();
    delete spinSender; spinSender = 0;
    delete spinReceiver; spinReceiver = 0;
    vBottleOut.close();
    yarp::os::BufferedPort<emorph::vBottle>::close();

    //remember to also deallocate any memory allocated by this class
}

/******************************************************************************/
void YARPspinIO::interrupt()
{
    vBottleOut.interrupt();
    yarp::os::BufferedPort<emorph::vBottle>::interrupt();
}

/**********************************************************/
void YARPspinIO::attachEIEIOmodules(spinnio::EIEIOSender* spinSenderPtr,
                                    spinnio::EIEIOReceiver *spinReceiverPtr)
{
    spinReceiver = spinReceiverPtr;
    spinSender = spinSenderPtr;
}

/**********************************************************/
void YARPspinIO::onRead(emorph::vBottle &inbottle)
{
    //create event queue
    yarp::os::Stamp yts;
    this->getEnvelope(yts);

    emorph::vBottle &outbottle = vBottleOut.prepare();
    outbottle = inbottle;

    emorph::vQueue q = inbottle.getAll();
    if(!q.size()) {
        std::cerr << "Callback function received no packets?" << std::endl;
        return;
    }

    int latestts = q.back()->getStamp();

    //first send on our packets we have read
    int count1 = 0;
    for(emorph::vQueue::iterator qi = q.begin(); qi != q.end(); qi++)
    {

        emorph::AddressEvent *v = (*qi)->getAs<emorph::AddressEvent>();
        if(!v) continue;
        if(v->getChannel()) continue;
        if(v->getX() % downsamplefactor != 0) continue;
        if(v->getY() % downsamplefactor != 0) continue;

        int neuronID = (v->getY() >> downsamplefactor) *
                (width / pow(downsamplefactor, 2.0)) +
                (v->getX() >> downsamplefactor);

        //eventsin << (int)(v->getX()) << " " << (int)(v->getY()) << " " << neuronID << std::endl;
        //std::cout << "x " << (int)v->getX() << " y " << (int)v->getY() << std::endl;
        spinSender->addSpikeToSendQueue(neuronID);
        count1++;

    }
    std::cout << "sent " << count1 << " spikes" << std::endl;

    //and then see if there are spikes to receive
    //int recvQueueSize = spinReceiver->getRecvQueueSize();

    //there is data to process so prepare out outbottle port
    emorph::ClusterEventGauss gaborevent;

    if(spinReceiver->getRecvQueueSize()) {
        std::cout << "received spikes back" << std::endl  << std::endl;
        //convert the data to readable packets
        std::list<std::pair<int, int> > spikepacket =
                spinReceiver->getNextSpikePacket();

        //iterate over all spikes and add them to the bottle
        std::list<std::pair<int, int> >::iterator i;
        for(i = spikepacket.begin(); i != spikepacket.end(); i++) {
            std::pair<int,int> spikeEvent = *i;
            std::cout << spikeEvent.second << " ";
            gaborevent.setStamp(latestts);
            gaborevent.setPolarity(0);
            gaborevent.setXCog(64);
            gaborevent.setYCog(64);
            gaborevent.setID(spikeEvent.second);
            if(spikeEvent.second == 0)  {
                gaborevent.setXYSigma(0);
                gaborevent.setYSigma2(5);
                gaborevent.setXSigma2(5);
            } else {
                gaborevent.setXYSigma(1);
                gaborevent.setYSigma2(5);
                gaborevent.setXSigma2(5);
            }
            gaborevent.setXVel(0);
            gaborevent.setYVel(0);
            outbottle.addEvent(gaborevent);
        }
        std::cout << std::endl;

    }

    //send the bottle on
    if(vBottleOut.getOutputCount()) {
        vBottleOut.setEnvelope(yts);
        vBottleOut.write();
    }

}


//empty line to make gcc happy
