#pragma once
#include <cheat-base/cheat/Feature.h>
#include <cheat-base/config/config.h>

namespace cheat::feature 
{
	class NoCD : public Feature
    {
	public:
		config::Field<config::ToggleHotkey> f_AbilityReduce;
		config::Field<float> f_AbilityReduceValue;

		config::Field<config::ToggleHotkey> f_UtimateMaxEnergy;
		config::Field<config::ToggleHotkey> f_Sprint;
		config::Field<config::ToggleHotkey> f_InstantBow;

		static NoCD& GetInstance();

		const FeatureGUIInfo& GetGUIInfo() const override;
		void DrawMain() override;

		virtual bool NeedStatusDraw() const override;
		void DrawStatus() override;
	
	private:
		NoCD();
	};
}

