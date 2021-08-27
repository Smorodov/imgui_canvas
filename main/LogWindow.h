#include <vector>
#include <algorithm>
#include <iostream>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
class LogWindow
{
public:
    ImGuiTextBuffer     Buf;
    ImVector<int>       LineOffsets;        // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
    bool                AutoScroll;
    bool                ScrollToBottom;

    LogWindow();
    void Clear();
    void AddLog(const char* fmt, ...) IM_FMTARGS(2);
    void Draw(const char* title, bool* p_open = NULL);
};


