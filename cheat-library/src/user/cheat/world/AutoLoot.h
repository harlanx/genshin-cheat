#pragma once
#include <cheat-base/cheat/Feature.h>
#include <cheat-base/config/config.h>
#include <cheat-base/thread-safe.h>

namespace cheat::feature 
{

	class AutoLoot : public Feature
    {
	public:
		config::Field<config::ToggleHotkey> f_AutoLoot;
		config::Field<config::ToggleHotkey> f_OpenChest;

		config::Field<int> f_DelayTime;
		config::Field<bool> f_UseCustomRange;
		config::Field<float> f_CustomRange;

		static AutoLoot& GetInstance();

		const FeatureGUIInfo& GetGUIInfo() const override;
		void DrawMain() override;

		virtual bool NeedStatusDraw() const override;
		void DrawStatus() override;

		bool OnCreateButton(app::BaseEntity* entity);
		void OnGameUpdate();
		void OnCheckIsInPosition(bool& result, app::BaseEntity* entity);
	
	private:
		
		SafeQueue<uint32_t> toBeLootedItems;
		SafeValue<int64_t> nextLootTime;
		AutoLoot();
	};
}

