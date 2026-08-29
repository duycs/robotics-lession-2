#include "chai3d.h"
#include <GLFW/glfw3.h>
#include "Game.h"
#include <iostream>

using namespace chai3d;
using namespace std;

// Global settings & objects
cWorld* world = nullptr;
cCamera* camera = nullptr;
cViewport* viewport = nullptr;
cDirectionalLight* light = nullptr;

cHapticDeviceHandler* handler = nullptr;
cGenericHapticDevicePtr hapticDevice;

Game game;

bool simulationRunning = false;
bool simulationFinished = true;
cThread* hapticsThread = nullptr;

int windowW = 1280;
int windowH = 720;

// Haptics simulation thread loop (~1000 Hz)
void updateHaptics() {
    simulationRunning = true;
    simulationFinished = false;

    while (simulationRunning) {
        if (hapticDevice) {
            game.updateHaptics(hapticDevice);
        }
        cSleepMs(1);
    }

    simulationFinished = true;
}

// Window resize callback
void onWindowSizeCallback(GLFWwindow* window, int width, int height) {
    windowW = width;
    windowH = height;
}

// Error callback
void onErrorCallback(int error, const char* description) {
    cout << "GLFW Error [" << error << "]: " << description << endl;
}

int main(int argc, char* argv[]) {
    cout << "========================================================" << endl;
    cout << "   3D TOWER DEFENSE GAME (CHAI3D)                      " << endl;
    cout << "========================================================" << endl;
    cout << "   Phim dieu khien:                                    " << endl;
    cout << "   - [A] / [D]: Xoay Base Yaw (Ngang 120 deg)          " << endl;
    cout << "   - [W] / [S]: Xoay Gun Pitch (Doc 60 deg)            " << endl;
    cout << "   - [SPACE]: Ban dan (Linear bullet)                  " << endl;
    cout << "   - [N]: Luot choi moi                                " << endl;
    cout << "   - [Q] / [ESC]: Thoat game                           " << endl;
    cout << "========================================================" << endl;

    if (!glfwInit()) {
        cerr << "Loi khoi tao GLFW!" << endl;
        return -1;
    }

    glfwSetErrorCallback(onErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(windowW, windowH, "3D Tower Defense - CHAI3D", NULL, NULL);
    if (!window) {
        cerr << "Loi tao cua so GLFW!" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowSizeCallback(window, onWindowSizeCallback);

#ifdef GLEW_VERSION
    if (glewInit() != GLEW_OK) {
        cerr << "Loi khoi tao GLEW!" << endl;
        glfwTerminate();
        return -1;
    }
#endif

    // 1. CHAI3D World initialization
    world = new cWorld();
    world->m_backgroundColor.set(0.08f, 0.12f, 0.18f);

    // 2. Camera Setup (Shoulder view showing full turret structure & barrel)
    camera = new cCamera(world);
    world->addChild(camera);

    camera->set(cVector3d(0.0, -11.0, 4.5),   // Eye position elevated to reveal turret body
                cVector3d(0.0, 2.5, 0.4),     // LookAt target down the battlefield
                cVector3d(0.0, 0.0, 1.0));    // Up vector
    camera->setClippingPlanes(0.1, 100.0);

    // 3. Directional Light
    light = new cDirectionalLight(world);
    world->addChild(light);
    light->setEnabled(true);
    light->setDir(-0.4, 0.6, -1.0);

    // 4. Haptic Device setup
    handler = new cHapticDeviceHandler();
    handler->getDevice(hapticDevice, 0);

    // Get asset directory path
    string execPath = cGetCurrentPath();
    if (!execPath.empty() && (execPath.back() == '/' || execPath.back() == '\\')) {
        execPath.pop_back();
    }
    string assetsDir = execPath + "/assets";

    // 5. Initialize Game Engine
    game.init(world, camera, assetsDir);

    // Start Haptic Thread
    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    cPrecisionClock clock;
    clock.start();

    // Main Graphics & Logic Loop
    while (!glfwWindowShouldClose(window)) {
        double dt = clock.stop();
        clock.start();
        if (dt > 0.1) dt = 0.1; // clamp delta time for stability

        glfwGetFramebufferSize(window, &windowW, &windowH);

        // Read Keyboard state
        bool keyA = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS);
        bool keyD = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS);
        bool keyW = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
        bool keyS = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
        bool keySpace = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
        bool keyR = (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS);
        bool keyP = (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS);
        bool keyN = (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        // Process game input and logic
        game.processInput(keyA, keyD, keyW, keyS, keySpace, keyR, keyP, keyN);
        game.update(dt);

        // Render CHAI3D Viewport
        camera->renderView(windowW, windowH);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Stop haptics thread
    simulationRunning = false;
    while (!simulationFinished) {
        cSleepMs(10);
    }
    delete hapticsThread;

    glfwDestroyWindow(window);
    glfwTerminate();

    game.cleanup();
    delete world;
    delete handler;

    return 0;
}
