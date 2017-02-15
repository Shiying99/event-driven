/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
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

#include "vAttention.h"
#include <iomanip>
#include <fstream>

using namespace yarp::math;
using namespace std;

bool vAttentionModule::configure(yarp::os::ResourceFinder &rf) {
    //set the name of the module
    std::string moduleName =
            rf.check("name", yarp::os::Value("vAttention")).asString();
    yarp::os::RFModule::setName(moduleName.c_str());

    bool strictness = rf.check("strictness") &&
                      rf.check("strictness", yarp::os::Value(true)).asBool();

    /* attach a port of the same name as the module (prefixed with a /) to the module
     so that messages received from the port are redirected to the respond method */

    std::string handlerPortName = "/";
    handlerPortName += getName();         // use getName() rather than a literal

    if (!handlerPort.open(handlerPortName.c_str())) {
        std::cout << getName() << ": Unable to open port " << handlerPortName << std::endl;
        return false;
    }

    attach(handlerPort);                  // attach to port

    /* set parameters */
    int sensorSize = rf.check("sensorSize", yarp::os::Value(128)).asInt();
    double tau = rf.check("tau", yarp::os::Value(100000000000.0)).asDouble();
    double thrSal = rf.check("thr", yarp::os::Value(20)).asDouble();
    string filtersPath = rf.check("filtersPath", yarp::os::Value("../../src/processing/vAttention/filters/")).asString();

    /* create the thread and pass pointers to the module parameters */
    attManager = new vAttentionManager(sensorSize, tau, thrSal, filtersPath);

    return attManager->open(moduleName, strictness);

}

bool vAttentionModule::interruptModule() {
    attManager->interrupt();
    yarp::os::RFModule::interruptModule();
    return true;
}

bool vAttentionModule::close() {
    attManager->close();
    delete attManager;
    yarp::os::RFModule::close();
    return true;
}

bool vAttentionModule::updateModule() {
    return true;
}

double vAttentionModule::getPeriod() {
    return 1;
}

bool vAttentionModule::respond(const yarp::os::Bottle &command, yarp::os::Bottle &reply) {
    std::string helpMessage = std::string(getName().c_str()) +
                              " commands are: \n" +
                              "help \n" +
                              "quit \n";
    reply.clear();

    if (command.get(0).asString() == "quit") {
        reply.addString("quitting");
        return false;
    } else if (command.get(0).asString() == "help") {
        std::cout << helpMessage;
        reply.addString("ok");
    }

    return true;
}

/******************************************************************************/
//vAttentionManager
/******************************************************************************/

void vAttentionManager::load_filter(const string filename, yarp::sig::Matrix &filterMap, int &filterSize) {

    //Opening filter file.
    ifstream file;
    file.open(filename.c_str(), ios::in | ios::out);
    if (!file.is_open()) {
        std::cerr << "Could not open filter file " << filename << std::endl;
    }

    string line;
    int r = 0;
    int c = 0;

    //File is parsed line by line. Values are separated by spaces
    while (!std::getline(file, line, '\n').eof()) {
        istringstream reader(line);
        string::const_iterator i = line.begin();
        if (line.empty())
            continue;
        c = 0;

        while (!reader.eof()) {

            double val;
            reader >> val;

            //Resize the map to contain new values if necessary
            if (r + 1 > filterMap.rows()) {
                filterMap.resize(r + 1, filterMap.cols());
            }
            if (c + 1 > filterMap.cols()) {
                filterMap.resize(filterMap.rows(), c + 1);
            }

            filterMap(r, c) = val;
            c++;
        }
        r++;
    }

    //The returned filterSize is updated with the maximum dimension of the filter
    int maxDimension = std::max(filterMap.rows(), filterMap.cols());
    filterSize = max(filterSize, maxDimension);
}

vAttentionManager::vAttentionManager(int sensorSize, double tau, double thrSal, std::string &filtersPath) {
    this->sensorSize = sensorSize;
    this->tau = tau;
    this->thrSal = thrSal;

    normSal = thrSal / 255;


    /** Gaussian Filter computation**/

    uniformFilterMap.resize(21,21);
    uniformFilterMap = 1;

    double sigma = 4;
    double sigmaBig = 5;

    double A = 1/(sqrt(2*M_PI) * sigma);
    double Abig = 1/(sqrt(2*M_PI) * sigmaBig);

    generateGaussianFilter(gaussianFilterMap, A, sigma, sigma, filterSize, 15);
    generateGaussianFilter(bigGaussianFilterMap, Abig, sigmaBig, sigmaBig, filterSize, 15);

//    generateGaussianFilter(vertGaussianFilterMap, A, sigma/3, sigma, filterSize, 15);
//    generateGaussianFilter(vertBigGaussianFilterMap, Abig, sigmaBig/3, sigmaBig, filterSize, 15);



    DOGFilterMap = gaussianFilterMap - bigGaussianFilterMap;
//    vertDOGFilterMap = vertGaussianFilterMap - vertBigGaussianFilterMap;
//    DOGFilterMap *= 150;
    negDOGFilterMap = DOGFilterMap * (-1.0);
//    vertNegDOGFilterMap = vertDOGFilterMap * (-1.0);

//    generateGaussianFilter(verticalGaussianFilterMap, 1, 15, 0.5, filterSize, 20);
//    generateGaussianFilter(horizontalGaussianFilterMap, 1, 0.5, 15, filterSize, 20);

    generateGaborFilter(orient0FilterMap, 21, 5, 0, filterSize);
//    generateGaborFilter(orient45FilterMap, 21, 5, 45, filterSize);
    generateGaborFilter(orient90FilterMap, 21, 5, 90, filterSize);
//    generateGaborFilter(orient135FilterMap, 21, 5, 135, filterSize);

    this->salMapPadding = filterSize / 2;

    //for speed we predefine the memory for some matrices
    //The saliency map is bigger than the image by the maximum size among the loaded filters
    salMapLeft = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    salMapRight = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    orient0FeatureMap = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    convolvedOrient0FeatureMap = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    convolvedOrient90FeatureMap = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    orient45FeatureMap = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    orient90FeatureMap = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    orient135FeatureMap = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    uniformFeatureMap = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    gaussianFeatureMap = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    DOGFeatureMap = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);

    activationMap = yarp::sig::Matrix(sensorSize + filterSize, sensorSize + filterSize);
    timeMap.resize(sensorSize,sensorSize);

    timeMap.zero();
    salMapLeft.zero();
    salMapRight.zero();
    activationMap.zero();
}


void vAttentionManager::generateGaussianFilter(yarp::sig::Matrix &filterMap, double A, double sigmaX, double sigmaY,
                                               int &filterSize, int gaussianFilterSize) {
    //Resize to desired size
    filterMap.resize(gaussianFilterSize, gaussianFilterSize);

    //Generate gaussian filter
    for (int r = 0; r < gaussianFilterSize; r++) {
        for (int c = 0; c < gaussianFilterSize; c++) {
            double center = gaussianFilterSize / 2;
            double rDist = r - center;
            double cDist = c - center;
            filterMap(r, c) =A * exp(-(pow(rDist, 2) / (2 * pow(sigmaX, 2)) + pow(cDist, 2) / (2 * pow(sigmaY, 2))));
        }
    }
    //Update filterSize to comply with new filter
    filterSize = max(gaussianFilterSize, filterSize);
}

void vAttentionManager::generateGaborFilter(yarp::sig::Matrix &filterMap, int gaborFilterSize, double B, int theta,
                                            int &filterSize)
{

    double th_r = (double)theta *M_PI/180.0;
    filterMap.resize(gaborFilterSize, gaborFilterSize);
    filterMap.zero();
    double sigma =(double) gaborFilterSize / 4.0;
    double f = 1.0 /(double) gaborFilterSize;
    for (double x = 0; x < gaborFilterSize; ++x) {
        for (double y = 0; y < gaborFilterSize; ++y) {
            double  x1 = x - (double)gaborFilterSize/2.0;
            double  y1 = y - (double)gaborFilterSize/2.0;

            double real =B * exp(-(pow(x1,2.0)+pow(y1,2.0))/(2*pow(sigma,2))) * cos(2.0*M_PI*f*(x1*cos(th_r)+y1*sin(th_r)));
            filterMap(x,y) = real;

        }
    }

    filterSize = max(gaborFilterSize,filterSize);

}
bool vAttentionManager::open(const std::string moduleName, bool strictness) {
    this->strictness = strictness;
    if (strictness) {
        std::cout << "Setting " << moduleName << " to strict" << std::endl;
        this->setStrict();
    }
    this->useCallback();

    // why is the input port treated differently???? both in open and close
    std::string inPortName = "/" + moduleName + "/vBottle:i";
    bool check = BufferedPort<emorph::vBottle>::open(inPortName);

    if (strictness) outPort.setStrict();
    std::string outPortName = "/" + moduleName + "/vBottle:o";
    check &= outPort.open(outPortName);

    outPortName = "/" + moduleName + "/salMapLeft:o";
    check &= outSalMapLeftPort.open(outPortName);

    outPortName = "/" + moduleName + "/activationMap:o";
    check &= outActivationMapPort.open(outPortName);

    outPortName = "/" + moduleName + "/orient0:o";
    check &= outorient0Port.open(outPortName);

    outPortName = "/" + moduleName + "/orient90:o";
    check &= outorient90Port.open(outPortName);

    outPortName = "/" + moduleName + "/orient45:o";
    check &= outorient45Port.open(outPortName);

    outPortName = "/" + moduleName + "/orient135:o";
    check &= outorient135Port.open(outPortName);

    outPortName = "/" + moduleName + "/gaussian:o";
    check &= outgaussianPort.open(outPortName);

    outPortName = "/" + moduleName + "/DOG:o";
    check &= outDOGPort.open(outPortName);

    outPortName = "/" + moduleName + "/uniform:o";
    check &= outuniformPort.open(outPortName);

    std::cout << "\ninitialisation correctly ended" << std::endl;

    return check;
}

void vAttentionManager::close() {
    //close ports
    outPort.close();
    yarp::os::BufferedPort<emorph::vBottle>::close();
    outSalMapLeftPort.close();
    outActivationMapPort.close();
}

void vAttentionManager::interrupt() {
    //pass on the interrupt call to everything needed
    outPort.interrupt();
    yarp::os::BufferedPort<emorph::vBottle>::interrupt();
    outSalMapLeftPort.interrupt();
    outActivationMapPort.interrupt();
}

void vAttentionManager::onRead(emorph::vBottle &bot) {
    numIterations ++;
    /* get the event queue in the vBottle bot */
    emorph::vQueue q = bot.get<emorph::AddressEvent>();
    q.sort(true);

    unsigned long int t;
    unsigned long int dt;

    int x,y;
    for (emorph::vQueue::iterator qi = q.begin(); qi != q.end(); qi++) {
        emorph::AddressEvent *aep = (*qi)->getAs<emorph::AddressEvent>();
        if (!aep) continue;

        x = aep->getX();
        y = aep->getY();
        t = unwrap(aep->getStamp());
        dt = t - timeMap(x,y);
        timeMap(x,y) = t;
        // --- increase energy of saliency map  --- //
        if (aep->getChannel() == 0) {
            //TODO handle left and right salMap
//            updateMap(DOGFeatureMap, DOGFilterMap, aep, dt);
            updateMap(orient0FeatureMap, orient0FilterMap, aep, dt);
//            updateMap(orient45FeatureMap, orient45FilterMap, aep, dt);
            updateMap(orient90FeatureMap, orient90FilterMap, aep, dt);
//            updateMap(orient135FeatureMap, orient135FilterMap, aep, dt);
//            updateMap(uniformFeatureMap,uniformFilterMap, aep, dt);
//            updateMap(gaussianFeatureMap,gaussianFilterMap, aep, dt);
        } else {
            //TODO
        }
    }

//    pool(DOGFeatureMap);
    pool(orient0FeatureMap);
    convolve(orient0FeatureMap, convolvedOrient0FeatureMap, negDOGFilterMap);
    pool(convolvedOrient0FeatureMap);
    pool(orient90FeatureMap);
    convolve(orient90FeatureMap, convolvedOrient90FeatureMap, negDOGFilterMap);
    pool(convolvedOrient90FeatureMap);
    normaliseMap(orient0FeatureMap,normalizedOrient0FeatureMap);
//    normaliseMap(orient45FeatureMap,normalizedOrient45FeatureMap);
    normaliseMap(orient90FeatureMap,normalizedOrient90FeatureMap);
//    normaliseMap(orient135FeatureMap,normalizedOrient135FeatureMap);
//    normaliseMap(gaussianFeatureMap,normalizedGaussianFeatureMap);
//    normaliseMap(DOGFeatureMap,normalizedDOGFeatureMap);
//    normaliseMap(uniformFeatureMap,normalizedUniformFeatureMap);


    // ---- adding the event to the output vBottle if it passes thresholds ---- //

    /*
     if(pFeaOn[posFeaImage] > thrOn) {
     std::cout << "adding On event to vBottle" << std::endl;

     emorph::AddressEvent ae = *aep;
     ae.setPolarity(1);
     ae.setX(xevent);
     ae.setY(yevent);

     outBottle->addEvent(ae);

     }
     else if(pFeaOff[posFeaImage] < thrOff) {
     std::cout << "adding Off event to vBottle" << std::endl;
     emorph::AddressEvent ae = *aep;
     ae.setPolarity(0);
     ae.setX(xevent);
     ae.setY(yevent);

     outBottle->addEvent(ae);

     }
    emorph::vBottle &outBottle = outPort.prepare();
    outBottle.clear();
    yarp::os::Stamp st;
    this->getEnvelope(st);
    outPort.setEnvelope(st);
    // --- writing vBottle on buffered output port
    if (strictness) {
        outPort.writeStrict();
    } else {
        outPort.write();
    }
     */

    //  --- convert to images for display --- //

    yarp::sig::ImageOf<yarp::sig::PixelBgr> &imageLeft = outSalMapLeftPort.prepare();
    yarp::sig::ImageOf<yarp::sig::PixelBgr> &imageActivation = outActivationMapPort.prepare();
//    yarp::sig::ImageOf<yarp::sig::PixelBgr> &imageUniform = outuniformPort.prepare();
    yarp::sig::ImageOf<yarp::sig::PixelBgr> &imageOrient0 = outorient0Port.prepare();
    yarp::sig::ImageOf<yarp::sig::PixelBgr> &imageOrient45 = outorient45Port.prepare();
    yarp::sig::ImageOf<yarp::sig::PixelBgr> &imageOrient90 = outorient90Port.prepare();
    yarp::sig::ImageOf<yarp::sig::PixelBgr> &imageOrient135 = outorient135Port.prepare();
//    yarp::sig::ImageOf<yarp::sig::PixelBgr> &imageGaussian = outgaussianPort.prepare();
//    yarp::sig::ImageOf<yarp::sig::PixelBgr> &imageDOG = outDOGPort.prepare();

    convertToImage(orient0FeatureMap,imageOrient0);

    convertToImage(convolvedOrient0FeatureMap,imageOrient45);
    convertToImage(convolvedOrient90FeatureMap,imageOrient135);
    salMapLeft =convolvedOrient0FeatureMap + convolvedOrient90FeatureMap;
//    salMapLeft *= 5000;
    computeAttentionPoint(salMapLeft);

    //line commented for debug purposes
//    convertToImage(orient45FeatureMap,imageOrient45);
//    convertToImage(orient135FeatureMap,imageOrient135);


    convertToImage(orient90FeatureMap,imageOrient90);
//    convertToImage(gaussianFeatureMap,imageGaussian);
//    convertToImage(DOGFeatureMap,imageDOG);
//    convertToImage(uniformFeatureMap,imageUniform);
    convertToImage(salMapLeft, imageLeft, attPointY, attPointX);
    convertToImage(activationMap, imageActivation, attPointY, attPointX);

    // --- writing images of left and right saliency maps on output port
    if (outSalMapLeftPort.getOutputCount()) {
        outSalMapLeftPort.write();
    }
    if (outActivationMapPort.getOutputCount()) {
        outActivationMapPort.write();
    }
    if (outorient135Port.getOutputCount()) {
        outorient135Port.write();
    }
    if (outorient0Port.getOutputCount()) {
        outorient0Port.write();
    }
    if (outorient45Port.getOutputCount()) {
        outorient45Port.write();
    }
    if (outorient90Port.getOutputCount()) {
        outorient90Port.write();
    }
//    if (outgaussianPort.getOutputCount()) {
//        outgaussianPort.write();
//    }
//    if (outDOGPort.getOutputCount()) {
//        outDOGPort.write();
//    }
//    if (outuniformPort.getOutputCount()) {
//        outuniformPort.write();
//    }

}

void vAttentionManager::convertToImage(const yarp::sig::Matrix &map, yarp::sig::ImageOf<yarp::sig::PixelBgr> &image,
                                       int rMax,
                                       int cMax) {

    /*prepare output vBottle with images */
    image.resize(sensorSize, sensorSize);
    image.setTopIsLowIndex(true);
    image.zero();

    int shiftedR, shiftedC;
    for (int r = sensorSize; r > 0; r--) {
        for (int c = 0; c < sensorSize; c++) {
            yarp::sig::PixelBgr pixelBgr;

//          Coordinates of saliency map are shifted by salMapPadding wrt the image
            shiftedR = r + salMapPadding;
            shiftedC = c + salMapPadding;

//            double pixelValue = std::min(map(shiftedR, shiftedC), thrSal);
//
//            //Normalize to maximum pixel bgr value 255
//            pixelValue /= normSal;

            double pixelValue = map(shiftedR, shiftedC);
            //Attention point is highlighted in red, negative values in blue, positive in green
            if (shiftedR == rMax && shiftedC == cMax) {
                pixelBgr.r = 255;
//                drawSquare(image, r + salMapPadding, c + salMapPadding, pixelBgr);
            } else if (pixelValue <= 0) {
                pixelBgr.b = std::min (fabs(pixelValue),255.0);
            } else {
                pixelBgr.g = std::min(fabs(pixelValue), 255.0);
            }

            image(c, sensorSize - r) = pixelBgr;
        }
    }
}

void vAttentionManager::drawSquare( yarp::sig::ImageOf<yarp::sig::PixelBgr> &image, int r, int c,
                                    yarp::sig::PixelBgr &pixelBgr)  {
    int squareSize = 2;
    for (int i = -squareSize; i <= squareSize; ++i) {
        for (int j = -squareSize; j <= squareSize; ++j) {
            if ((r + i< (image.height() -1)) && r + i>= 0)
                if ((c +j< (image.width() -1)) && c + j>= 0)
                    image(r +i,c+j) = pixelBgr;
        }
    }
}

void vAttentionManager::updateMap(yarp::sig::Matrix &map, const yarp::sig::Matrix &filterMap, emorph::AddressEvent *aep,
                                  unsigned long int dt) {
    //Pixel coordinates are shifted to match with the location in the saliency map

    int filterRows = filterMap.rows();
    int filterCols = filterMap.cols();

    // unmask event: get x, y
    int r = aep->getX();
    int c = aep->getY();
    // ---- increase energy in the location of the event ---- //

    for (int rf = 0; rf < filterRows; rf++) {
        for (int cf = 0; cf < filterCols; cf++) {
            map(r + rf, c + cf) += filterMap(rf, cf);
            map(r + rf, c + cf) = min(map(r + rf, c + cf), 2000.0);
            map(r + rf, c + cf) = max(map(r + rf, c + cf), -2000.0);
        }
    }

    decayMap(map,dt);
}

void vAttentionManager::printMap(const yarp::sig::Matrix &map) {
    for (int r = 0; r < map.rows(); r++) {
        for (int c = 0; c < map.cols(); c++) {
            std::cout << std::setprecision(2) << map(r, c) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void vAttentionManager::decayMap(yarp::sig::Matrix &map, unsigned long int dt) {
    double decayFactor = exp(-((double) dt) / tau);
    map*= decayFactor;
}

//void vAttentionManager::normaliseMap(yarp::sig::Matrix &map) {
//    double min;
//    double max;
//
//    min = map(0, 0);
//    max = min;
//
//    // ---- find max and min values of saliency map ---- //
//    for (int r = 0; r < map.rows(); r++) {
//        for (int c = 0; c < map.cols(); c++) {
//            if (map(r, c) > max) {
//                max = map(r, c);
//            }
//            if (map(r, c) < min) {
//                min = map(r, c);
//            }
//        }
//    }
//
//    if (max == min) {
//        return;
//    }
//    double value;
//    double salValue;
//    // ---- normalise ---- //
//    for (int r = 0; r < map.rows(); r++) {
//        for (int c = 0; c < map.cols(); c++) {
//            salValue = map(r,c);
//            value = (map(r, c) - min) / (max - min);
//            map(r, c) = value;
//        }
//    }
//}


void vAttentionManager::normaliseMap(const yarp::sig::Matrix &map, yarp::sig::Matrix &normalisedMap) {
    double totalEnergy;
    normalisedMap = map;
    for (int r = 0; r < map.rows(); ++r) {
        for (int c = 0; c < map.cols(); ++c) {
            totalEnergy += map(r,c);
        }
    }
    normalisedMap /= totalEnergy;
}

void vAttentionManager::computeAttentionPoint(const yarp::sig::Matrix &map) {

    maxInMap(map);
    activationMap(attPointY,attPointX) ++;
    maxInMap(activationMap);
    activationMap *= 0.95;
}

void vAttentionManager::maxInMap(const yarp::sig::Matrix &map) {
    double max = 0;
    attPointX = 0;
    attPointY = 0;
    for (int r = 0; r < map.rows(); r++) {
        for (int c = 0; c < map.cols(); c++) {
            if (map(r, c) > max) {
                max = map(r, c);
                attPointY = r;
                attPointX = c;
            }
        }
    }
}

void vAttentionManager::computeBoundingBox(const yarp::sig::Matrix &map, double threshold) {
    double internalEnergy = 0;
    double previousInternalEnergy = 0;
    int boxWidth = 0;
    int boxHeight = 0;
    int topX, topY;
    int bottomX, bottomY;
    do{

        previousInternalEnergy = internalEnergy;
        internalEnergy = 0;
        boxWidth += 5;
        boxHeight += 5;
        topY = max(attPointY - boxHeight/2,0);
        bottomY = min(attPointY + boxHeight/2, map.rows());
        topX = max(attPointX-boxWidth/2,0);
        bottomX = min (attPointX + boxWidth/2, map.cols());
        //compute energy inside the box
        for (int r = topY; r < bottomY; ++r) {
            for (int c = topX; c < bottomX; ++c) {
                internalEnergy += map(r,c);
            }
        }
    }while (internalEnergy - previousInternalEnergy < threshold);

    //TODO debug and draw bounding box
}

void vAttentionManager::convolve(const yarp::sig::Matrix &featureMap, yarp::sig::Matrix &convolvedFeatureMap,
                                 const yarp::sig::Matrix &filterMap) {
    int startRow = filterMap.rows()/2 + filterMap.rows() % 2;
    int finalRow = featureMap.rows()- filterMap.rows()/2;
    int startCol = filterMap.cols()/2 + filterMap.cols() % 2;
    int finalCol = featureMap.cols()- filterMap.cols()/2;
    convolvedFeatureMap.zero();
     for (int mapRow = startRow; mapRow < finalRow; ++mapRow) {
        for (int mapCol = startCol; mapCol < finalCol; ++mapCol) {

            for (int filterRow = 0; filterRow < filterMap.rows(); ++filterRow) {
                for (int filterCol = 0; filterCol < filterMap.cols(); ++filterCol) {
                    convolvedFeatureMap(mapRow,mapCol) += featureMap(mapRow + filterRow -startRow, mapCol + filterCol -startCol) * filterMap(filterRow, filterCol);
                }
            }

        }
    }

};

void vAttentionManager::pool(yarp::sig::Matrix &featureMap) {
    for (int i = 0; i < featureMap.rows(); ++i) {
        for (int j = 0; j < featureMap.cols(); ++j) {
            featureMap(i,j) = std::max(featureMap(i,j), 0.0);
        }
    }
}
//empty line to make gcc happy
