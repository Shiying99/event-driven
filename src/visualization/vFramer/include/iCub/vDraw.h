/*
 * Copyright (C) 2010 eMorph Group iCub Facility
 * Authors: Arren Glover
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

#ifndef __vDraw__
#define __vDraw__

#include <iCub/emorph/all.h>
#include <string>
#include <opencv2/opencv.hpp>

namespace emorph
{

/**
 * @brief The vDraw class is the base class from which all vDrawers should
 * inherit. It contains the draw and getTag functions which must be overloaded,
 * and the sensor size for reference.
 */
class vDraw {

protected:

    int Xlimit;
    int Ylimit;
    int stagnantCount;
    int pTS;
    int clearThreshold;
    int twindow;

    int checkStagnancy(const emorph::vQueue &eSet) {
        if(!eSet.size()) return 0;
        if(pTS == eSet.back()->getStamp())
            stagnantCount++;
        else
            stagnantCount = 0;
        pTS = eSet.back()->getStamp();
        return stagnantCount;
    }

public:

    vDraw() : Xlimit(128), Ylimit(128), stagnantCount(0), pTS(0),
            clearThreshold(30), twindow(781250/2) {}
    virtual ~vDraw() {}

    ///
    /// \brief setLimits sets the maximum possible values of the position of
    /// events that could be drawn (mostly governed by the sensor size).
    /// \param Xlimit is the maximum x value (width)
    /// \param Ylimit is the maximium y value (height)
    ///
    void setLimits(int Xlimit, int Ylimit)
    {
        this->Xlimit = Xlimit;
        this->Ylimit = Ylimit;
    }

    void setWindow(int twindow)
    {
        this->twindow = twindow;
    }

    virtual void initialise() {}

    ///
    /// \brief draw takes an image and overlays the new visualisation textures
    /// \param canvas is the image which may or may not yet exist
    /// \param eSet is the set of events which could possibly be drawn
    ///
    virtual void draw(cv::Mat &canvas, const emorph::vQueue &eSet) = 0;

    ///
    /// \brief getTag returns the unique code for this drawing method. The
    /// arguments given on the command line must match this code exactly
    /// \return the tag code
    ///
    virtual std::string getTag() = 0;


};

/**
 * @brief The addressDraw class is the standard instance of a vDrawer that draws
 *  simple address events. The addressDraw will completely overwrite any
 * images previously drawn beforehand and thus should always be first in the
 * argument list.
 */
class addressDraw : public vDraw {

public:

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

class lifeDraw : public vDraw {

public:

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

class integralDraw : public vDraw {

private:

    cv::Mat iimage;

public:

    integralDraw();

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

class flowDraw : public vDraw {

public:

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

/**
 * @brief The clusterDrawer overlays the image with visualisation of event
 * clusters.
 */
class clusterDraw : public vDraw {

protected:

    std::map<int, emorph::ClusterEvent *> persistance;
    int stagnantCount;

public:

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

class blobDraw : public vDraw {

public:

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

class circleDraw : public vDraw {

public:

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};


class surfDraw : public vDraw {

public:

    const static int gradient = 100000;

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

class fixedDraw : public vDraw {

public:

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

class fflowDraw : public vDraw {

public:

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

class isoDraw : public vDraw {
private:

    //angles
    double theta1;
    double theta2;
    double c1, s1;
    double c2, s2;

    //image with warped square drawn
    cv::Mat baseimage;

    int tsscalar;
    int maxdt;
    int imagewidth;
    int imageheight;
    int imagexshift;
    int imageyshift;
    double scale;


public:

    void initialise();
    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

class interestDraw : public vDraw {

public:

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

class disparityDraw : public vDraw {

private:

    cv::Mat dimage;

public:

    ///
    /// \brief see vDraw
    ///
    virtual void draw(cv::Mat &image, const emorph::vQueue &eSet);

    ///
    /// \brief see vDraw
    ///
    virtual std::string getTag();

};

/**
 * @brief createDrawer returns an instance of a drawer that matches the tag
 * specified
 * @param tag is the code of the drawer
 * @return a pointer to a drawer on success. 0 if no appropiate drawer was
 * found
 */
vDraw * createDrawer(std::string tag);


} //namespace emorph



#endif


