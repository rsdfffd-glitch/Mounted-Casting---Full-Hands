#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace
{
    constexpr auto kWantRight = "bWantCastRight";
    constexpr auto kWantLeft  = "bWantCastLeft";

    bool g_ownsRight = false;
    bool g_ownsLeft = false;

    bool IsSpellEquipped(RE::PlayerCharacter* player, bool leftHand)
    {
        if (!player) {
            return false;
        }

        auto* form = player->GetEquippedObject(leftHand);
        return form && form->GetFormType() == RE::FormType::Spell;
    }

    void SetWanted(RE::PlayerCharacter* player, const char* variable, bool value)
    {
        if (player) {
            player->SetGraphVariableBool(RE::BSFixedString(variable), value);
        }
    }

    void ClearOwnedState(RE::PlayerCharacter* player)
    {
        if (!player) {
            g_ownsRight = false;
            g_ownsLeft = false;
            return;
        }

        if (g_ownsRight) {
            SetWanted(player, kWantRight, false);
            g_ownsRight = false;
        }
        if (g_ownsLeft) {
            SetWanted(player, kWantLeft, false);
            g_ownsLeft = false;
        }
    }

    class MountedCastingInput final : public RE::BSTEventSink<RE::InputEvent*>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            RE::InputEvent* const* events,
            RE::BSTEventSource<RE::InputEvent*>*) override
        {
            if (!events) {
                return RE::BSEventNotifyControl::kContinue;
            }

            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                return RE::BSEventNotifyControl::kContinue;
            }

            // Never touch normal on-foot casting.
            if (!player->IsOnMount()) {
                ClearOwnedState(player);
                return RE::BSEventNotifyControl::kContinue;
            }

            auto* userEvents = RE::UserEvents::GetSingleton();
            if (!userEvents) {
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

                // Skyrim names the primary/LMB action "leftAttack", but it drives
                // the RIGHT hand. The secondary/RMB action drives the LEFT hand.
                if (userEvent == userEvents->leftAttack) {
                    if (button->IsDown()) {
                        if (IsSpellEquipped(player, false)) {
                            SetWanted(player, kWantRight, true);
                            g_ownsRight = true;
                        }
                    } else if (button->IsUp()) {
                        if (g_ownsRight) {
                            SetWanted(player, kWantRight, false);
                            g_ownsRight = false;
                        }
                    }
                } else if (userEvent == userEvents->rightAttack) {
                    if (button->IsDown()) {
                        if (IsSpellEquipped(player, true)) {
                            SetWanted(player, kWantLeft, true);
                            g_ownsLeft = true;
                        }
                    } else if (button->IsUp()) {
                        if (g_ownsLeft) {
                            SetWanted(player, kWantLeft, false);
                            g_ownsLeft = false;
                        }
                    }
                } else if (userEvent == userEvents->dualAttack) {
                    // Some input paths/controllers can surface Skyrim's synthetic
                    // "Dual Attack" user event directly. Supporting it costs nothing
                    // and keeps the bridge compatible with remapped controls.
                    if (button->IsDown()) {
                        if (IsSpellEquipped(player, false) && IsSpellEquipped(player, true)) {
                            SetWanted(player, kWantRight, true);
                            SetWanted(player, kWantLeft, true);
                            g_ownsRight = true;
                            g_ownsLeft = true;
                        }
                    } else if (button->IsUp()) {
                        if (g_ownsRight) {
                            SetWanted(player, kWantRight, false);
                            g_ownsRight = false;
                        }
                        if (g_ownsLeft) {
                            SetWanted(player, kWantLeft, false);
                            g_ownsLeft = false;
                        }
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
                SKSE::log::info("Mounted casting input bridge active");
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
