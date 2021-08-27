
#include "LogWindow.h"

    LogWindow::LogWindow()
    {
        AutoScroll = true;
        ScrollToBottom = false;
        Clear();
    }

    void    LogWindow::Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void    LogWindow::AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
        if (AutoScroll)
            ScrollToBottom = true;
    }

    void  LogWindow::Draw(const char* title, bool* p_open)
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup(u8"Options menu"))
        {
            if (ImGui::Checkbox(u8"Autoscsroll", &AutoScroll))
                if (AutoScroll)
                    ScrollToBottom = true;
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button(u8"Options"))
            ImGui::OpenPopup(u8"Options pop-up");
        ImGui::SameLine();
        bool clear = ImGui::Button(u8"Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button(u8"Copy");
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::BeginChild(u8"scroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
            Clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = Buf.begin();
        const char* buf_end = Buf.end();

            ImGuiListClipper clipper;
            clipper.Begin(LineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        ImGui::PopStyleVar();

        if (ScrollToBottom)
            ImGui::SetScrollHereY(1.0f);
        ScrollToBottom = false;
        ImGui::EndChild();
        ImGui::End();
    }

