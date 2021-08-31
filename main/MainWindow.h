#pragma once
#include <iostream>
#include <mutex>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "boost/thread.hpp"
#include "boost/signals2.hpp"
#include "Observer.h"
#include "Observable.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLEW 1
#include "LogWindow.h"
#include <string>
#include <fstream>
#include "boost/assign/list_of.hpp"
// About OpenGL function loaders: modern OpenGL doesn't have a standard header file and requires individual function pointers to be loaded manually.
// Helper libraries are often used for this purpose! Here we are supporting a few common ones: gl3w, glew, glad.
// You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>
// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
//#include <common/vboindexer.hpp>

#include "ImGui2dCanvas.h"
#include "ImGui3dCanvas.h"
#include "icons_font_awesome.h"


void ShowAppDockSpace(bool* p_open);

void renderCallback(ImGui2dCanvas* thisCanvas);
void render3dCallback(ImGui3dCanvas* thisCanvas);
void buttonCallback(std::string buttonName);
void keyCallback(int key, int scancode, int action, int mods);
void mouseCallback(int x, int y, bool button[5]);
// -----------------------------
//
// -----------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
// -----------------------------
//
// -----------------------------
void glfw_error_callback(int error, const char* description);
// -----------------------------
//
// -----------------------------
struct MainWindowObservers
{
    enum {
        OnButtonPressEvent,
        OnRunEvent,
        OnWorkerEvent,
        OnCloseEvent
    };
    using ObserverTable = std::tuple<
        Observer<void(std::string buttonName)>,
        Observer<void(void)>,
        Observer<void(void)>,
        Observer<void(void)>
    >;
};


// -----------------------------
//
// -----------------------------
class MainWindow : public Observable< MainWindowObservers>
{
public:
    std::string LocaleName;
    int width;
    int height;
    int sidePanelWidth;
    // Main window
    GLFWwindow* window;
    ImGui2dCanvas* canvas2D;
    ImGui3dCanvas* canvas3D;
    std::function<void(MainWindow*)> afterInitGraphicsCallback;
    std::function<void(MainWindow*)> GUICallback;
    std::function<void(MainWindow*)> beforeTerminateGraphicsCallback;
    std::chrono::steady_clock::time_point prev;

    bool isRunning;
    std::mutex m;
    boost::thread* thr;
    // -----------------------------
    //
    // -----------------------------
    MainWindow();
    // -----------------------------
    //
    // -----------------------------
    ~MainWindow();   
    // -----------------------------
    //
    // -----------------------------
    void InitGraphics();

    // -----------------------------
    // Free graphic resources
    // -----------------------------
    void TerminateGraphics(void);

    // -----------------------------
    // Main loop
    // -----------------------------
    void worker(void);
    // -----------------------------
    //
    // -----------------------------
    void Run(void);
    // -----------------------------
    //
    // -----------------------------
    void Stop(void);

};

// -----------------------------
//
// -----------------------------
// Application: our Observer.
class Application
{
public:
    // -----------------------------
    //
    // -----------------------------
    explicit Application(MainWindow& worker);
    // -----------------------------
    //
    // -----------------------------
    ~Application();

private:
    bool finished;
    boost::thread* thrm;
    // -----------------------------
    //
    // -----------------------------
    void OnRun();
    // -----------------------------
    //
    // -----------------------------
    void OnClose();
    // -----------------------------
    //
    // -----------------------------
    void OnWorker();
    // -----------------------------
    //
    // -----------------------------
    void OnButton(std::string button_name);
    MainWindow& worker_;
};