#define  IMGUI_DEFINE_MATH_OPERATORS 1
#include "MainWindow.h"
#include "ImGui3dCanvas.h"

static LogWindow log_win;

// How to do a toolbar in Dear ImGui.

const float toolbarSize = 50;
const float statusbarSize = 50;
float menuBarHeight=0;

void DockSpaceUI()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    // Save off menu bar height for later.
    //menuBarHeight = ImGui::GetCurrentWindow()->MenuBarHeight();

    ImGui::SetNextWindowPos(viewport->Pos + ImVec2(0, toolbarSize+menuBarHeight));
    ImGui::SetNextWindowSize(viewport->Size - ImVec2(0, toolbarSize + menuBarHeight + statusbarSize));
    
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags window_flags = 0
        | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
    ImGui::Begin("Master DockSpace", NULL, window_flags);       
    ImGuiID dockMain = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockMain, ImVec2(0.0f, 0.0f));         
    ImGui::End();
    ImGui::PopStyleVar(3);
}

void ToolbarUI()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarSize+menuBarHeight));
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags window_flags = 0
        | ImGuiWindowFlags_MenuBar
        | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings
        ;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.5f, 1.0f));
    ImGui::Begin("TOOLBAR", NULL, window_flags);
    ImGui::PopStyleVar();
    menuBarHeight = ImGui::GetCurrentWindow()->MenuBarHeight();
    ImGui::Button(ICON_FA_FILE, ImVec2(0, 37));
    ImGui::SameLine();
    ImGui::Button(ICON_FA_FOLDER_OPEN_O, ImVec2(0, 37));
    ImGui::SameLine();
    ImGui::Button(ICON_FA_FLOPPY_O, ImVec2(0, 37));
    
    
    // ----------------------------------------------------
    // Отрисовка меню
    // ----------------------------------------------------
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load", "", false)) { ; }
            if (ImGui::MenuItem("Save", "", false)) { ; }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "", false)) { ; }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }


    ImGui::End();
    ImGui::PopStyleColor();
}

void StatusbarUI()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y+viewport->Size.y-statusbarSize));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, statusbarSize));
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags window_flags = 0
        | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings
        ;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.5f, 1.0f));
    ImGui::Begin("STATUSBAR", NULL, window_flags);
    ImGui::PopStyleVar();
    ImGui::Text("Status bar message.");
    ImGui::End();
    ImGui::PopStyleColor();
}
// -----------------------------
// Organize our dockspace
// -----------------------------
void ProgramUI()
{
    DockSpaceUI();
    ToolbarUI();
    StatusbarUI();
}
// -----------------------------
// Keybord callback
// -----------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    keyCallback(key, scancode, action, mods);
}
// -----------------------------
// Peocess errors
// -----------------------------
void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}
// -----------------------------
// Main UI class
// -----------------------------
MainWindow::MainWindow()
{
    LocaleName = "ru_RU.utf8";
    setlocale(LC_ALL, LocaleName.c_str());
    // Size of window
    width = 1024;
    height = 768;
    // Width of side panel
    sidePanelWidth = 300;
    // Main window
    window = nullptr;
    // peocess runnin flag indicates when main thread (worker) works.
    isRunning = false;
    // the main thread handle
    thr = nullptr;
}
// -----------------------------
// destructor
// -----------------------------
MainWindow::~MainWindow()
{
    // terminating main thread
    if (thr != nullptr)
    {
        thr->interrupt();
        thr->join();
        delete thr;
        thr = nullptr;
    }
}
// -----------------------------
// Initializing openGL things
// -----------------------------
void MainWindow::InitGraphics()
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        return;
    }

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    window = glfwCreateWindow(width, height, "CGAL Framework", NULL, NULL);
    if (window == NULL)
        return;
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return;
    }
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    
    // Load Fonts        
    io.Fonts->AddFontFromFileTTF("fonts/FiraCode/ttf/FiraCode-Regular.ttf", 30, NULL, io.Fonts->GetGlyphRangesCyrillic());
    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF("fonts/fontawesome-webfont.ttf", 30.0f, &icons_config, icons_ranges);

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) 
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return;
    }

    // Create 2d canvas
    // Destructor is in TerminateGraphics method
    // size paeameters are temporal? they will
    // be recalculated and adjusted according window size in runtime.
    canvas2D = new ImGui2dCanvas("canves 2d",1024, 1024);
    prev = std::chrono::steady_clock::now();
    // Create 3d canvas
    // Destructor is in TerminateGraphics method
    // size paeameters are temporal? they will
    // be recalculated and adjusted according window size in runtime.
    canvas3D = new ImGui3dCanvas("canves 3d",1024, 1024);


    // Do things (once) after graphics is initialized
    if (afterInitGraphicsCallback)
    {
        afterInitGraphicsCallback(this);
    }
}

// -----------------------------
// Free graphic resources
// -----------------------------
void MainWindow::TerminateGraphics(void)
{
    // Do things (once) before graphics is terminated
    if (beforeTerminateGraphicsCallback)
    {
        beforeTerminateGraphicsCallback(this);
    }
    // free our canvases
    delete canvas2D;
    delete canvas3D;
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

// -----------------------------
// Main loop
// -----------------------------
void MainWindow::worker(void)
{
    // inform all we are running
    isRunning = true;
    static bool open = true;
    // initialize openGL stuff.
    InitGraphics();
    // do user commands before worker runs.
    Notify<MainWindowObservers::OnRunEvent>();
    // -------------
    // Main loop
    // -------------
    while (!boost::this_thread::interruption_requested() && !glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        ImGui::GetIO().WantCaptureMouse = true;
        glfwPollEvents();
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // Render our dock (menu, toolbar, status bar).
        ProgramUI();
        // Rendering user content
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        canvas2D->Render();

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Elapseed = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
        std::cout << "Elapseed (frame)= " << std::chrono::duration_cast<std::chrono::microseconds>(end - prev).count() << "[µs]" << std::endl;
        prev = end;

        log_win.Draw(u8"Logs", &open);

        ImGui::Begin(u8"Control", nullptr);
        if (ImGui::Button(u8"Process", ImVec2(-1, 0)))
        {
            Notify<MainWindowObservers::OnButtonPressEvent>(u8"Обработать");
        }
        ImGui::End();


        Notify<MainWindowObservers::OnWorkerEvent>();


        canvas3D->Render();

        // ImGui rendering
        glClearColor(0, 0, 0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        //std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    

    TerminateGraphics();
    isRunning = false;
    Notify<MainWindowObservers::OnCloseEvent>();
}
// -----------------------------
//
// -----------------------------
void MainWindow::Run(void)
{
    if (!isRunning)
    {
        thr = new boost::thread(&MainWindow::worker, this);
        isRunning = true;
    }
}
// -----------------------------
//
// -----------------------------
void MainWindow::Stop(void)
{
    if (isRunning)
    {
        thr->interrupt();
        thr->join();
        delete thr;
        thr = nullptr;
        isRunning = false;
    }
}

// -----------------------------
//
// -----------------------------
// Application: our Observer.
    // -----------------------------
    //
    // -----------------------------
Application::Application(MainWindow& worker) :
    worker_(worker)
{
    finished = false;

    worker_.Register < MainWindowObservers::OnCloseEvent >([this](void)
        {
            OnClose();
        });

    worker_.Register < MainWindowObservers::OnRunEvent >([this](void)
        {
            OnRun();
        });
    worker_.Register < MainWindowObservers::OnWorkerEvent >([this](void)
        {
            OnWorker();
        });
    worker_.Register < MainWindowObservers::OnButtonPressEvent >([this](std::string button_name)
        {
            OnButton(button_name);
        });
    std::cout << "Events - registered" << std::endl;

    worker_.Run();

    while (worker_.isRunning)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        finished = true;
    }
    worker_.Stop();
}
// -----------------------------
//
// -----------------------------
Application::~Application()
{

}
// -----------------------------
//
// -----------------------------
void Application::OnRun()
{
    std::cout << "On run event." << std::endl;
    log_win.AddLog(u8"On run event.\n");
}
// -----------------------------
//
// -----------------------------
void Application::OnClose()
{
    std::cout << "On close event." << std::endl;
}
// -----------------------------
//
// -----------------------------
void Application::OnWorker()
{

}
// -----------------------------
//
// -----------------------------
void Application::OnButton(std::string button_name)
{
    buttonCallback(button_name);
}

