#pragma once
#include "GL/glew.h"
#include "GL/GL.h"
#include "imgui.h"
#include <functional>
#include <common/shader.hpp>

class ImGui3dCanvas
{
public:
    std::string name;
    GLuint FramebufferID;
    GLuint renderedTextureID;
    GLuint depthrenderbufferID;
    int width, height;
    std::function<void(void)> renderCallback;
    ImGui3dCanvas(std::string name, int w,int h);
    ~ImGui3dCanvas();
    void Render();
};

