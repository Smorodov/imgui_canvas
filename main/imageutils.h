
#include "opencv2/opencv.hpp"
#include <GLFW/glfw3.h>
#include <string>
#include <sstream>
#include <iostream>

class GLMat : public cv::Mat
{
public:
    GLMat() : Mat()
    {
        texture = 0;
        open = true;
        name = "noname";
    }
   
    GLMat(cv::Mat& frame, bool swap_RB = true) : Mat()
    {
        Clear();
        SetMat(frame, swap_RB);
    }
 /*
    ~OpenCVImage()
    {
        Clear();
    }
   */   
    // clear texture and realease all memory associated with it
    void Clear(void) 
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
            catch(...)
            {

            }
        }
        texture = 0; 
    }

    GLuint& getTexture(void) 
    {
        return texture; 
    }

    void switchOpen(void) 
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

    bool* getOpen() { return &open; } 
    
    void SetName(const char* nme) 
    {
        name = std::string(nme);
    }

    const char* GetName(void)
    {
        return name.c_str();
    }
    GLMat& GLMat::operator=(cv::Mat& frame) // copy/move constructor is called to construct arg
    {
        SetMat(frame, true);
        return *this;
    }
    GLMat& GLMat::operator=(GLMat& frame) // copy/move constructor is called to construct arg
    {
        SetMat(frame, true);
        return *this;
    }
       
private:
    void SetMat(cv::Mat& frame, bool swap_RB = false)
    {
        if (frame.empty())
        {
            return;
        }
        bool needUpdate = !(frame.size() == this->size());
        if (frame.channels() == 3)
        {
            if (swap_RB)
            {
                cv::cvtColor(frame, *this, cv::COLOR_BGR2RGBA);
            }
            else
            {
                cv::cvtColor(frame, *this, cv::COLOR_RGB2RGBA);
            }
        }

        if (frame.channels() == 4)
        {
            if (swap_RB)
            {
                cv::cvtColor(frame, *this, cv::COLOR_BGRA2RGBA);
            }
            else
            {
                *this = frame.clone();
            }
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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr());
            // attach it to currently bound framebuffer object
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,texture, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr());
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                texture, 0);
            
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

        }
    }

    GLuint texture;
    GLuint frameBuffer_;
    bool open;
    std::string name;

};


