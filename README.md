| Read the [Documentation](http://robotology.github.io/event-driven/doxygen/doc/html/index.html) | Download the [Code](https://github.com/robotology/event-driven) |

[![event-driven-icub ball](https://img.youtube.com/vi/xS-7xYRYSLc/0.jpg)](https://youtu.be/xS-7xYRYSLc)
![event-stream ball](https://github.com/robotology/event-driven/blob/gh-pages/doxygen/images/ballstream.png)
Click the thumbnail to watch the [video](https://youtu.be/xS-7xYRYSLc)!

# The event-driven YARP Project

Libraries that handle neuromorphic sensors, such as the dynamic vision sensor, installed on the iCub can be found here, along with algorithms to process the event-based data. Examples include, optical flow, corner detection and ball detection. Demo applications for controlling the robot based on event input are also found here.

## Libraries


## Modules

 * **Optical Flow** -- an estimate of object velocity in the visual plane is given by the rate at which the spatial location of events change over time. Such a signal manifests as a manifold in the spatio-temporal event space. Local velocity can be extracted by fitting planes to these manifolds. The `ev::vFlow` module converts the ED camera output `ev::AddressEvent` to `ev::FlowEvent`.
 * **Cluster Tracking** -- The movement of an object across the visual field of an ED camera produces a detailed, unbroken trace of events. Local clusters of events can be tracked by updating a tracker position as new events are observed that belong to the same trace. The spatial distribution of the events can be estimated with a Gaussian distribution. The cluster centre and distribution statistics is output from the `ev::vCluster` module as a `ev::GaussianAE` event.
 * **Corner Detection** -- using an event-driven Harris algorithm, the full event stream is filtered to contain only the events falling on the corners of objects or structure in the scene. Corner events are useful to avoid the aperture problem and to reduce the data stream to informative events for further processing. Compared to a traditional camera, the ED corner algorithm requires less processing. `ev::vCorner` converts `ev::AddressEvent` to `ev::LabelledAE`.
 * **Circle Detection** -- detection of circular shapes in the event stream can be performed using an ED Hough transform. As the camera moves on a robot, many background events clutter the detection algorithm. The `ev::vCircle` module reduces the false positive detections by using optical flow information to provide a more accurate understanding of only the most up-to-date spatial structure. `ev::vCircle` accepts `ev::AddressEvent` and `ev::FlowEvent` and outputs `ev::GaussianAE`.
 * **Particle filtering** -- probabilistic filtering is used to provide a robust tracking over time. The particle filter is robust to variations in speed of the target by also sampling within the temporal dimension. A observation likelihood function that responds to a circular shape was developed to instigate a comparison with the Hough transform. The tracking position is output as `ev::GaussianAE`. Future work involves adapting the filter to respond to different target shapes, and templates learned from data.


## Applications for the iCub Humanoid Robot



## How to Install:

1. Install [YARP](https://github.com/robotology/yarp) and [icub-contrib-common](https://github.com/robotology/icub-contrib-common) following these [instructions](http://wiki.icub.org/wiki/Linux:Installation_from_sources).
3. Clone event-driven and install as an icub-contrib module.

## References

Glover, A., and Bartolozzi C. (2016) *Event-driven ball detection and gaze fixation in clutter*. In IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS), October 2016, Daejeon, Korea. **Finalist for RoboCup Best Paper Award**

Vasco V., Glover A., and Bartolozzi C. (2016) *Fast event-based harris corner detection exploiting the advantages of event-driven cameras*. In IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS), October 2016, Daejeon, Korea.

V. Vasco, A. Glover, Y. Tirupachuri, F. Solari, M. Chessa, and Bartolozzi C. *Vergence control with a neuromorphic iCub. In IEEE-RAS International Conference on Humanoid Robots (Humanoids)*, November 2016, Mexico.





