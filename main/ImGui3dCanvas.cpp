#include "ImGui3dCanvas.h"

ImGui3dCanvas::ImGui3dCanvas(std::string name, int w, int h)
{
    this->name = name;
    width = w;
    height = h;
    // ------------- start rendering to texture init
    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    FramebufferID = 0;
    glGenFramebuffers(1, &FramebufferID);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferID);

    // The texture we're going to render to    
    glGenTextures(1, &renderedTextureID);
    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, renderedTextureID);
    // Give an empty image to OpenGL ( the last "0" means "empty" )
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    // Poor filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // The depth buffer    
    glGenRenderbuffers(1, &depthrenderbufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbufferID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1024, 768);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbufferID);
    // Set "renderedTexture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTextureID, 0);
    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
    // Always check that our framebuffer is ok
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return;
    // ------------- end rendering to texture init
}
ImGui3dCanvas::~ImGui3dCanvas()
{
    glDeleteTextures(1, &renderedTextureID);
    glDeleteFramebuffers(1, &FramebufferID);
}
void ImGui3dCanvas::Render()
{
    ImGui::Begin(name.c_str());
    ImGui::BeginChild((name+"_child").c_str());
    ImVec2 avail_size = ImGui::GetContentRegionAvail();

    if (avail_size.x != width || avail_size.y != height)
    {
        glDeleteRenderbuffers(1, &depthrenderbufferID);
        glDeleteTextures(1, &renderedTextureID);
        glDeleteFramebuffers(1, &FramebufferID);


        glGenFramebuffers(1, &FramebufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, FramebufferID);

        // The texture we're going to render to    
        glGenTextures(1, &renderedTextureID);
        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, renderedTextureID);
        // Give an empty image to OpenGL ( the last "0" means "empty" )
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, avail_size.x, avail_size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        // Poor filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // The depth buffer    
        glGenRenderbuffers(1, &depthrenderbufferID);
        glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbufferID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, avail_size.x, avail_size.y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbufferID);
        // Set "renderedTexture" as our colour attachement #0
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTextureID, 0);
        // Set the list of draw buffers.
        GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
        // Always check that our framebuffer is ok
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            return;

    }
    width = avail_size.x;
    height = avail_size.y;
    // ---------------------------------------------
    // Render to Texture - specific code begins here
    // ---------------------------------------------
    // Render to our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferID);
    glViewport(0, 0, width, height); // Render on the whole framebuffer, complete from the lower left corner to the upper right                
    glClearColor(1, 0, 1, 1.0);
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // RENDER
    if (renderCallback)
    {
        renderCallback(this);
    }
    // Render to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  
    ImGui::Image((void*)(intptr_t)renderedTextureID, ImVec2(avail_size.x, avail_size.y));
    ImGui::EndChild();
    ImGui::End();

}