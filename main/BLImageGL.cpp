#include "BLImageGL.h"
#include "GL/glew.h"
#include "GL/GL.h"
// ----------------------------------------------------
// 
// ----------------------------------------------------
BLImageGL::BLImageGL(int w, int h) :BLImage(w, h, BL_FORMAT_PRGB32)
{
    open = true;
    name = "noname";
    SetMat(*this, true);
}
// ----------------------------------------------------
// 
// ----------------------------------------------------
BLImageGL::BLImageGL(BLImage& frame) : BLImage(frame)
{
    Clear();
    SetMat(frame);
}
// ----------------------------------------------------
// clear texture and realease all memory associated with it
// ----------------------------------------------------
void BLImageGL::Clear(void)
{
    if (texture)
    {
        try
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &texture);
            glDeleteBuffers(1, &frameBuffer_);
        }
        catch (...)
        {

        }
    }
    texture = 0;
}
// ----------------------------------------------------
// 
// ----------------------------------------------------
unsigned int& BLImageGL::getTexture(void)
{
    return texture;
}
// ----------------------------------------------------
//
// ----------------------------------------------------
void BLImageGL::switchOpen(void)
{
    if (open)
    {
        open = false;
    }
    else
    {
        open = true;
    }
}
// ----------------------------------------------------
// 
// ----------------------------------------------------
bool* BLImageGL::getOpen() { return &open; }
// ----------------------------------------------------
// 
// ----------------------------------------------------
void BLImageGL::SetName(const char* nme)
{
    name = std::string(nme);
}
// ----------------------------------------------------
// 
// ----------------------------------------------------
const char* BLImageGL::GetName(void)
{
    return name.c_str();
}
// ----------------------------------------------------
// 
// ----------------------------------------------------
void BLImageGL::update(void)
{
    SetMat(*this);
}


void BLImageGL::SetMat(BLImage& frame, bool forceUpdate)
{
    if (frame.empty())
    {
        return;
    }
    bool needUpdate = !((frame.width() == this->width()) && (frame.height() == this->height()));
    needUpdate |= forceUpdate;

    if (this != &frame)
    {
        memcpy(this->impl->pixelData, frame.impl->pixelData, this->width() * this->height() * 4);
    }

    if (needUpdate)
    {
        Clear();
        glGenFramebuffers(1, &frameBuffer_);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // Set texture clamping method
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width(), height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, this->impl->pixelData);
        // attach it to currently bound framebuffer object
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width(), height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, this->impl->pixelData);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    }
}
