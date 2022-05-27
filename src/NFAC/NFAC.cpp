#include "NFAC.h"

static inline bool toggle = true;
uintptr_t funcCheckValidTargetOriginal = 0;
uintptr_t funcPCCheckValidTargetOriginal = 0;

#ifdef SKYRIM_SUPPORT_AE
void Loki::NoFollowerAttackCollision::InstallMeleeHook() {
	REL::Relocation<std::uintptr_t> MeleeHook{ REL::ID(38603) }; //+45A
	// SkyrimSE.exe+64E54A - E8 B10F0000           - call SkyrimSE.exe+64F500

	auto& trmp = SKSE::GetTrampoline();
	_MeleeFunction = trmp.write_call<5>(MeleeHook.address() + 0x45A, MeleeFunction);

	logger::info("Unk Hook injected");
}

void Loki::NoFollowerAttackCollision::InstallSweepHook() {
	REL::Relocation<std::uintptr_t> SweepHook{ REL::ID(38603) }; //+40A
	// SkyrimSE.exe+64E4FA - E8 01100000           - call SkyrimSE.exe+64F500

	auto& trmp = SKSE::GetTrampoline();
	_SweepFunction = trmp.write_call<5>(SweepHook.address() + 0x40A, SweepFunction);

	logger::info("Unk Hook 2 injected");
}

void Loki::NoFollowerAttackCollision::InstallArrowHook() {
	REL::Relocation<std::uintptr_t> arrowHook{ REL::ID(44218) }; //+90
	// SkyrimSE.exe+782E60 - E8 9BC6ECFF           - call SkyrimSE.exe+64F500

	auto& trmp = SKSE::GetTrampoline();
	_ArrowFunction = trmp.write_call<5>(arrowHook.address() + 0x90, ArrowFunction);

	logger::info("Arrow hook injected");
}

#else
void Loki::NoFollowerAttackCollision::InstallMeleeHook() {
	REL::Relocation<std::uintptr_t> MeleeHook{ REL::ID(37650) }; //+38B

	auto& trmp = SKSE::GetTrampoline();
	_MeleeFunction = trmp.write_call<5>(MeleeHook.address() + 0x38B, MeleeFunction);

	logger::info("Unk Hook injected");
}

void Loki::NoFollowerAttackCollision::InstallSweepHook() {
	REL::Relocation<std::uintptr_t> SweepHook{ REL::ID(37689) }; //+DD

	auto& trmp = SKSE::GetTrampoline();
	_SweepFunction = trmp.write_call<5>(SweepHook.address() + 0xDD, SweepFunction);

	logger::info("Unk Hook 2 injected");
}

void Loki::NoFollowerAttackCollision::InstallArrowHook() {
	REL::Relocation<std::uintptr_t> arrowHook{ REL::ID(43027) }; //+90

	auto& trmp = SKSE::GetTrampoline();
	_ArrowFunction = trmp.write_call<5>(arrowHook.address() + 0x90, ArrowFunction);

	logger::info("Arrow hook injected");
}
#endif

// This hook is used as a fallback if the magic hook somehow failed.
// It prevents hostile effects from applying but not the pain so the target might retaliate despite taking no harm.
void Loki::NoFollowerAttackCollision::InstallVaildTargetHook()
{
	REL::Relocation<uintptr_t> vtbl{ REL::ID( RE::VTABLE_Character[0] ) };
	funcCheckValidTargetOriginal = vtbl.write_vfunc( 0xD6, &Loki::NoFollowerAttackCollision::IsTargetVaild );

	REL::Relocation<uintptr_t> vtblPC{ REL::ID( RE::VTABLE_PlayerCharacter[0] ) };
	funcPCCheckValidTargetOriginal = vtblPC.write_vfunc( 0xD6, &Loki::NoFollowerAttackCollision::IsTargetVaild );
}

void Loki::NoFollowerAttackCollision::InstallInputSink() {
	auto deviceMan = RE::BSInputDeviceManager::GetSingleton();
	deviceMan->AddEventSink(OnInput::GetSingleton());
}

void Loki::NoFollowerAttackCollision::MeleeFunction(RE::Character* a_aggressor, RE::Actor* a_victim, std::int64_t a3, char a4, float a5) {

	if (!a_victim || !a_aggressor || !toggle) { return _MeleeFunction(a_aggressor, a_victim, a3, a4, a5); }

	if( !IsTargetVaild( a_aggressor, *a_victim ) ) return;

	return _MeleeFunction(a_aggressor, a_victim, a3, a4, a5);

}

void Loki::NoFollowerAttackCollision::SweepFunction(RE::Character* a_aggressor, RE::Actor* a_victim, std::int64_t a3, char a4, float a5) {

	if (!a_victim || !a_aggressor || !toggle) { return _SweepFunction(a_aggressor, a_victim, a3, a4, a5); }

	if( !IsTargetVaild( a_aggressor, *a_victim ) ) return;

	return _SweepFunction(a_aggressor, a_victim, a3, a4, a5);

}

void Loki::NoFollowerAttackCollision::ArrowFunction(RE::Character* a_aggressor, RE::Actor* a_victim, std::int64_t a3, char a4, float a5) {

	if (!a_victim || !a_aggressor || !toggle) { return _ArrowFunction(a_aggressor, a_victim, a3, a4, a5); }

	if( !IsTargetVaild( a_aggressor, *a_victim ) ) return;

	return _ArrowFunction(a_aggressor, a_victim, a3, a4, a5);

}

bool Loki::NoFollowerAttackCollision::IsTargetVaild( RE::Actor* a_this, RE::TESObjectREFR& a_target )
{
	bool isValid = true;
	if( a_target.Is( RE::FormType::ActorCharacter ) && toggle )
	{
		auto target = static_cast<RE::Actor*>( &a_target );
		if( !target->IsDead() && !target->IsHostileToActor( a_this ) )
		{
			auto targetOwnerHandle		= target->GetCommandingActor();
			auto thisOwnerHandle		= a_this->GetCommandingActor();
			auto targetOwner			= targetOwnerHandle ? targetOwnerHandle.get() : nullptr;
			auto thisOwner				= thisOwnerHandle ? thisOwnerHandle.get() : nullptr;

			bool isThisPlayer			= a_this->IsPlayerRef();
			bool isThisTeammate			= a_this->IsPlayerTeammate();
			bool isThisSummonedByPC		= a_this->IsSummoned() && thisOwner && thisOwner->IsPlayerRef();
			bool isTargetPlayer			= target->IsPlayerRef();
			bool isTargetTeammate		= target->IsPlayerTeammate();
			bool isTargetSummonedByPC	= target->IsSummoned() && targetOwner && targetOwner->IsPlayerRef();
			bool isTargetAGuard			= target->IsGuard();
			bool isTargetAMount			= target->IsAMount();

			// Prevent player's team from hitting each other, a guard or a mount.
			if( (isThisPlayer || isThisTeammate || isThisSummonedByPC) &&
				(isTargetPlayer || isTargetTeammate || isTargetSummonedByPC || isTargetAGuard || isTargetAMount) )
				isValid = false;

			// Protect neutral actor if desired
			if( protectNeutralActor )
			{
				// If the target is commanded by the hostile actor, do not protect it
				auto targetCommanderHandle = target->GetCommandingActor();
				if( targetCommanderHandle )
				{
					auto targetCommander = targetCommanderHandle.get();
					if( targetCommander->IsHostileToActor( a_this ) )
						isValid = true;
					else
						isValid = false;
				}
				else
					isValid = false;
			}
				

			// Override valid status if set to disable outside combat
			if( !isValid && disableOutsideCombat && !a_this->IsInCombat() )
				isValid = true;
		}
	}

	//return isValid;
	// 41241
	if( a_this->IsPlayerRef() )
	{
		using func_t = decltype(&IsTargetVaild);
		REL::Relocation<func_t> func( funcPCCheckValidTargetOriginal );
		return isValid && func( a_this, a_target );
	}
	else
	{
		using func_t = decltype(&IsTargetVaild);
		REL::Relocation<func_t> func( funcCheckValidTargetOriginal );
		return isValid && func( a_this, a_target );
	}
}


RE::BSEventNotifyControl Loki::OnInput::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource) {
	using EventType = RE::INPUT_EVENT_TYPE;
	using DeviceType = RE::INPUT_DEVICE;

	for (auto event = *a_event; event; event = event->next) {
		if (event->eventType != EventType::kButton) {
			continue;
		}

		auto button = skyrim_cast<RE::ButtonEvent*>(event);
		if (button->IsDown()) {
			auto key = button->idCode;
			switch (button->device.get()) {

				case DeviceType::kMouse: {
					key += NoFollowerAttackCollision::kMouseOffset;
					break;
				}

				case DeviceType::kKeyboard: {
					key += NoFollowerAttackCollision::kKeyboardOffset;
					break;
				}

				case DeviceType::kGamepad: {
					key = NoFollowerAttackCollision::GetGamepadIndex((RE::BSWin32GamepadDevice::Key)key);
					break;
				}

				default: continue;

			}

			auto ui = RE::UI::GetSingleton();
			auto controlmap = RE::ControlMap::GetSingleton();
			if (ui->GameIsPaused() || !controlmap->IsMovementControlsEnabled()) {
				continue;
			}

			//RE::ConsoleLog::GetSingleton()->Print("mod key -> %i", NoFollowerAttackCollision::toggleKey);
			//RE::ConsoleLog::GetSingleton()->Print("event key -> %i", key);
			if (key == NoFollowerAttackCollision::toggleKey) {
				if (!toggle) {
					toggle = true;
					RE::DebugNotification("NFAC: On");
				} else {
					toggle = false;
					RE::DebugNotification("NFAC: Off");
				}
			}
		}
	}

	return RE::BSEventNotifyControl::kContinue;
}