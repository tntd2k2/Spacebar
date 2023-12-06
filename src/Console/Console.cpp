#include "Console.h"

void Console::Clear() {
    Buffer.clear();
    LineOffsets.clear();
}

void Console::Show()
{
    bIsOpen = true;
}

void Console::Hide() {
    bIsOpen = false;
}

void Console::Render() {
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("LeagueDebugger by dev.tntd2k2", &bIsOpen, window_flags);
    ImGui::Checkbox("Auto-scroll", &AutoScroll);
    ImGui::SameLine();
    if (ImGui::Button("Clear"))
        Clear();
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
    ImGui::SameLine();
    Filter.Draw("Filter", -100.0f);
    ImGui::Separator();
    ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
    if (copy) ImGui::LogToClipboard();

    if (Filter.IsActive()) {
        const char* buf_begin = Buffer.begin();
        const char* line = buf_begin;
        for (int line_no = 0; line != NULL; line_no++) {
            const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
            if (Filter.PassFilter(line, line_end))
                ImGui::TextUnformatted(line, line_end);
            line = line_end && line_end[1] ? line_end : NULL;
        }
    } else {
        const char* buf_begin = Buffer.begin();
        const char* line = buf_begin;
        for (int line_no = 0; line != NULL; line_no++) {
            const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
            ImGui::TextUnformatted(line, line_end);
            line = line_end && line_end[1] ? line_end: NULL;
        }
    }
    ImGui::PopStyleVar();

    if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
}

void Console::Print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Buffer.appendfv(fmt, args);
    va_end(args);
    LineOffsets.push_back(Buffer.size());
}