#include "pch-il2cpp.h"
#include "AutoLoot.h"

#include <helpers.h>
#include <cheat/events.h>
#include <cheat/game/EntityManager.h>
#include <cheat/game/filters.h>
#include <cheat/game/Chest.h>

namespace cheat::feature 
{
	static void LCSelectPickup_AddInteeBtnByID_Hook(void* __this, app::BaseEntity* entity, MethodInfo* method);
	static bool LCSelectPickup_IsInPosition_Hook(void* __this, app::BaseEntity* entity, MethodInfo* method);
	static bool LCSelectPickup_IsOutPosition_Hook(void* __this, app::BaseEntity* entity, MethodInfo* method);

    AutoLoot::AutoLoot() : Feature(),
        NF(f_AutoLoot,       "Auto loot",               "AutoLoot", false),
		NF(f_OpenChest,      "Open Chest",              "AutoLoot", false),
        NF(f_DelayTime,      "Delay time (in ms)",      "AutoLoot", 150),
        NF(f_UseCustomRange, "Use custom pickup range", "AutoLoot", false),
        NF(f_CustomRange,    "Pickup Range",            "AutoLoot", 5.0f),
		toBeLootedItems(), nextLootTime(0)
    {
		// Auto loot
		HookManager::install(app::LCSelectPickup_AddInteeBtnByID, LCSelectPickup_AddInteeBtnByID_Hook);
		HookManager::install(app::LCSelectPickup_IsInPosition, LCSelectPickup_IsInPosition_Hook);
		HookManager::install(app::LCSelectPickup_IsOutPosition, LCSelectPickup_IsOutPosition_Hook);

		events::GameUpdateEvent += MY_METHOD_HANDLER(AutoLoot::OnGameUpdate);
	}

    const FeatureGUIInfo& AutoLoot::GetGUIInfo() const
    {
        static const FeatureGUIInfo info{ "Auto Loot", "World", true };
        return info;
    }

    void AutoLoot::DrawMain()
    {
		ConfigWidget("Auto loot", f_AutoLoot, "Loots dropped items.\n" \
            "Note: Custom range and low delay times are high-risk features.\n" \
			"Abuse will definitely merit a ban.");
		ConfigWidget("Open Chest", f_OpenChest, "Auto Open Chest.\n" \
			"Note: Custom range and low delay times are high-risk features.\n" \
			"Abuse will definitely merit a ban.");

		ConfigWidget("Delay Time (ms)", f_DelayTime, 1, 0, 1000, "Delay (in ms) beetwen looting items.\n" \
            "Values under 200ms are unsafe.");
		ConfigWidget("Use Custom Pickup Range", f_UseCustomRange, "Enable custom pickup range.\n" \
            "Using this feature is not recommended, as it is easily detected by the server.");
		ConfigWidget("Range (m)", f_CustomRange, 0.1f, 0.5f, 60.0f, "Modifies pickup range to this value (in meters).");
    }

    bool AutoLoot::NeedStatusDraw() const
{
        return f_AutoLoot;
    }

    void AutoLoot::DrawStatus() 
    {
		ImGui::Text("Auto Loot [%dms%s]",
			f_DelayTime.value(),
			f_UseCustomRange ? fmt::format("|{:.1f}m ", f_CustomRange.value()).c_str() : "");

		if (f_OpenChest)
			ImGui::Text("Auto Open Chest");
    }

    AutoLoot& AutoLoot::GetInstance()
    {
        static AutoLoot instance;
        return instance;
    }

	bool AutoLoot::OnCreateButton(app::BaseEntity* entity)
	{
		if (!f_AutoLoot)
			return false;

		auto itemModule = GET_SINGLETON(ItemModule);
		if (itemModule == nullptr)
			return false;

		auto entityId = entity->fields._runtimeID_k__BackingField;
		if (f_DelayTime == 0)
		{
			app::ItemModule_PickItem(itemModule, entityId, nullptr);
			return true;
		}

		toBeLootedItems.push(entityId);
		return false;
	}

	void AutoLoot::OnGameUpdate()
	{
		auto currentTime = util::GetCurrentTimeMillisec();
		if (currentTime < nextLootTime)
			return;

		auto entityManager = GET_SINGLETON(EntityManager);
		if (entityManager == nullptr)
			return;

		// RyujinZX#6666
		if (f_OpenChest) 
		{
			auto& manager = game::EntityManager::instance();
			for (auto& entity : manager.entities(game::filters::combined::Chests)) 
			{
				float range = f_UseCustomRange ? f_CustomRange : 3.5f;
				if (manager.avatar()->distance(entity) >= range)
					continue;

				auto chest = reinterpret_cast<game::Chest*>(entity);

				auto ChestState = chest->chestState();
				if (ChestState != game::Chest::ChestState::None)
					continue;

				uint32_t entityId = entity->runtimeID();
				toBeLootedItems.push(entityId);
			}
		}

		auto entityId = toBeLootedItems.pop();
		if (!entityId)
			return;

		auto itemModule = GET_SINGLETON(ItemModule);
		if (itemModule == nullptr)
			return;

		auto entity = app::EntityManager_GetValidEntity(entityManager, *entityId, nullptr);
		if (entity == nullptr)
			return;

		app::ItemModule_PickItem(itemModule, *entityId, nullptr);
		nextLootTime = currentTime + (int)f_DelayTime;
	}

	void AutoLoot::OnCheckIsInPosition(bool& result, app::BaseEntity* entity)
	{
		if (f_AutoLoot && f_UseCustomRange)
		{
			auto& manager = game::EntityManager::instance();
			result = manager.avatar()->distance(entity) < f_CustomRange;
		}
	}

	static void LCSelectPickup_AddInteeBtnByID_Hook(void* __this, app::BaseEntity* entity, MethodInfo* method)
	{
		AutoLoot& autoLoot = AutoLoot::GetInstance();
		bool canceled = autoLoot.OnCreateButton(entity);
		if (!canceled)
			callOrigin(LCSelectPickup_AddInteeBtnByID_Hook, __this, entity, method);
	}

	static bool LCSelectPickup_IsInPosition_Hook(void* __this, app::BaseEntity* entity, MethodInfo* method)
	{
		bool result = callOrigin(LCSelectPickup_IsInPosition_Hook, __this, entity, method);

		AutoLoot& autoLoot = AutoLoot::GetInstance();
		autoLoot.OnCheckIsInPosition(result, entity);

		return result;
	}

	static bool LCSelectPickup_IsOutPosition_Hook(void* __this, app::BaseEntity* entity, MethodInfo* method)
	{
		bool result = callOrigin(LCSelectPickup_IsOutPosition_Hook, __this, entity, method);

		AutoLoot& autoLoot = AutoLoot::GetInstance();
		autoLoot.OnCheckIsInPosition(result, entity);

		return result;
	}
}

