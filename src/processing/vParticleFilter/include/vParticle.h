#ifndef __VPARTICLE__
#define __VPARTICLE__

#include <iCub/eventdriven/all.h>
#include <yarp/sig/all.h>
#include <iomanip>

using namespace ev;

class vParticle;

void drawEvents(yarp::sig::ImageOf< yarp::sig::PixelBgr> &image, ev::vQueue &q, int currenttime, double tw = 0, bool flip = false);

void drawcircle(yarp::sig::ImageOf<yarp::sig::PixelBgr> &image, int cx, int cy, int cr, int id = 0);

void drawDistribution(yarp::sig::ImageOf<yarp::sig::PixelBgr> &image, std::vector<vParticle> &indexedlist);

class preComputedBins;

/*////////////////////////////////////////////////////////////////////////////*/
//VPARTICLETRACKER
/*////////////////////////////////////////////////////////////////////////////*/
class vParticle
{
private:

    //static parameters
    int id;
    double minlikelihood;
    double inlierParameter;
    double outlierParameter;
    double variance;
    int angbuckets;
    preComputedBins *pcb;

    //temporary parameters (on update cycle)
    double likelihood;

    double predlike;
    double theta;
    int    outlierCount;
    int    inlierCount;
    double maxtw;
    yarp::sig::Vector angdist;
    yarp::sig::Vector negdist;
    int nupdates;

    //state and weight
    double x;
    double y;
    double r;
    double tw;
    double weight;

    //timing
    unsigned long int stamp;

public:



    int score;


    vParticle();
    vParticle& operator=(const vParticle &rhs);

    //initialise etc.
    void initialiseParameters(int id, double minLikelihood, double outlierParam, double inlierParam, double variance, int angbuckets);
    void attachPCB(preComputedBins *pcb) { this->pcb = pcb; }

    void initialiseState(double x, double y, double r);
    void randomise(int x, int y, int r);

    void resetStamp(unsigned long int value);
    void resetWeight(double value);
    void resetRadius(double value);

    void printL() {

        std::cout << std::setprecision(3) << std::fixed;
        for(int i = 0; i < angbuckets; i++) {
            //if(angdist[i] > 0.5) std::cout << "1";
            //else std::cout << "0";
            std::cout << angdist[i] << " ";
        }

        std::cout << std::endl;
    }




    //update
    void predict(double sigma = -1);

    void initLikelihood();

    inline int incrementalLikelihood(int vx, int vy)
    {
        static double twopi = 2.0*M_PI;
        double dx = vx - x;
        double dy = vy - y;

        double sqrd = sqrt(pow(dx, 2.0) + pow(dy, 2.0)) - r;

        if(sqrd > inlierParameter) return nupdates;

        double phi = atan2(dy, dx);
        //double gain = fabs(cos(theta - phi));

        int a = 0.5 + (angbuckets-1) * (phi + M_PI) / twopi;

        if(sqrd > -inlierParameter) {
            angdist[a] = 1.0;
            nupdates++;
        } else {
            angdist[a] = 0.0;
        }

        return nupdates;

    }

    void concludeLikelihood(double decay);

    void updateWeightSync(double normval);

    //get
    inline int    getid() { return id; }
    inline double getx()  { return x; }
    inline double gety()  { return y; }
    inline double getr()  { return r; }
    inline double getw()  { return weight; }
    inline double getl()  { return likelihood; }
    inline double gettw() { return tw; }


};

class preComputedBins
{

private:

    yarp::sig::Matrix ds;
    yarp::sig::Matrix bs;
    int rows;
    int cols;
    int offsetx;
    int offsety;

public:

    preComputedBins()
    {
        rows = 0;
        cols = 0;
        offsetx = 0;
        offsety = 0;
    }

    void configure(int height, int width, double maxrad, int nBins)
    {
        rows = (height + maxrad) * 2 + 1;
        cols = (width + maxrad) * 2 + 1;
        offsety = rows/2;
        offsetx = cols/2;

        ds.resize(rows, cols);
        bs.resize(rows, cols);
        for(int i = 0; i < rows; i++) {
            for(int j = 0; j < cols; j++) {

                int dy = i - offsety;
                int dx = j - offsetx;

                ds(i, j) = sqrt(pow(dx, 2.0) + pow(dy, 2.0));
                bs(i, j) = (nBins-1) * (atan2(dy, dx) + M_PI) / (2.0 * M_PI);

            }
        }
    }

    double queryDistance(int dy, int dx)
    {
        dy += offsety; dx += offsetx;
//        if(dy < 0 || dy > rows || dx < 0 || dx > cols) {
//            std::cout << "preComputatedBins not large enough" << std::endl;
//            return 0.0;
//        }
        return ds(dy, dx);
    }

    int queryBinNumber(double dy, double dx)
    {
        dy += offsety; dx += offsetx;
//        if(dy < 0 || dy > rows || dx < 0 || dx > cols) {
//            std::cout << "preComputatedBins not large enough" << std::endl;
//            return 0.0;
//        }
        return (int)(bs(dy, dx) + 0.5);
    }



};


#endif
