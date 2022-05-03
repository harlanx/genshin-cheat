#pragma once
#include <cheat-base/cheat/Feature.h>
#include <cheat-base/config/config.h>

#include <il2cpp-appdata.h>

namespace cheat::feature 
{

	class InfiniteStamina : public Feature
    {
	public:
		config::Field<config::Toggle<Hotkey>> f_Enabled;
		config::Field<bool> f_PacketReplacement;

		static InfiniteStamina& GetInstance();

		const FeatureGUIInfo& GetGUIInfo() const override;
		void DrawMain() override;

		virtual bool NeedStatusDraw() const override;
		void DrawStatus() override;
	
		bool OnPropertySet(app::PropType__Enum propType);
		void OnMoveSync(uint32_t entityId, app::MotionInfo* syncInfo);

	private:
		InfiniteStamina();
	};
}

