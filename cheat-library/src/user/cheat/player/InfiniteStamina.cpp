#include "pch-il2cpp.h"
#include "InfiniteStamina.h"

#include <helpers.h>
#include <cheat/events.h>
#include <cheat/game/EntityManager.h>

namespace cheat::feature 
{
	static void AvatarPropDictionary_SetItem_Hook(app::Dictionary_2_JNHGGGCKJNA_JKNLDEEBGLL_* __this, app::JNHGGGCKJNA key,
		app::JKNLDEEBGLL value, MethodInfo* method);

    InfiniteStamina::InfiniteStamina() : Feature(),
        NF(f_Enabled, "Infinite stamina", "InfiniteStamina", false),
        NF(f_PacketReplacement, "Move sync packet replacement", "InfiniteStamina", false)
    {
		HookManager::install(app::AvatarPropDictionary_SetItem, AvatarPropDictionary_SetItem_Hook);

		events::MoveSyncEvent += MY_METHOD_HANDLER(InfiniteStamina::OnMoveSync);
    }

    const FeatureGUIInfo& InfiniteStamina::GetGUIInfo() const
    {
        static const FeatureGUIInfo info { "Infinite Stamina", "Player", true };
        return info;
    }

    void InfiniteStamina::DrawMain()
    {
		ConfigWidget("Enabled", f_Enabled, "Enables infinite stamina option.");

		ConfigWidget("Move Sync Packet Replacement", f_PacketReplacement,
			"This mode prevents sending server packets with stamina cost actions,\n" \
			"e.g. swim, climb, sprint, etc.\n" \
			"NOTE: This is may be more safe than the standard method. More testing is needed.");
    }

    bool InfiniteStamina::NeedStatusDraw() const
{
        return f_Enabled;
    }

    void InfiniteStamina::DrawStatus() 
    { 
        ImGui::Text("Inf. Stamina [%s]", f_PacketReplacement ? "Packet" : "Normal");
    }

    InfiniteStamina& InfiniteStamina::GetInstance()
    {
        static InfiniteStamina instance;
        return instance;
    }

	// Infinite stamina offline mode. Blocks changes for stamina property. 
	// Note. Changes received from the server (not sure about this for current time), 
	//       that means that server know our stamina, and changes it in client can be detected.
	// Not working for water because server sending drown action when your stamina down to zero. (Also guess for now)
	bool InfiniteStamina::OnPropertySet(app::PropType__Enum propType) 
	{
		using PT = app::PropType__Enum;

		return !f_Enabled || f_PacketReplacement ||
					(propType != PT::PROP_MAX_STAMINA &&
				     propType != PT::PROP_CUR_PERSIST_STAMINA &&
					 propType != PT::PROP_CUR_TEMPORARY_STAMINA);
	}

	// Infinite stamina packet mode.
	// Note. Blocking packets with movement information, to prevent ability server to know stamina info.
	//       But server may see incorrect movements. What mode safer don't tested.
	void InfiniteStamina::OnMoveSync(uint32_t entityId, app::MotionInfo* syncInfo)
	{
		static bool afterDash = false;

		auto& manager = game::EntityManager::instance();
		if (manager.avatar()->runtimeID() != entityId)
			return;

		// LOG_DEBUG("Movement packet: %s", magic_enum::enum_name(syncInfo->fields.motionState).data());
		if (f_Enabled && f_PacketReplacement)
		{
			auto state = syncInfo->fields.motionState;
			switch (state)
			{
			case app::MotionState__Enum::MotionDash:
			case app::MotionState__Enum::MotionClimb:
			case app::MotionState__Enum::MotionClimbJump:
			case app::MotionState__Enum::MotionStandbyToClimb:
			case app::MotionState__Enum::MotionSwimDash:
			case app::MotionState__Enum::MotionSwimIdle:
			case app::MotionState__Enum::MotionSwimMove:
			case app::MotionState__Enum::MotionSwimJump:
			case app::MotionState__Enum::MotionFly:
			case app::MotionState__Enum::MotionFight:
			case app::MotionState__Enum::MotionDashBeforeShake:
			case app::MotionState__Enum::MotionDangerDash:
				syncInfo->fields.motionState = app::MotionState__Enum::MotionRun;
				break;
			case app::MotionState__Enum::MotionJump:
				if (afterDash)
					syncInfo->fields.motionState = app::MotionState__Enum::MotionRun;
				break;
			}
			if (state != app::MotionState__Enum::MotionJump && state != app::MotionState__Enum::MotionFallOnGround)
				afterDash = state == app::MotionState__Enum::MotionDash;
		}
	}

	static void AvatarPropDictionary_SetItem_Hook(app::Dictionary_2_JNHGGGCKJNA_JKNLDEEBGLL_* __this, app::JNHGGGCKJNA key, app::JKNLDEEBGLL value, MethodInfo* method)
	{
		app::PropType__Enum propType = app::AvatarProp_DecodePropType(nullptr, key, nullptr);
		auto& infiniteStamina = InfiniteStamina::GetInstance();
		if (infiniteStamina.OnPropertySet(propType))
			CALL_ORIGIN(AvatarPropDictionary_SetItem_Hook, __this, key, value, method);
	}
}

