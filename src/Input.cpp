#include "Input.h"
#include "Game.h"
#include "Settings.h"

namespace Input
{
	namespace
	{
		bool IsMoveButton(const RE::ButtonEvent* a_event)
		{
			return a_event && a_event->GetDevice() == RE::INPUT_DEVICE::kMouse && a_event->GetIDCode() == Settings::MoveButton;
		}

		// Runtime 1.7.99 added two functions to the front of every input handler's table,
		// which pushes the ones hooked here two slots down.
		std::size_t HandlerSlot(std::size_t a_slot)
		{
			return REL::VersionShift(a_slot, RE::PlayerInputHandler::kAE1799AddedVFuncCount, SKSE::RUNTIME_SSE_1_7_99);
		}

		// The move button is normally the attack button. Clicking the ground must not swing.
		struct AttackButtonHook
		{
			static void thunk(RE::AttackBlockHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
			{
				if (IsMoveButton(a_event) && Game::OnPrimaryButton(a_event)) {
					return;
				}
				func(a_this, a_event, a_data);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		// While the cursor is live the mouse moves it, so the camera must not turn.
		struct LookMouseHook
		{
			static void thunk(RE::LookHandler* a_this, RE::MouseMoveEvent* a_event, RE::PlayerControlsData* a_data)
			{
				if (Game::BlockLook()) {
					return;
				}
				func(a_this, a_event, a_data);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		// The third-person camera's own mouse handling (free rotation around the player).
		struct CameraMouseHook
		{
			static void thunk(RE::PlayerInputHandler* a_this, RE::MouseMoveEvent* a_event, RE::PlayerControlsData* a_data)
			{
				if (Game::BlockLook()) {
					return;
				}
				func(a_this, a_event, a_data);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		class Sink : public RE::BSTEventSink<RE::InputEvent*>
		{
		public:
			static Sink* Get()
			{
				static Sink s_sink;
				return &s_sink;
			}

			RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override
			{
				if (!a_event) {
					return RE::BSEventNotifyControl::kContinue;
				}
				for (auto* e = *a_event; e; e = e->next) {
					switch (e->GetEventType()) {
					case RE::INPUT_EVENT_TYPE::kMouseMove:
						if (auto* move = e->AsMouseMoveEvent()) {
							MoveCursor(move);
						}
						break;
					case RE::INPUT_EVENT_TYPE::kButton:
						if (auto* button = e->AsButtonEvent()) {
							if (e->GetDevice() == RE::INPUT_DEVICE::kKeyboard) {
								OnKey(button);
							} else if (e->GetDevice() == RE::INPUT_DEVICE::kMouse) {
								OnMouseButton(button);
							}
						}
						break;
					default:
						break;
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}

		private:
			static void MoveCursor(const RE::MouseMoveEvent* a_move)
			{
				if (!State::gameplay.load() || State::rotateHeld.load()) {
					return;
				}
				const float w = State::screenW.load();
				const float h = State::screenH.load();
				float       x = State::cursorX.load();
				float       y = State::cursorY.load();
				if (x < 0.0f || y < 0.0f) {
					x = w * 0.5f;
					y = h * 0.5f;
				}
				// Clamped, so the cursor can never leave the screen.
				x = std::clamp(x + static_cast<float>(a_move->mouseInputX) * Settings::Sensitivity, 0.0f, std::max(w - 1.0f, 0.0f));
				y = std::clamp(y + static_cast<float>(a_move->mouseInputY) * Settings::Sensitivity, 0.0f, std::max(h - 1.0f, 0.0f));
				State::cursorX.store(x);
				State::cursorY.store(y);
			}

			void OnKey(const RE::ButtonEvent* a_button)
			{
				const auto code = a_button->GetIDCode();
				const bool pressed = a_button->IsPressed();

				if (code != 0) {
					if (code == Settings::RotateKey) {
						State::rotateHeld.store(pressed);
					}
					if (code == Settings::AttackInPlaceKey) {
						State::attackInPlaceHeld.store(pressed);
					}
					if (code == Settings::LabelKey) {
						State::labelKeyHeld.store(pressed);
					}
					if (code == Settings::ToggleKey && a_button->IsDown()) {
						auto* ui = RE::UI::GetSingleton();
						if (ui && !ui->GameIsPaused()) {
							Game::Toggle();
						}
					}
				}

				// WASD cancels a click-move and takes over until every movement key is released.
				// Releases are matched by key code: a key let go while a menu is open no longer
				// carries its movement name.
				if (pressed) {
					if (auto* userEvents = RE::UserEvents::GetSingleton()) {
						const auto& name = a_button->QUserEvent();
						int         index = -1;
						if (name == userEvents->forward) {
							index = 0;
						} else if (name == userEvents->back) {
							index = 1;
						} else if (name == userEvents->strafeLeft) {
							index = 2;
						} else if (name == userEvents->strafeRight) {
							index = 3;
						}
						if (index >= 0) {
							_moveKeys[index] = code;
						}
					}
				} else {
					for (auto& key : _moveKeys) {
						if (key == code) {
							key = kNoKey;
						}
					}
				}
				int held = 0;
				for (const auto key : _moveKeys) {
					held += key != kNoKey ? 1 : 0;
				}
				State::moveKeysHeld.store(held);
			}

			static void OnMouseButton(RE::ButtonEvent* a_button)
			{
				const auto code = a_button->GetIDCode();
				if (code == Settings::PickupButton && a_button->IsDown()) {
					Game::OnPickupButton();
				}
				if (code == Settings::MoveButton) {
					// A move button bound to attack is handled in the attack hook, where the swing
					// can be stopped. Any other button has nothing to stop, so handle it here.
					auto*       userEvents = RE::UserEvents::GetSingleton();
					const auto& name = a_button->QUserEvent();
					if (!userEvents || (name != userEvents->rightAttack && name != userEvents->leftAttack)) {
						Game::OnPrimaryButton(a_button);
					}
				}
			}

			static constexpr std::uint32_t kNoKey = 0xFFFFFFFF;
			std::uint32_t                  _moveKeys[4]{ kNoKey, kNoKey, kNoKey, kNoKey };
		};
	}

	void Install()
	{
		REL::Relocation<std::uintptr_t> attack{ RE::VTABLE_AttackBlockHandler[0] };
		AttackButtonHook::func = attack.write_vfunc(HandlerSlot(0x4), AttackButtonHook::thunk);

		REL::Relocation<std::uintptr_t> look{ RE::VTABLE_LookHandler[0] };
		LookMouseHook::func = look.write_vfunc(HandlerSlot(0x3), LookMouseHook::thunk);

		REL::Relocation<std::uintptr_t> camera{ RE::VTABLE_ThirdPersonState[1] };
		CameraMouseHook::func = camera.write_vfunc(HandlerSlot(0x3), CameraMouseHook::thunk);

		logs::info("[input] hooked attack button (slot {:#x}) and mouse look (slot {:#x})", HandlerSlot(0x4), HandlerSlot(0x3));
	}

	void Register()
	{
		if (auto* input = RE::BSInputDeviceManager::GetSingleton()) {
			input->AddEventSink(static_cast<RE::BSTEventSink<RE::InputEvent*>*>(Sink::Get()));
			logs::info("[input] registered: move button {}, pickup button {}, toggle key {:#x}", Settings::MoveButton, Settings::PickupButton,
				Settings::ToggleKey);
		} else {
			logs::error("[input] no input device manager; point and click unavailable");
		}
	}
}
