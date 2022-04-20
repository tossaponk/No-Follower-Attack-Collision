#pragma once
#include <SimpleIni.h>

namespace Loki {

	class OnInput :
		public RE::BSTEventSink<RE::InputEvent*> {

	public:
		static OnInput* GetSingleton() {
			static OnInput singleton;
			return &singleton;
		}
		virtual RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;

	protected:
		OnInput() = default;
		OnInput(const OnInput&) = delete;
		OnInput(OnInput&&) = delete;
		virtual ~OnInput() = default;

		auto operator=(const OnInput&)->OnInput & = delete;
		auto operator=(OnInput&&)->OnInput & = delete;

	};

	class NoFollowerAttackCollision {

	public:
		enum Offset : std::uint32_t {
			kInvalid = static_cast<std::uint32_t>(-1),
			kKeyboardOffset = 0,
			kMouseOffset = 256,
			kGamepadOffset = 266
		};
		static std::uint32_t GetGamepadIndex(RE::BSWin32GamepadDevice::Key a_key)
		{
			using Key = RE::BSWin32GamepadDevice::Key;

			std::uint32_t index;
			switch (a_key)
			{
				case Key::kUp:
					index = 0;
					break;
				case Key::kDown:
					index = 1;
					break;
				case Key::kLeft:
					index = 2;
					break;
				case Key::kRight:
					index = 3;
					break;
				case Key::kStart:
					index = 4;
					break;
				case Key::kBack:
					index = 5;
					break;
				case Key::kLeftThumb:
					index = 6;
					break;
				case Key::kRightThumb:
					index = 7;
					break;
				case Key::kLeftShoulder:
					index = 8;
					break;
				case Key::kRightShoulder:
					index = 9;
					break;
				case Key::kA:
					index = 10;
					break;
				case Key::kB:
					index = 11;
					break;
				case Key::kX:
					index = 12;
					break;
				case Key::kY:
					index = 13;
					break;
				case Key::kLeftTrigger:
					index = 14;
					break;
				case Key::kRightTrigger:
					index = 15;
					break;
				default:
					index = kInvalid;
					break;
			}

			return index != kInvalid ? index + kGamepadOffset : kInvalid;
		}

		NoFollowerAttackCollision() {
			CSimpleIniA ini;
			ini.SetUnicode();
			auto filename = L"Data/SKSE/Plugins/loki_NFAC.ini";
			SI_Error rc = ini.LoadFile(filename);

			toggleKey = ini.GetLongValue("SETTINGS", "key", -1);
			protectNeutralActor = ini.GetBoolValue("SETTINGS", "bProtectNeutralActor", false);
		}
		~NoFollowerAttackCollision() {

		}
		static NoFollowerAttackCollision* GetSingleton() {
			static NoFollowerAttackCollision* singleton = new NoFollowerAttackCollision();
			return singleton;
		}

		static void InstallMeleeHook();
		static void InstallSweepHook();
		static void InstallArrowHook();
		static void InstallVaildTargetHook();

		// This hook includes magic projectile from spells and enchanted arrows
		struct MagicProjectileHitHook
		{
			static void thunk( RE::MagicCaster* a_magicCaster, void* a_unk1, RE::Projectile* a_projectile, RE::TESObjectREFR* a_target, float a_unk2, float a_unk3 )
			{
				auto attacker = a_magicCaster->GetCaster();

				// This function is only called when there is at least one hostile effect so we don't need additional check
				if( attacker && a_target &&
					attacker->Is( RE::FormType::ActorCharacter ) &&
					a_target->Is( RE::FormType::ActorCharacter ) &&
					!IsTargetVaild( attacker, *a_target ) )
					return;
				
				func( a_magicCaster, a_unk1, a_projectile, a_target, a_unk2, a_unk3 );
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct MagicExplosionHitHook
		{
			// This struct is not available in CommonLibSSE
			struct ExplosionHitData
			{
				RE::MagicCaster*	caster;				// 00
				uint64_t			unk08;				// 08
				RE::Effect*			applyEffect;		// 10
				RE::Effect*			mainEffect;			// 18
				RE::MagicItem*		spell;				// 20
				uint64_t			unk28;				// 28
				RE::TESObjectREFR*	target;				// 30
				float				magnitudeOverride;	// 38
				uint32_t			unk3C;				// 3C
				RE::NiPoint3*		location;			// 40
				float				area;				// 48
				uint64_t			unk50;				// 50
				uint32_t			unk58;				// 58
				bool				unk5C;				// 5C
				bool				unk5D;				// 5D
				bool				unk5E;				// 5E
				bool				unk5F;				// 5F
			};

			static uint32_t thunk( ExplosionHitData* a_hitData, RE::TESObjectREFR* a_target )
			{
				// Cancel all effects if at least one of effect is hostile and target is friendly
				if( a_hitData->spell->hostileCount > 0 )
				{
					auto attacker = a_hitData->caster->GetCaster();
					if( attacker && a_target &&
						attacker->Is( RE::FormType::ActorCharacter ) &&
						a_target->Is( RE::FormType::ActorCharacter ) &&
						!IsTargetVaild( attacker, *a_target ) )
						return 1;
				}

				return func( a_hitData, a_target );
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct ProjectileExplosionHitHook
		{
			static uint64_t thunk( RE::Explosion* a_explosion, RE::TESObjectREFR* a_target, uint32_t a_unk1, uint32_t a_unk2 )
			{
				// Cancel explosion hit handler if friendly fire is off
				auto actorCause = a_explosion->GetActorCause();
				if( actorCause && actorCause->actor )
				{
					auto attacker = actorCause->actor.get();
					if( attacker && a_target &&
						attacker->Is( RE::FormType::ActorCharacter ) &&
						a_target->Is( RE::FormType::ActorCharacter ) &&
						!IsTargetVaild( attacker.get(), *a_target ) )
						return 0;
				}

				return func( a_explosion, a_target, a_unk1, a_unk2 );
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		// Magic hits include both from projectile and explosion
		static void InstallMagicHitHook()
		{
#ifdef SKYRIM_SUPPORT_AE
			// 44206+0x218
			// SkyrimSE.exe+0x780F50+0x218
			stl::write_thunk_call<MagicProjectileHitHook>( REL::ID( 44206 ).address() + 0x218 );

			// SkyrimSE.exe+056826A -> 34410+0xB9A
			stl::write_thunk_call<MagicExplosionHitHook>( REL::ID( 34410 ).address() + 0xB9A );

			// SkyrimSE.exe+0765B4B -> 43847+0x18B
			stl::write_thunk_call<ProjectileExplosionHitHook>( REL::ID( 43847 ).address() + 0x18B );
#else
			// TODO: Find SE offset
#endif
		}

		struct MagicApplyHook // For debug only
		{
			static uint32_t thunk( RE::MagicTarget* a_this, RE::MagicTarget::CreationData* a_creationData )
			{
				REL::Relocation<decltype(thunk)> reloc( REL::ID( 38786 ) );
				return reloc( a_this, a_creationData );
			}
			static inline REL::Relocation<decltype(thunk)> func;
			static inline size_t size = 1;
		};

		static void InstallMagicApplyHook()
		{
			// 38786+0x0
			// SkyrimSE.exe+0x659D30+0x0
			stl::write_vfunc<RE::Character, 4, MagicApplyHook>();
			stl::write_vfunc<RE::PlayerCharacter, 4, MagicApplyHook>();
		}

		static void InstallInputSink();

		static inline std::uint32_t toggleKey;
		static inline bool protectNeutralActor;

	private:
		// RCX = Aggressor, RDX = Victim, R8 = ???, R9 = ???, XMM0 = ???
		static void MeleeFunction(RE::Character* a_victim, RE::Actor* a_aggressor, std::int64_t a3, char a4, float a5);
		static void SweepFunction(RE::Character* a_victim, RE::Actor* a_aggressor, std::int64_t a3, char a4, float a5);
		static void ArrowFunction(RE::Character* a_victim, RE::Actor* a_aggressor, std::int64_t a3, char a4, float a5);

		static bool IsTargetVaild(RE::Actor* a_this, RE::TESObjectREFR& a_target);

		static inline REL::Relocation<decltype(MeleeFunction)> _MeleeFunction;
		static inline REL::Relocation<decltype(SweepFunction)> _SweepFunction;
		static inline REL::Relocation<decltype(ArrowFunction)> _ArrowFunction;

	};

}