#pragma once
#include "BLImageGL.h"
#include "imgui.h"
#include <functional>

class ImGui2dCanvas
{
public:
    std::string name;
    BLImageGL* canvas;
    BLContext ctx;
    BLFontFace face;
    BLFont font;
    BLGradient background_gradient;
    float canvas_scale;

    float offset_x;
    float offset_y;
    float tmp_offset_x;
    float tmp_offset_y;
    float gridStep_x;
    float gridStep_y;
    float minor_gridStep_x;
    float minor_gridStep_y;

    ImGui2dCanvas(std::string name, int w,int h);
    ~ImGui2dCanvas();
    std::function<void(ImGui2dCanvas*)> renderCallback;
    void ImGui2dCanvas::Render();
};

