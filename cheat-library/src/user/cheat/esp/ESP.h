#pragma once
#include <cheat-base/cheat/Feature.h>
#include <cheat-base/config/config.h>

#include <cheat/esp/ESPItem.h>
#include <cheat/game/IEntityFilter.h>
#include <cheat/game/CacheFilterExecutor.h>

namespace cheat::feature 
{

	class ESP : public Feature
    {
	public:
		enum class DrawMode
		{
			None,
			Rectangle,
			Box
		};

		config::Field<config::ToggleHotkey> f_Enabled;
		config::Field<config::Enum<DrawMode>> f_DrawBoxMode;
		config::Field<bool> f_Fill;
		config::Field<float> f_FillTransparency;

		config::Field<bool> f_DrawLine;
		config::Field<bool> f_DrawDistance;
		config::Field<bool> f_DrawName;

		config::Field<float> f_FontSize;
		config::Field<ImColor> f_FontColor;
		config::Field<ImColor> f_BoxColor;
		config::Field<ImColor> f_RectColor;
		config::Field<ImColor> f_LineColor;
		config::Field<bool> f_ApplyGlobalColor;

		config::Field<float> f_MinSize;
		config::Field<float> f_Range;

		std::string m_Search;

		static ESP& GetInstance();

		const FeatureGUIInfo& GetGUIInfo() const override;
		void DrawMain() override;

		virtual bool NeedStatusDraw() const override;
		void DrawStatus() override;

		void DrawExternal() override;

	private:
		using FilterInfo = std::pair<config::Field<esp::ESPItem>, game::IEntityFilter*>;
		using Filters = std::vector<FilterInfo>;
		using Sections = std::map<std::string, Filters>;

		Sections m_Sections;
		game::CacheFilterExecutor m_FilterExecutor;

		void InstallFilters();
		void AddFilter(const std::string& section, const std::string& name, game::IEntityFilter* filter);
		
		void DrawSection(const std::string& section, const Filters& filters);
		void DrawFilterField(const config::Field<esp::ESPItem>& field);

		void OnKeyUp(short key, bool& cancelled);

		ESP();
	};
}

