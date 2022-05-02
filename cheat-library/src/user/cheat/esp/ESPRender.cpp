#include <pch-il2cpp.h>
#include "ESPRender.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include <helpers.h>
#include <cheat/game/EntityManager.h>
#include <sys/timeb.h>

#include "ESP.h"

namespace cheat::feature::esp::render
{
	static app::Camera* s_Camera = nullptr;
	static ImVec2 s_ResolutionScale = ImVec2(0, 0);
	static ImVec2 s_ScreenResolution = ImVec2(0, 0);
	static ImVec2 s_AvatarPosition = ImVec2(0, 0);

	static void UpdateMainCamera()
	{
		UPDATE_DELAY(1000);

		s_Camera = nullptr;

		auto camera = app::Camera_get_main(nullptr, nullptr);
		if (camera == nullptr)
			return;

		if (!app::Behaviour_get_isActiveAndEnabled(reinterpret_cast<app::Behaviour*>(camera), nullptr))
			return;

		auto loadingManager = GET_SINGLETON(LoadingManager);
		if (loadingManager == nullptr || !app::LoadingManager_IsLoaded(loadingManager, nullptr))
			return;

		s_Camera = camera;
	}

	static void UpdateResolutionScale()
	{
		UPDATE_DELAY(1000);

		SAFE_BEGIN();
		s_ResolutionScale = { 0, 0 };

		if (s_Camera == nullptr)
			return;

		auto pixelWidth = app::Camera_get_pixelWidth(s_Camera, nullptr);
		auto pixelHeight = app::Camera_get_pixelHeight(s_Camera, nullptr);

		if (pixelWidth == 0 || pixelHeight == 0)
			return;

		auto screenWidth = app::Screen_get_width(nullptr, nullptr);
		auto screenHeight = app::Screen_get_height(nullptr, nullptr);

		if (screenWidth == 0 || screenHeight == 0)
			return;

		if (screenHeight == pixelHeight && screenWidth == pixelWidth)
			return;

		s_ScreenResolution.x = screenWidth;
		s_ScreenResolution.y = screenHeight;

		s_ResolutionScale.x = static_cast<float>(screenWidth) / static_cast<float>(pixelWidth);
		s_ResolutionScale.y = static_cast<float>(screenHeight) / static_cast<float>(pixelHeight);
		SAFE_EEND();
	}

	static app::Vector3 WorldToScreenPosScalled(const app::Vector3& relPosition)
	{
		if (s_Camera == nullptr)
			return {};

		auto screenPos = app::Camera_WorldToScreenPoint(s_Camera, relPosition, nullptr);

		if (s_ResolutionScale.x != 0)
		{
			screenPos.x *= s_ResolutionScale.x;
			screenPos.y *= s_ResolutionScale.y;
		}

		screenPos.y = app::Screen_get_height(nullptr, nullptr) - screenPos.y;
		return screenPos;
	}

	struct Rect
	{
		float xMin, xMax;
		float yMin, yMax;

		bool empty() const
		{
			return xMin == 0 && xMax == 0 && yMin == 0 && yMax == 0;
		}
	};

	struct BoxScreen
	{
		ImVec2 lowerTopLeft;
		ImVec2 lowerTopRight;
		ImVec2 lowerBottomLeft;
		ImVec2 lowerBottomRight;

		ImVec2 upperTopLeft;
		ImVec2 upperTopRight;
		ImVec2 upperBottomLeft;
		ImVec2 upperBottomRight;
	};

#undef min
#undef max

	static ImVec2 FromVec3(const app::Vector3& vec3)
	{
		return { vec3.x, vec3.y };
	}

	static app::Bounds GetEntityMinBounds(game::Entity* entity, float minSize)
	{
		auto entityPosition = entity->relativePosition();
		return { entityPosition, { minSize, minSize, minSize } };
	}

	static app::Bounds GetObjectBounds(game::Entity* entity)
	{
		auto& esp = ESP::GetInstance();
		auto gameObject = entity->gameObject();
		if (gameObject == nullptr)
			return GetEntityMinBounds(entity, esp.f_MinSize);

		SAFE_BEGIN();

		// Sometimes occurs access violation in UnityPlayer.dll
		// Callow: Have no idea what to do with it unless just catch exception
		auto bounds = app::Utils_1_GetBounds(nullptr, gameObject, nullptr);
		if (bounds.m_Extents.x < esp.f_MinSize &&
			bounds.m_Extents.y < esp.f_MinSize &&
			bounds.m_Extents.z < esp.f_MinSize)
			bounds.m_Extents = { esp.f_MinSize, esp.f_MinSize, esp.f_MinSize };

		auto min = bounds.m_Center - bounds.m_Extents;
		auto max = bounds.m_Center + bounds.m_Extents;

		// When monster or some another object in a far from player - they disappear
		// And for some reason game object extends
		if ((min.x == 0 || min.y == 0 || min.z == 0))
			return GetEntityMinBounds(entity, 1);

		return bounds;
		
		SAFE_ERROR();
		
		return GetEntityMinBounds(entity, esp.f_MinSize);
		
		SAFE_END();
	}

	static std::optional<BoxScreen> GetEntityScreenBox(game::Entity* entity)
	{
		if (s_Camera == nullptr)
			return {};

		app::Bounds bounds = GetObjectBounds(entity);

		auto min = bounds.m_Center - bounds.m_Extents;
		auto max = bounds.m_Center + bounds.m_Extents;

		BoxScreen box;
		app::Vector3 temp;
#define BOX_FIELD_SET(field, px, py, pz) temp = app::Camera_WorldToScreenPoint(s_Camera, { px, py, pz }, nullptr);\
			if (temp.z < 1) return {};\
			box.##field = FromVec3(temp);

		BOX_FIELD_SET(lowerTopLeft, min.x, min.y, max.z);
		BOX_FIELD_SET(lowerTopRight, max.x, min.y, max.z);
		BOX_FIELD_SET(lowerBottomLeft, min.x, min.y, min.z);
		BOX_FIELD_SET(lowerBottomRight, max.x, min.y, min.z);

		BOX_FIELD_SET(upperTopLeft, min.x, max.y, max.z);
		BOX_FIELD_SET(upperTopRight, max.x, max.y, max.z);
		BOX_FIELD_SET(upperBottomLeft, min.x, max.y, min.z);
		BOX_FIELD_SET(upperBottomRight, max.x, max.y, min.z);

#undef BOX_FIELD_SET

		return box;
	}

	static void ScaleBoxScreen(BoxScreen& boxScreen)
	{
		if (s_ResolutionScale.x != 0)
		{

#define SCALE_FIELD(field) boxScreen.##field##.x *= s_ResolutionScale.x; boxScreen.##field##.y *= s_ResolutionScale.y

			SCALE_FIELD(lowerTopLeft);
			SCALE_FIELD(lowerTopRight);
			SCALE_FIELD(lowerBottomLeft);
			SCALE_FIELD(lowerBottomRight);

			SCALE_FIELD(upperTopLeft);
			SCALE_FIELD(upperTopRight);
			SCALE_FIELD(upperBottomLeft);
			SCALE_FIELD(upperBottomRight);

#undef SCALE_FIELD

		}

		auto screenHeight = app::Screen_get_height(nullptr, nullptr);

#define FIX_Y(field) boxScreen.##field##.y = screenHeight - boxScreen.##field##.y

		FIX_Y(lowerTopLeft);
		FIX_Y(lowerTopRight);
		FIX_Y(lowerBottomLeft);
		FIX_Y(lowerBottomRight);

		FIX_Y(upperTopLeft);
		FIX_Y(upperTopRight);
		FIX_Y(upperBottomLeft);
		FIX_Y(upperBottomRight);

#undef FIX_Y

	}

	static Rect GetEntityScreenRect(const BoxScreen& box, bool scalling = true)
	{
		Rect boxRect{};

		boxRect.xMin = std::min({ box.lowerTopLeft.x, box.lowerTopRight.x, box.lowerBottomLeft.x, box.lowerBottomRight.x,
			box.upperTopLeft.x, box.upperTopRight.x, box.upperBottomRight.x, box.upperBottomLeft.x });
		boxRect.xMax = std::max({ box.lowerTopLeft.x, box.lowerTopRight.x, box.lowerBottomLeft.x, box.lowerBottomRight.x,
			box.upperTopLeft.x, box.upperTopRight.x, box.upperBottomRight.x, box.upperBottomLeft.x });

		boxRect.yMin = std::max({ box.lowerTopLeft.y, box.lowerTopRight.y, box.lowerBottomLeft.y, box.lowerBottomRight.y,
			box.upperTopLeft.y, box.upperTopRight.y, box.upperBottomRight.y, box.upperBottomLeft.y });
		boxRect.yMax = std::min({ box.lowerTopLeft.y, box.lowerTopRight.y, box.lowerBottomLeft.y, box.lowerBottomRight.y,
			box.upperTopLeft.y, box.upperTopRight.y, box.upperBottomRight.y, box.upperBottomLeft.y });

		if (!scalling)
		{
			std::swap(boxRect.yMin, boxRect.yMax);
			return boxRect;
		}

		if (s_ResolutionScale.x != 0)
		{
			boxRect.xMin *= s_ResolutionScale.x;
			boxRect.xMax *= s_ResolutionScale.x;

			boxRect.yMin *= s_ResolutionScale.y;
			boxRect.yMax *= s_ResolutionScale.y;
		}

		auto screenHeight = app::Screen_get_height(nullptr, nullptr);
		boxRect.yMin = screenHeight - boxRect.yMin;
		boxRect.yMax = screenHeight - boxRect.yMax;
		return boxRect;
	}

	static void DrawQuadLines(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col)
	{
		auto draw = ImGui::GetBackgroundDrawList();

		draw->AddLine(p1, p2, col);
		draw->AddLine(p2, p3, col);
		draw->AddLine(p3, p4, col);
		draw->AddLine(p4, p1, col);
	}

	static bool HasCenter(const Rect& rect)
	{
		ImVec2 centerPoint = ImVec2(s_ScreenResolution.x / 2, s_ScreenResolution.y / 2);

		return rect.xMin < centerPoint.x&& centerPoint.x < rect.xMax &&
			rect.yMin < centerPoint.y&& centerPoint.y < rect.yMax;
	}

	static Rect DrawRect(game::Entity* entity, const ImColor& color)
	{
		auto box = GetEntityScreenBox(entity);
		if (!box)
			return {};

		auto entityRect = GetEntityScreenRect(*box);
		if (entityRect.empty())
			return {};

		auto& esp = ESP::GetInstance();
		auto draw = ImGui::GetBackgroundDrawList();

		auto pMin = ImVec2(entityRect.xMin, entityRect.yMin);
		auto pMax = ImVec2(entityRect.xMax, entityRect.yMax);
		if (esp.f_Fill)
		{
			ImColor newColor = color;
			newColor.Value.w = 1.0f - esp.f_FillTransparency;
			draw->AddRectFilled(pMin, pMax, newColor);
		}
		draw->AddRect(pMin, pMax, color);

		return entityRect;
	}

	// Callow: This way to drawing is slower than native
	static Rect DrawBox(game::Entity* entity, const ImColor& color)
	{
		auto box = GetEntityScreenBox(entity);
		if (!box)
			return {};

		ScaleBoxScreen(*box);
		auto& esp = ESP::GetInstance();
		auto draw = ImGui::GetBackgroundDrawList();

		if (esp.f_Fill)
		{
			ImColor newColor = color;
			newColor.Value.w = 1.0f - esp.f_FillTransparency;

			float threshold = 2.0f;
#define ADD_FIXED_QUAD(p1, p2, p3, p4, col) {\
                                                ImVec2 p13 { std::abs(p3.x - p1.x), std::abs(p3.y - p1.y) };\
                                                ImVec2 p24 { std::abs(p2.x - p4.x), std::abs(p2.y - p4.y) };\
                                                if ((p13.x > threshold && p13.y > threshold) || \
                                                    (p24.x > threshold && p24.y > threshold) || \
                                                    (p13.x > threshold && p24.y > threshold) || \
                                                    (p24.x > threshold && p13.y > threshold))\
                                                        draw->AddQuadFilled(p1, p2, p3, p4, newColor);\
                                            }

			ADD_FIXED_QUAD(box->lowerBottomLeft, box->lowerTopLeft, box->lowerTopRight, box->lowerBottomRight, newColor);
			ADD_FIXED_QUAD(box->upperBottomLeft, box->upperTopLeft, box->upperTopRight, box->upperBottomRight, newColor);

			ADD_FIXED_QUAD(box->lowerBottomLeft, box->upperBottomLeft, box->upperBottomRight, box->lowerBottomRight, newColor);
			ADD_FIXED_QUAD(box->lowerTopLeft, box->upperTopLeft, box->upperTopRight, box->lowerTopRight, newColor);

			ADD_FIXED_QUAD(box->lowerBottomLeft, box->lowerTopLeft, box->upperTopLeft, box->upperBottomLeft, newColor);
			ADD_FIXED_QUAD(box->lowerBottomRight, box->lowerTopRight, box->upperTopRight, box->upperBottomRight, newColor);

#undef ADD_FIXED_QUAD
		}

		DrawQuadLines(box->lowerBottomLeft, box->lowerTopLeft, box->lowerTopRight, box->lowerBottomRight, color);
		DrawQuadLines(box->upperBottomLeft, box->upperTopLeft, box->upperTopRight, box->upperBottomRight, color);

		draw->AddLine(box->lowerBottomLeft, box->upperBottomLeft, color);
		draw->AddLine(box->lowerTopLeft, box->upperTopLeft, color);
		draw->AddLine(box->lowerTopRight, box->upperTopRight, color);
		draw->AddLine(box->lowerBottomRight, box->upperBottomRight, color);

		auto rect = GetEntityScreenRect(*box, false);
		return rect;
	}

	static void UpdateAvatarPosition()
	{
		SAFE_BEGIN();
		auto& manager = game::EntityManager::instance();
		auto avatarPos = WorldToScreenPosScalled(manager.avatar()->relativePosition());

		s_AvatarPosition = ImVec2(avatarPos.x, avatarPos.y);
		SAFE_EEND();
	}

	static std::optional<ImVec2> GetEntityScreenPos(game::Entity* entity)
	{
		auto targetPos = WorldToScreenPosScalled(entity->relativePosition());
		if (targetPos.z < 1)
			return {};

		return ImVec2(targetPos.x, targetPos.y);
	}

	static void DrawLine(game::Entity* entity, const ImColor& color)
	{
		auto screenPos = GetEntityScreenPos(entity);
		if (!screenPos)
			return;

		auto draw = ImGui::GetBackgroundDrawList();
		draw->AddLine(s_AvatarPosition, *screenPos, color);
	}

	static void DrawName(const Rect& boxRect, game::Entity* entity, const std::string& name, const ImColor& color)
	{
		auto& esp = ESP::GetInstance();
		auto& manager = game::EntityManager::instance();
		
		std::string text;
		if (esp.f_DrawName && esp.f_DrawDistance)
			text = fmt::format("{} | {:.1f}m", name, manager.avatar()->distance(entity));
		else if (esp.f_DrawDistance)
			text = fmt::format("{:.1f}m", manager.avatar()->distance(entity));
		else
			text = name;

		ImVec2 namePosition;
		if (!boxRect.empty())
			namePosition = { boxRect.xMin, boxRect.yMin - esp.f_FontSize };
		else
		{
			auto screenPos = GetEntityScreenPos(entity);
			if (!screenPos)
				return;
			namePosition = *screenPos;

			// Might need to be aware of performance hit but there shouldn't be any.
			ImGuiContext& g = *GImGui;
			ImFont* font = g.Font;
			auto textSize = font->CalcTextSizeA(esp.f_FontSize, FLT_MAX, FLT_MAX, text.c_str());
			namePosition.x -= (textSize.x / 2.0f);
			namePosition.y -= esp.f_FontSize;
		}

		auto draw = ImGui::GetBackgroundDrawList();
		draw->AddText(NULL, esp.f_FontSize, namePosition, color, text.c_str());
	}

	bool DrawEntity(const std::string& name, game::Entity* entity, const ImColor& color)
	{
		SAFE_BEGIN();
		auto& esp = ESP::GetInstance();

		Rect rect;
		switch (esp.f_DrawBoxMode.value())
		{
		case ESP::DrawMode::Box:
			rect = DrawBox(entity, esp.f_ApplyGlobalColor ? esp.f_BoxColor : color);
			break;
		case ESP::DrawMode::Rectangle:
			rect = DrawRect(entity, esp.f_ApplyGlobalColor ? esp.f_RectColor : color);
			break;
		default:
			rect = {};
			break;
		}

		if (esp.f_DrawLine)
			DrawLine(entity, esp.f_ApplyGlobalColor ? esp.f_LineColor : color);

		if (esp.f_DrawName)
			DrawName(rect, entity, name, esp.f_ApplyGlobalColor ? esp.f_FontColor : color);

		return HasCenter(rect);
		SAFE_ERROR();
		return false;
		SAFE_END();
	}

	void PrepareFrame()
	{
		UpdateMainCamera();
		UpdateResolutionScale();

		auto& esp = ESP::GetInstance();
		if (esp.f_DrawLine)
			UpdateAvatarPosition();
	}
}
