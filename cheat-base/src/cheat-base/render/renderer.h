#pragma once
#include <Windows.h>
#include <cheat-base/Event.h>
#include <imgui.h>

namespace renderer
{

	void Init(LPBYTE pFontData, DWORD dFontDataSize);

	// Font sizing
	ImFont* GetFontBySize(float fontSize);
	float GetScaleByFontSize(float fontSize);

	void SetGlobalFontSize(float globalFontSize);
	float GetGlobalFontSize();

	// Input lock
	void SetInputLock(void* id, bool value);
	void AddInputLocker(void* id);
	void RemoveInputLocker(void* id);
	bool IsInputLocked();

	class events 
	{
	public:
		inline static TEvent<> RenderEvent{};
	};
}
