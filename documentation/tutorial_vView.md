# Getting started with the viewer

Visualise a stream of events from the cameras / from a pre-recorded sequence.

## Description

This application demonstrates how to visualise a stream of address events either from the cameras or from a pre-recorded sequence. 
The event stream is transmitted from the cameras (/zynqGrabber/vBottle:o) to the vPepper (/vPepper/vBottle:i), that removes salt-and-pepper noise from the event stream. The filtered stream (/vPepper/vBottle:o) is sent to vFramer (/vFramer/AE:i), that converts it to a yarpview-able image. The "images" from left (/vFramer/left) and right camera (/vFramer/right) are then sent to the yarp viewers (/viewCh0 and /viewCh1).

Here is a visualisation of the instantiated modules and connections.

![vView visualization](https://github.com/robotology-playground/event-driven/blob/master/documentation/images/vView.png)

## Dependencies

No special dependencies are required, all the required modules will be executed by the application. 

To visualise events from a pre-recorded dataset, you can run *yarpdataplayer*.
Since *yarpdataplayer* opens the port with the same name as the real robot, make sure the same port is not running (or that you start an instance of the nameserver with your own namespace).

## Instantiated modules

 * **zynqGrabber** --
 * **vFramer** --
 * **vPepper** --
 * **yarpview** --

## Opened ports

* `/zynqGrabber/vBottle:o`
* `/vPepper/vBottle:i`
* `/vPepper/vBottle:o`
* `/vFramer/AE:i`
* `/vFramer/left`
* `/vFramer/right`
* `/viewCh0`
* `/viewCh1`

## How to run the application

On a console, run *yarpserver* (if not already running).

You can now run *yarpmanager*.

Inside the *Application* folder in the yarpmanager gui, you should see an entry called *vView*. Double click and open it.

Now you are ready to run the application. 

If you want to visualise events from the cameras, hit the *run* button and then *connect* on the yarpmanager gui.
