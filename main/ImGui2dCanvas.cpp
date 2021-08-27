#include "ImGui2dCanvas.h"

ImGui2dCanvas::ImGui2dCanvas(std::string name,int w,int h)
{
    this->name = name;
    canvas_scale = 0.5;
    tmp_offset_x = 0 * canvas_scale;
    tmp_offset_y = 0 * canvas_scale;
    offset_x = 0 * canvas_scale;
    offset_y = 0 * canvas_scale;
    gridStep_x = 100 * canvas_scale;
    gridStep_y = 100 * canvas_scale;
    minor_gridStep_x = gridStep_x / 10.0;
    minor_gridStep_y = gridStep_y / 10.0;

    canvas = new BLImageGL(1024, 1024);
    ctx = BLContext(*canvas);
    // There are many overloads available in C++ API.
    background_gradient.create(
        BLLinearGradientValues(0, 0, canvas->width(), canvas->height()),
        BL_EXTEND_MODE_PAD);
    background_gradient.addStop(0.0, BLRgba32(0xff550000));// abgr
    background_gradient.addStop(1.0, BLRgba32(0xffaa0000));// abgr

    BLResult err = face.createFromFile("fonts/FiraCode/ttf/FiraCode-Regular.ttf");
    // We must handle a possible error returned by the loader.
    if (err)
    {
        printf("Failed to load a font-face (err=%u)\n", err);
    }
    font.createFromFace(face, 24.0f);
}

ImGui2dCanvas::~ImGui2dCanvas()
{
    delete canvas;
}

// -----------------------------
//
// -----------------------------
void ImGui2dCanvas::Render()
{
    ImGui::Begin(name.c_str());
    ImGui::BeginChild( (name+"_child").c_str());    
    float x = ImGui::GetIO().MousePos.x - ImGui::GetCursorScreenPos().x - ImGui::GetScrollX();
    float y = ImGui::GetIO().MousePos.y - ImGui::GetCursorScreenPos().y - ImGui::GetScrollY();
    //mouseCallback(x, y, ImGui::GetIO().MouseDown);
    ImVec2 avail_size = ImGui::GetContentRegionAvail();

    ImVec2 clickedPos = ImVec2(x, y);
    if (ImGui::GetIO().MouseDown[2])
    {
        clickedPos = ImGui::GetIO().MouseClickedPos[2];
        clickedPos.x = clickedPos.x - ImGui::GetCursorScreenPos().x - ImGui::GetScrollX();
        clickedPos.y = clickedPos.y - ImGui::GetCursorScreenPos().y - ImGui::GetScrollY();
        tmp_offset_x = x - clickedPos.x;
        tmp_offset_y = y - clickedPos.y;
    }
    else
    {
        offset_x += tmp_offset_x;
        offset_y += tmp_offset_y;
        tmp_offset_x = 0;
        tmp_offset_y = 0;

        clickedPos = ImVec2(x, y);
    }

    if (avail_size.x != canvas->width() || avail_size.y != canvas->height())
    {
        delete canvas;
        canvas = new BLImageGL(avail_size.x, avail_size.y);
    }

    ctx.begin(*canvas);
    ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.setFillStyle(background_gradient);
    ctx.fillAll();

    ctx.setStrokeStyle(BLRgba32(0xFF446666));
    ctx.setStrokeWidth(1);
    for (float x = 0 - gridStep_x; x < canvas->width()+ gridStep_x; x += minor_gridStep_x)
    {
        ctx.strokeLine(x-gridStep_x+((int)(offset_x+tmp_offset_x) % (int)gridStep_x), 0, x - gridStep_x + ((int)(offset_x + tmp_offset_x) % (int)gridStep_x), canvas->height());
    }
    for (float y = 0 - gridStep_y; y < canvas->height() + gridStep_y; y += minor_gridStep_y)
    {
        ctx.strokeLine(0, y + ((int)(offset_y+tmp_offset_y) % (int)gridStep_y), canvas->width(), y + ((int)(offset_y + tmp_offset_y) % (int)gridStep_y));
    }
    ctx.setStrokeStyle(BLRgba32(0xFF666666));
    ctx.setStrokeWidth(2);
    for (float x = 0 - gridStep_x; x < canvas->width() + gridStep_x; x += gridStep_x)
    {
        ctx.strokeLine(x+ ((int)(offset_x+tmp_offset_x) % (int)gridStep_x), 0, x + ((int)(offset_x + tmp_offset_x) % (int)gridStep_x), canvas->height());
    }
    for (float y = 0 - gridStep_y; y < canvas->height() + gridStep_y; y += gridStep_y)
    {
        ctx.strokeLine(0, y + ((int)(offset_y + tmp_offset_y) % (int)gridStep_y), canvas->width(), y + ((int)(offset_y + tmp_offset_y) % (int)gridStep_y));
    }

    if (renderCallback)
    {
        BLMatrix2D m = BLMatrix2D::makeTranslation(offset_x+tmp_offset_x, offset_y + tmp_offset_y);
        ctx.setMatrix(m);
        renderCallback(ctx);
        
        ctx.setFillStyle(BLRgba32(0xFFFFFFFF));
        ctx.fillUtf8Text(BLPoint(60, 80), font, "Hello Blend2D!");
        ctx.rotate(0.785398);
        ctx.fillUtf8Text(BLPoint(250, 80), font, "Rotated Text");

        ctx.resetMatrix();
    }    
    ctx.end();  
    canvas->update();

    ImGui::Image((void*)(intptr_t)canvas->getTexture(), ImVec2(canvas->width(), canvas->height()));
    ImGui::EndChild();
    ImGui::End();

}