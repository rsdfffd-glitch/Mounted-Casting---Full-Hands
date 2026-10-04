#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <chrono>

namespace
{
    using Clock = std::chrono::steady_clock;

    bool g_injecting = false;
    bool g_primaryDown = false;
    bool g_secondaryDown = false;
    bool g_dualDown = false;

    Clock::time_point g_primaryStart{};
    Clock::time_point g_secondaryStart{};
    Clock::time_point g_dualStart{};

    bool IsSpellEquipped(RE::PlayerCharacter* player, bool leftHand)
    {
        if (!player) {
            return false;
        }

        auto* form = player->GetEquippedObject(leftHand);
        return form && form->GetSavedFormType() == RE::FormType::Spell;
    }

    float HeldSeconds(const Clock::time_point& start)
    {
        const auto elapsed = std::chrono::duration<float>(Clock::now() - start).count();
        return elapsed < 0.01f ? 0.01f : elapsed;
    }

    void SendButton(const RE::BSFixedString& userEvent, std::uint32_t idCode, bool down, float heldSecs = 0.0f)
    {
        auto* input = RE::BSInputDeviceManager::GetSingleton();
        if (!input) {
            return;
        }

        auto* event = RE::ButtonEvent::Create(
            RE::INPUT_DEVICE::kMouse,
            userEvent,
            idCode,
            down ? 1.0f : 0.0f,
            heldSecs);

        if (!event) {
            return;
        }

        RE::InputEvent* eventPtr = event;
        g_injecting = true;
        input->SendEvent(&eventPtr);
        g_injecting = false;
    }

    void ResetState()
    {
        g_primaryDown = false;
        g_secondaryDown = false;
        g_dualDown = false;
    }

    class MountedCastingInput final : public RE::BSTEventSink<RE::InputEvent*>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            RE::InputEvent* const* events,
            RE::BSTEventSource<RE::InputEvent*>*) override
        {
            if (!events || g_injecting) {
                return RE::BSEventNotifyControl::kContinue;
            }

            auto* player = RE::PlayerCharacter::GetSingleton();
            auto* userEvents = RE::UserEvents::GetSingleton();
            if (!player || !userEvents) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (!player->IsOnMount()) {
                ResetState();
                return RE::BSEventNotifyControl::kContinue;
            }

            for (auto* event = *events; event; event = event->next) {
                if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
                    continue;
                }

                auto* button = event->AsButtonEvent();
                if (!button) {
                    continue;
                }

                const auto& userEvent = button->QUserEvent();
                const auto idCode = button->GetIDCode();

                // Prefer the physical mouse buttons so this still works when
                // mounted spell combat has no normal attack user-event mapping.
                const bool primary =
                    (button->device == RE::INPUT_DEVICE::kMouse &&
                     idCode == static_cast<std::uint32_t>(RE::BSWin32MouseDevice::Key::kLeftButton)) ||
                    userEvent == userEvents->leftAttack;

                const bool secondary =
                    (button->device == RE::INPUT_DEVICE::kMouse &&
                     idCode == static_cast<std::uint32_t>(RE::BSWin32MouseDevice::Key::kRightButton)) ||
                    userEvent == userEvents->rightAttack;

                if (!primary && !secondary) {
                    continue;
                }

                if (button->IsDown()) {
                    if (primary && !g_primaryDown) {
                        g_primaryDown = true;
                        g_primaryStart = Clock::now();

                        if (IsSpellEquipped(player, false)) {
                            SendButton(
                                userEvents->leftAttack,
                                static_cast<std::uint32_t>(RE::BSWin32MouseDevice::Key::kLeftButton),
                                true);
                        }
                    }

                    if (secondary && !g_secondaryDown) {
                        g_secondaryDown = true;
                        g_secondaryStart = Clock::now();

                        if (IsSpellEquipped(player, true)) {
                            SendButton(
                                userEvents->rightAttack,
                                static_cast<std::uint32_t>(RE::BSWin32MouseDevice::Key::kRightButton),
                                true);
                        }
                    }

                    if (g_primaryDown && g_secondaryDown && !g_dualDown &&
                        IsSpellEquipped(player, false) && IsSpellEquipped(player, true)) {
                        g_dualDown = true;
                        g_dualStart = Clock::now();

                        SendButton(userEvents->dualAttack, 0, true);
                    }
                } else if (button->IsUp()) {
                    // Match DualCastHotkey's charge/release behaviour: releases
                    // carry the real held duration so concentration/charged
                    // spells are handled by Skyrim instead of being force-cast.
                    if (primary && g_primaryDown) {
                        if (IsSpellEquipped(player, false)) {
                            SendButton(
                                userEvents->leftAttack,
                                static_cast<std::uint32_t>(RE::BSWin32MouseDevice::Key::kLeftButton),
                                false,
                                HeldSeconds(g_primaryStart));
                        }
                        g_primaryDown = false;
                    }

                    if (secondary && g_secondaryDown) {
                        if (IsSpellEquipped(player, true)) {
                            SendButton(
                                userEvents->rightAttack,
                                static_cast<std::uint32_t>(RE::BSWin32MouseDevice::Key::kRightButton),
                                false,
                                HeldSeconds(g_secondaryStart));
                        }
                        g_secondaryDown = false;
                    }

                    if (g_dualDown && (!g_primaryDown || !g_secondaryDown)) {
                        SendButton(userEvents->dualAttack, 0, false, HeldSeconds(g_dualStart));
                        g_dualDown = false;
                    }
                }
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    MountedCastingInput g_input;

    void OnSKSEMessage(SKSE::MessagingInterface::Message* message)
    {
        if (message && message->type == SKSE::MessagingInterface::kInputLoaded) {
            if (auto* input = RE::BSInputDeviceManager::GetSingleton()) {
                input->AddEventSink(&g_input);
            }
        }
    }
}

extern "C" __declspec(dllexport) bool SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);

    if (auto* messaging = SKSE::GetMessagingInterface()) {
        messaging->RegisterListener(OnSKSEMessage);
    }

    return true;
}
