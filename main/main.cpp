#include "main.h"
#include "imfilebrowser.h"
// -----------------------------
// https://github.com/AirGuanZ/imgui-filebrowser
// -----------------------------
// create a file browser instance
// -----------------------------
ImGui::FileBrowser fileDialog;
// -----------------------------
//
// -----------------------------
void keyCallback(int key, int scancode, int action, int mods)
{

}
// -----------------------------
//
// -----------------------------
void mouseCallback(int x, int y, bool button[5])
{
    // Всю остальную инфу о мышке брать здесь.
    ImGuiIO io = ImGui::GetIO();
    
}
// -----------------------------
// Render callback for 2D canvas
// Must be assigned after graphisc initialized
// -----------------------------
static void renderCallback(ImGui2dCanvas* thisCanvas)
{
    BLContext& ctx = thisCanvas->ctx;
    BLFont& font = thisCanvas->font;
    if (ctx)
    {
        ctx.setFillStyle(BLRgba32(0xFFFFFFFF));
        ctx.fillUtf8Text(BLPoint(60, 80), font, "Hello Blend2D!");
        
        ctx.rotate(0.785398);
        ctx.fillUtf8Text(BLPoint(250, 80), font, "Rotated Text");
        ctx.rotate(-0.785398);

        ctx.setStrokeStyle(BLRgba32(0xFF00ffff));
        ctx.setStrokeWidth(2);
        ctx.strokeLine(0, 0, 100, 199);
    }
}

static GLuint vertexBufferID;
static GLuint indexBufferID;
static GLuint programID;
static void render3dCallback(ImGui3dCanvas* thisCanvas)
{
    // Use our shader
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glUseProgram(programID);    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
// -----------------------------
//
// -----------------------------
void buttonCallback(std::string buttonName)
{
    if (buttonName == u8"Обработать")
    {
        fileDialog.Open();
    }
}
// -----------------------------
//
// -----------------------------
void GUICallback(MainWindow* sender)
{
    fileDialog.Display();
    if (fileDialog.HasSelected())
    {
        std::cout << "Selected filename" << fileDialog.GetSelected().string() << std::endl;
        fileDialog.ClearSelected();
    }
}
// -----------------------------
//
// -----------------------------
void afterInitGraphicsCallback(MainWindow* sender)
{
    // (optional) set browser properties
    fileDialog.SetTitle("title");
    fileDialog.SetTypeFilters({ ".h", ".cpp" });

    // begin rendered content init
    GLfloat vertices[] =
    {
        +0.0f, +0.0f, //0
        +1.0f, +1.0f, //1
        -1.0f, +1.0f, //2
        -1.0f, -1.0f, //3
        +1.0f, -1.0f, //4
    };


    glGenBuffers(1, &vertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, 0);

    GLushort indices[] = { 0,1,2, 0,3,4 };
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // Create and compile our GLSL program from the shaders
    programID = LoadShaders("StandardShadingRTT.vertexshader", "StandardShadingRTT.fragmentshader");
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    sender->canvas3D->renderCallback = &render3dCallback;
    sender->canvas2D->renderCallback = &renderCallback;
}
// -----------------------------
//
// -----------------------------
void beforeTerminateGraphicsCallback(MainWindow* sender)
{
    glDeleteProgram(programID);
    glDeleteBuffers(1, &indexBufferID);
    glDeleteBuffers(1, &vertexBufferID);
}
// -----------------------------
//
// -----------------------------
int main()
{
    MainWindow worker;
    worker.afterInitGraphicsCallback = &afterInitGraphicsCallback;
    worker.beforeTerminateGraphicsCallback = beforeTerminateGraphicsCallback;
    worker.GUICallback = GUICallback;
    Application application{ worker };
}
