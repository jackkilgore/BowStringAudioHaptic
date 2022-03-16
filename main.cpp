//==============================================================================
/*
    Software License Agreement (BSD License)
    Copyright (c) 2003-2016, CHAI3D.
    (www.chai3d.org)

    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.

    * Neither the name of CHAI3D nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE. 

    \author    <http://www.chai3d.org>
    \author    Francois Conti
    \version   $MAJOR.$MINOR.$RELEASE $Rev: 1869 $
*/
//==============================================================================

//------------------------------------------------------------------------------
#include "chai3d.h"
#include "utility.hpp"
#include "network.hpp"
//------------------------------------------------------------------------------
#include <GLFW/glfw3.h>
#include <thread>

//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled 
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;


//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera to render the world in the window display
cCamera* camera;

// a light source to illuminate the objects in the world
cDirectionalLight *light;

// a small sphere (cursor) representing the haptic device 
cShapeSphere* cursor;

// a line representing the velocity vector of the haptic device
cShapeLine* velocity;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice;

// a label to display the haptic device model
cLabel* labelHapticDeviceModel;

// a label to display the position [m] of the haptic device
cLabel* labelHapticDevicePosition;

// a global variable to store the position [m] of the haptic device
cVector3d hapticDevicePosition;

// a font for rendering text
cFontPtr font;

// a label to display the rate [Hz] at which the simulation is running
cLabel* labelRates;

// a flag for using damping (ON/OFF)
bool useDamping = false;

// a flag for using force field (ON/OFF)
bool useForceField = false;

// a flag to indicate if the haptic simulation currently running
bool simulationRunning = false;

// a flag to indicate if the haptic simulation has terminated
bool simulationFinished = true;

// a frequency counter to measure the simulation graphic rate
cFrequencyCounter freqCounterGraphics;

// a frequency counter to measure the simulation haptic rate
cFrequencyCounter freqCounterHaptics;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = NULL;

// current width of window
int width  = 0;

// current height of window
int height = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;

// root resource path
string resourceRoot;


//------------------------------------------------------------------------------
// HAPTIC BOW MODEL STUFF
//------------------------------------------------------------------------------

// aka z_k
float anchor_dist = 0.0f;
float ANCHOR_DIST_MAX = 0.1f;
float user_pos = 0.0f;

// aka w_k
// aka v_w
// aka u_w
float anchor_pos = 0.0f;
float anchor_vel = 0.0f;
float anchor_absorb_coeff = 0.0f;

// aka b_k
float bow_pos = 0.0f;

// aka v_s
float string_pos = 0.0f;
float string_vel = 1.0f;

// friction force
float friction_force = 0.0f;

// Downward bow pressure, p
float bow_pressure = 0.0f;


/// VECTOR STYLE
//// aka z_k
//float anchor_dist = 0.0f;
//float ANCHOR_DIST_MAX = 0.0f;
//cVector3d user_pos(0.0,0.0,0.0);
//
//// aka w_k
//// aka v_w
//// aka u_w
//cVector3d anchor_pos(0.0,0.0,0.0);
//cVector3d anchor_vel(0.0,0.0,0.0);
//float anchor_absorb_coeff = 0.0f;
//
//// aka b_k
//cVector3d bow_pos(0.0,0.0,0.0);
//
//
//// aka v_s
//cVector3d string_pos(0.0,0.0,0.0);
//cVector3d string_vel(0.0,0.0,0.0);
//
//// friction force
//cVector3d friction_force(0.0,0.0,0.0);
//
//// Downward bow pressure, p
//float bow_pressure = 1.0f;


double get_absorb_coeff(double delta_vel, double pressure) {
    double result = fabs(delta_vel * (5.0 - (4.0 * pressure))) + 0.75;
    result = pow(result, -4.0);
    return result < 1.0 ? result : 1.0;
}

//------------------------------------------------------------------------------
// NETWORK STUFF
//------------------------------------------------------------------------------
ClientOSC client_osc;
ServerOSC server_osc;
const uint32_t PORT = 7777;
const uint32_t AUDIO_OSC_PORT = 7778;


void server_recv_thread() {
    while(true)
        server_osc.recv_vec3d("/string_vel");
}


//------------------------------------------------------------------------------
// DECLARED MACROS
//------------------------------------------------------------------------------

// convert to resource path
#define RESOURCE_PATH(p)    (char*)((resourceRoot+string(p)).c_str())

//------------------------------------------------------------------------------
// AUDIO DECLARATIONS
//------------------------------------------------------------------------------

// audio device to play sound
cAudioDevice* audioDevice;

// audio buffers to store sound files
cAudioBuffer* audioBuffer1;

// audio source
cAudioSource* audioSource1;


//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window display is resized
void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when an error GLFW occurs
void errorCallback(int error, const char* a_description);

// callback when a key is pressed
void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);

// this function renders the scene
void updateGraphics(void);

// this function contains the main haptics simulation loop
void updateHaptics(void);

// this function closes the application
void close(void);


//==============================================================================
/*
    DEMO:   01-mydevice.cpp

    This application illustrates how to program forces, torques and gripper
    forces to your haptic device.

    In this example the application opens an OpenGL window and displays a
    3D cursor for the device connected to your computer. If the user presses 
    onto the user button (if available on your haptic device), the color of 
    the cursor changes from blue to green.

    In the main haptics loop function  "updateHaptics()" , the position,
    orientation and user switch status are read at each haptic cycle. 
    Force and torque vectors are computed and sent back to the haptic device.
*/
//==============================================================================

int main(int argc, char* argv[])
{
    //--------------------------------------------------------------------------
    // INITIALIZATION
    //--------------------------------------------------------------------------

    cout << endl;
    cout << "-----------------------------------" << endl;
    cout << "CHAI3D" << endl;
    cout << "Demo: 01-mydevice" << endl;
    cout << "Copyright 2003-2016" << endl;
    cout << "-----------------------------------" << endl << endl << endl;
    cout << "Keyboard Options:" << endl << endl;
    cout << "[1] - Enable/Disable potential field" << endl;
    cout << "[2] - Enable/Disable damping" << endl;
    cout << "[f] - Enable/Disable full screen mode" << endl;
    cout << "[m] - Enable/Disable vertical mirroring" << endl;
    cout << "[q] - Exit application" << endl;
    cout << endl << endl;
    
    // parse first arg to try and locate resources
    resourceRoot = string(argv[0]).substr(0,string(argv[0]).find_last_of("/\\")+1);
    
    
    //--------------------------------------------------------------------------
    // INIT OSC
    //--------------------------------------------------------------------------
    client_osc.open(PORT);
    server_osc.open(AUDIO_OSC_PORT);
    
    std::thread osc_server_thrd(server_recv_thread);
    
    //--------------------------------------------------------------------------
    // OPENGL - WINDOW DISPLAY
    //--------------------------------------------------------------------------

    // initialize GLFW library
    if (!glfwInit())
    {
        cout << "failed initialization" << endl;
        cSleepMs(1000);
        return 1;
    }

    // set error callback
    glfwSetErrorCallback(errorCallback);

    // compute desired size of window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int w = 0.8 * mode->height;
    int h = 0.5 * mode->height;
    int x = 0.5 * (mode->width - w);
    int y = 0.5 * (mode->height - h);

    // set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // set active stereo mode
    if (stereoMode == C_STEREO_ACTIVE)
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_STEREO, GL_FALSE);
    }

    // create display context
    window = glfwCreateWindow(w, h, "CHAI3D", NULL, NULL);
    if (!window)
    {
        cout << "failed to create window" << endl;
        cSleepMs(1000);
        glfwTerminate();
        return 1;
    }

    // get width and height of window
    glfwGetWindowSize(window, &width, &height);

    // set position of window
    glfwSetWindowPos(window, x, y);

    // set key callback
    glfwSetKeyCallback(window, keyCallback);

    // set resize callback
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    // set current display context
    glfwMakeContextCurrent(window);

    // sets the swap interval for the current display context
    glfwSwapInterval(swapInterval);

#ifdef GLEW_VERSION
    // initialize GLEW library
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW library" << endl;
        glfwTerminate();
        return 1;
    }
#endif


    //--------------------------------------------------------------------------
    // WORLD - CAMERA - LIGHTING
    //--------------------------------------------------------------------------

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    world->m_backgroundColor.setBlack();

    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

    // position and orient the camera
    camera->set( cVector3d (0.5, 0.0, 0.0),    // camera position (eye)
                 cVector3d (0.0, 0.0, 0.0),    // look at position (target)
                 cVector3d (0.0, 0.0, 1.0));   // direction of the (up) vector

    // set the near and far clipping planes of the camera
    camera->setClippingPlanes(0.01, 10.0);

    // set stereo mode
    camera->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    camera->setStereoEyeSeparation(0.01);
    camera->setStereoFocalLength(0.5);

    // set vertical mirrored display mode
    camera->setMirrorVertical(mirroredDisplay);

    // create a directional light source
    light = new cDirectionalLight(world);

    // insert light source inside world
    world->addChild(light);

    // enable light source
    light->setEnabled(true);

    // define direction of light beam
    light->setDir(-1.0, 0.0, 0.0);

    // create a sphere (cursor) to represent the haptic device
    cursor = new cShapeSphere(0.01);

    // insert cursor inside world
    world->addChild(cursor);

    // create small line to illustrate the velocity of the haptic device
    velocity = new cShapeLine(cVector3d(0,0,0), 
                              cVector3d(0,0,0));

    // insert line inside world
    world->addChild(velocity);


    //--------------------------------------------------------------------------
    // HAPTIC DEVICE
    //--------------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

    // get a handle to the first haptic device
    handler->getDevice(hapticDevice, 0);

    // open a connection to haptic device
    hapticDevice->open();

    // calibrate device (if necessary)
    hapticDevice->calibrate();

    // retrieve information about the current haptic device
    cHapticDeviceInfo info = hapticDevice->getSpecifications();

    // display a reference frame if haptic device supports orientations
    if (info.m_sensedRotation == true)
    {
        // display reference frame
        cursor->setShowFrame(true);

        // set the size of the reference frame
        cursor->setFrameSize(0.05);
    }

    // if the device has a gripper, enable the gripper to simulate a user switch
    hapticDevice->setEnableGripperUserSwitch(true);
    
    //--------------------------------------------------------------------------
    // SETUP AUDIO
    //--------------------------------------------------------------------------
    
    // create an audio device to play sounds
    audioDevice = new cAudioDevice();
    
    // attach audio device to camera
    camera->attachAudioDevice(audioDevice);
    
    audioBuffer1 = new cAudioBuffer();
    bool fileload1 = audioBuffer1->loadFromFile(RESOURCE_PATH("../resources/sounds/opBass.wav"));
    if (!fileload1)
    {
        #if defined(_MSVC)
        fileload5 = audioBuffer1->loadFromFile("../../../bin/resources/sounds/opBass.wav");
        #endif
    }
    if (!fileload1)
    {
        cout << "Error - Sound file failed to load or initialize correctly." << endl;
        close();
        return (-1);
    }
    
    // create audio source
    audioSource1 = new cAudioSource();
    audioSource1->setAudioBuffer(audioBuffer1);
    
    // loop playing of sound
    audioSource1->setLoop(true);
    
    // turn off sound for now
    audioSource1->setGain(0.0);

    // set pitch
    audioSource1->setPitch(0.2);

    // play sound
    audioSource1->play();
    

    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    // create a font
    font = NEW_CFONTCALIBRI20();

    // create a label to display the haptic device model
    labelHapticDeviceModel = new cLabel(font);
    camera->m_frontLayer->addChild(labelHapticDeviceModel);
    labelHapticDeviceModel->setText(info.m_modelName);

    // create a label to display the position of haptic device
    labelHapticDevicePosition = new cLabel(font);
    camera->m_frontLayer->addChild(labelHapticDevicePosition);
    
    // create a label to display the haptic and graphic rate of the simulation
    labelRates = new cLabel(font);
    camera->m_frontLayer->addChild(labelRates);


    //--------------------------------------------------------------------------
    // START SIMULATION
    //--------------------------------------------------------------------------

    // create a thread which starts the main haptics rendering loop
    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    // setup callback when application exits
    atexit(close);


    //--------------------------------------------------------------------------
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------

    // call window size callback at initialization
    windowSizeCallback(window, width, height);

    // main graphic loop
    while (!glfwWindowShouldClose(window))
    {
        // get width and height of window
        glfwGetWindowSize(window, &width, &height);

        // render graphics
        updateGraphics();

        // swap buffers
        glfwSwapBuffers(window);

        // process events
        glfwPollEvents();

        // signal frequency counter
        freqCounterGraphics.signal(1);
    }

    // close window
    glfwDestroyWindow(window);

    // terminate GLFW library
    glfwTerminate();

    // exit
    return 0;
}

//------------------------------------------------------------------------------

void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update window size
    width  = a_width;
    height = a_height;

    // update position of label
    labelHapticDeviceModel->setLocalPos(20, height - 40, 0);

    // update position of label
    labelHapticDevicePosition->setLocalPos(20, height - 60, 0);
}

//------------------------------------------------------------------------------

void errorCallback(int a_error, const char* a_description)
{
    cout << "Error: " << a_description << endl;
}

//------------------------------------------------------------------------------

void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
    // filter calls that only include a key press
    if ((a_action != GLFW_PRESS) && (a_action != GLFW_REPEAT))
    {
        return;
    }

    // option - exit
    else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
    {
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);
    }

    // option - enable/disable force field
    else if (a_key == GLFW_KEY_1)
    {
        useForceField = !useForceField;
        if (useForceField)
            cout << "> Enable force field     \r";
        else
            cout << "> Disable force field    \r";
    }

    // option - enable/disable damping
    else if (a_key == GLFW_KEY_2)
    {
        useDamping = !useDamping;
        if (useDamping)
            cout << "> Enable damping         \r";
        else
            cout << "> Disable damping        \r";
    }

    // option - toggle fullscreen
    else if (a_key == GLFW_KEY_F)
    {
        // toggle state variable
        fullscreen = !fullscreen;

        // get handle to monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        // get information about monitor
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // set fullscreen or window mode
        if (fullscreen)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
        else
        {
            int w = 0.8 * mode->height;
            int h = 0.5 * mode->height;
            int x = 0.5 * (mode->width - w);
            int y = 0.5 * (mode->height - h);
            glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
    }

    // option - toggle vertical mirroring
    else if (a_key == GLFW_KEY_M)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
    }
}

//------------------------------------------------------------------------------

void close(void)
{
    
    client_osc.close();
    
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close haptic device
    hapticDevice->close();

    // delete resources
    delete hapticsThread;
    delete world;
    delete handler;
    delete audioDevice;
    delete audioBuffer1;
    delete audioSource1;
}

//------------------------------------------------------------------------------

void updateGraphics(void)
{
    /////////////////////////////////////////////////////////////////////
    // UPDATE WIDGETS
    /////////////////////////////////////////////////////////////////////

    // update position data
    labelHapticDevicePosition->setText(hapticDevicePosition.str(3));

    // update haptic and graphic rate data
    labelRates->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
                        cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

    // update position of label
    labelRates->setLocalPos((int)(0.5 * (width - labelRates->getWidth())), 15);


    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////

    // update shadow maps (if any)
    world->updateShadowMaps(false, mirroredDisplay);

    // render world
    camera->renderView(width, height);

    // wait until all OpenGL commands are completed
    glFinish();

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(err);
}

//------------------------------------------------------------------------------

void updateHaptics(void)
{
    // simulation in now running
    simulationRunning  = true;
    simulationFinished = false;

    // main haptic simulation loop
    while(simulationRunning)
    {
        /////////////////////////////////////////////////////////////////////
        // READ HAPTIC DEVICE
        /////////////////////////////////////////////////////////////////////

        // read position 
        cVector3d position;
        hapticDevice->getPosition(position);
        
        // read orientation
        // JCKL: Will never be used with the falcon.
        cMatrix3d rotation;
        hapticDevice->getRotation(rotation);

        // read gripper position
        // JCKL: Will never be used with the falcon.
        double gripperAngle;
        hapticDevice->getGripperAngleRad(gripperAngle);

        // read linear velocity 
        cVector3d linearVelocity;
        hapticDevice->getLinearVelocity(linearVelocity);

        // read angular velocity
        // JCKL: Will never be used with the falcon.
        cVector3d angularVelocity;
        hapticDevice->getAngularVelocity(angularVelocity);

        // read gripper angular velocity
        // JCKL: Will never be used with the falcon.
        double gripperAngularVelocity;
        hapticDevice->getGripperAngularVelocity(gripperAngularVelocity);

        // read user-switch status (button 0)
        /**
         Novint Falcon Button Layout
         button0 := center
         button1 := left
         button2 := top
         button3 := right
        */
        bool button0, button1, button2, button3;
        button0 = false;
        button1 = false;
        button2 = false;
        button3 = false;
        
        hapticDevice->getUserSwitch(0, button0);
        hapticDevice->getUserSwitch(1, button1);
        hapticDevice->getUserSwitch(2, button2);
        hapticDevice->getUserSwitch(3, button3);

        /////////////////////////////////////////////////////////////////////
        // UPDATE 3D CURSOR MODEL
        /////////////////////////////////////////////////////////////////////
       
        // update arrow
        velocity->m_pointA = position;
        velocity->m_pointB = cAdd(position, linearVelocity);

        // update position and orientation of cursor
        cursor->setLocalPos(position);
        cursor->setLocalRot(rotation);

        // adjust the  color of the cursor according to the status of
        // the user-switch (ON = TRUE / OFF = FALSE)
        if (button0)
        {
            cursor->m_material->setGreenMediumAquamarine(); 
        }
        else if (button1)
        {
            cursor->m_material->setYellowGold();
        }
        else if (button2)
        {
            cursor->m_material->setOrangeCoral();
        }
        else if (button3)
        {
            cursor->m_material->setPurpleLavender();
        }
        else
        {
            cursor->m_material->setBlueRoyal();
        }

        // update global variable for graphic display update
        hapticDevicePosition = position;
        
        /////////////////////////////////////////////////////////////////////
        // Apply Audio Modulations
        /////////////////////////////////////////////////////////////////////
        audioSource1->setPitch(1.0 + linearVelocity.x() * 10);


        /////////////////////////////////////////////////////////////////////
        // COMPUTE AND APPLY FORCES
        /////////////////////////////////////////////////////////////////////

        // desired position
        cVector3d desiredPosition;
        desiredPosition.set(0.0, 0.0, 0.0);

        // desired orientation
        cMatrix3d desiredRotation;
        desiredRotation.identity();
        
        // variables for forces
        cVector3d force (0,0,0);
        cVector3d torque (0,0,0);
        double gripperForce = 0.0;
        
        cVector3d norm_pos = normalize_position(position);

        
        /////////////////////////////////////////////////////////////////////
        // COMPUTE AND APPLY BOW SIMULATION
        /////////////////////////////////////////////////////////////////////

//        user_pos = position.y();
//        bow_pressure = 1 - norm_pos.z();
//        anchor_dist = user_pos - anchor_pos;
//
//        float next_anchor_pos;
//        if (abs(anchor_dist) > ANCHOR_DIST_MAX) {
//         next_anchor_pos = user_pos - ANCHOR_DIST_MAX * (anchor_dist / abs(anchor_dist));
//        }
//        else {
//         next_anchor_pos = anchor_pos;
//        }
//
//        anchor_absorb_coeff = get_absorb_coeff(anchor_vel - string_vel, bow_pressure);
//
//        float next_bow_pos;
//        next_bow_pos = anchor_pos - (anchor_pos - bow_pos) * anchor_absorb_coeff;
//
//        friction_force = (user_pos - bow_pos) * bow_pressure * anchor_absorb_coeff;
//
//        anchor_pos = next_anchor_pos;
//        bow_pos = next_bow_pos;
//        std::cout << user_pos << " " << bow_pos << " " << anchor_pos << std::endl;
//        force.y(friction_force * 6000);
        
        
        /////////////////////////////////////////////////////////////////////
        // COMPUTE AND APPLY BOW SIMULATION Simulation
        /////////////////////////////////////////////////////////////////////
        
        float string_rest_pos_z = 0.01;
        float string_force_z = 0;
        float F_zmax = 100.0;
        float string_width_m = 0.02;
        float k = F_zmax / string_width_m;
        
        cVector3d string_force_friction (0,0,0);
        float max_friction_force_y = 30;
        float string_vel = server_osc.get_vec3().y();
        float string_force_y = 0;
        
        // Apply Hooke's law if below position of string
        // Apply Friction
        
        // STICK == v_string = v_bow, then |friction force (N)| < maximum static friction force (N)
        // SLIP == v_string != v_bow, then
        
        // SLip Phase happens if absolute value of the elastic force higher than the static friction force and
        //                    string velocity unequal to the bow velocity and no null string acceleration
        
        if ((string_rest_pos_z > position.z())) {
            string_force_z = (string_rest_pos_z - position.z()) * k;
//            float friction_coeff =
//                    get_absorb_coeff(linearVelocity.y() - string_vel,
//                                    string_force_z/F_zmax > 1.0 ? 1.0: string_force_z/300
//                                     );
            if( abs(linearVelocity.length()) < 0.00001) {
                string_force_friction.set(max_friction_force_y,max_friction_force_y,max_friction_force_y);
            } else {
                // STICK
                // Friction relative to the opposite direction of movement
                string_force_friction = -1 * (linearVelocity / linearVelocity.length()) * string_force_z * 0.4;
//                string_force_y = -1*position.y() * string_force_z * friction_coeff;

            }
        }
        
        // SLIP
        if (abs(string_force_friction.y()) > max_friction_force_y) {
            string_force_friction = -1 * (linearVelocity / linearVelocity.length()) * string_force_z * 0.07;
//            std::cout << "HERE\n";
        }
        
        force.z(string_force_z);
//        force.y(string_force_y);
        force.add(string_force_friction);
        
        
//        std::cout << "POSITION: " << position << std::endl;
//        std::cout << "FORCE: " << force << std::endl;

        // apply force field
        if (useForceField)
        {
            // compute linear force
            double Kp = 1000; // [N/m]
            cVector3d forceField = Kp * (desiredPosition - position);
            force.add(forceField);

            // compute angular torque
            double Kr = 0.05; // [N/m.rad]
            cVector3d axis;
            double angle;
            cMatrix3d deltaRotation = cTranspose(rotation) * desiredRotation;
            deltaRotation.toAxisAngle(axis, angle);
            torque = rotation * ((Kr * angle) * axis);
        }
        
        // apply damping term
        if (useDamping)
        {
            cHapticDeviceInfo info = hapticDevice->getSpecifications();

            // compute linear damping force
            // Map it to a sine wave.
            double damp_amount = norm_pos.x(); //sin(norm_pos.x() * M_PI * 2);
            std::cout << damp_amount << std::endl;
            double Kv = damp_amount * info.m_maxLinearDamping;
            cVector3d forceDamping = -Kv * linearVelocity;
            force.add(forceDamping);

            // compute angular damping force
            double Kvr = 1.0 * info.m_maxAngularDamping;
            cVector3d torqueDamping = -Kvr * angularVelocity;
            torque.add(torqueDamping);

            // compute gripper angular damping force
            double Kvg = 1.0 * info.m_maxGripperAngularDamping;
            gripperForce = gripperForce - Kvg * gripperAngularVelocity;
        }
//        std::cout << force << std::endl;
        // send computed force, torque, and gripper force to haptic device
        hapticDevice->setForceAndTorqueAndGripperForce(force, torque, gripperForce);
        client_osc.send_vec3d("/bow_force", force);
        client_osc.send_vec3d("/bow_vel", linearVelocity);

        // signal frequency counter
        freqCounterHaptics.signal(1);
    }
    
    // exit haptics thread
    simulationFinished = true;
}

//------------------------------------------------------------------------------
