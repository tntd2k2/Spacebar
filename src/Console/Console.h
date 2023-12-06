#pragma once
#include <imgui.h>
#include <deque>
#include <string>
#include <Windows.h>

class Console {
public:
	void Render();
	void Show();
	void Hide();
	void Clear();
	void Print(const char* fmt, ...);
private:
	ImGuiTextBuffer Buffer;
	ImGuiTextFilter Filter;
	ImVector<int> LineOffsets;
	bool bIsOpen = false;
	bool AutoScroll = true;
};
extern Console console;