#include "Integrations.h"
#include "Manager.h"
#include "SkyPrompt/API.hpp"

namespace {
    constexpr std::string_view lorebox_key = "LoreBox_FuelableFramework";
    constexpr SkyPromptAPI::HandshakeKey handshake_key = 0x4655454C41424C45;  // ASCII FUELABLE
    constexpr std::size_t advance_movie_slot = 0x05;

    RE::TESBoundObject* SelectedItem() {
        const auto ui = RE::UI::GetSingleton();
        const auto menu = ui ? ui->GetMenu<RE::InventoryMenu>() : nullptr;
        if (!menu) return nullptr;
        const auto list = menu->GetRuntimeData().itemList;
        const auto item = list ? list->GetSelectedItem() : nullptr;
        return item && item->data.objDesc ? item->data.objDesc->object : nullptr;
    }

    void Replace(std::string& text, std::string_view key, std::string_view value) {
        std::size_t position = 0;
        while ((position = text.find(key, position)) != std::string::npos) {
            text.replace(position, key.size(), value);
            position += value.size();
        }
    }

    std::string FormatText(std::string text, LightSource& source, float remaining, float total_remaining = 0) {
        Replace(text, "{remaining}", Utilities::FormatDuration(remaining));
        Replace(text, "{total_remaining}", Utilities::FormatDuration(total_remaining));
        Replace(text, "{requirements}", source.GetFuelName());
        Replace(text, "{item}", source.GetName());
        Replace(text, "{duration}", Utilities::FormatDuration(source.duration));
        return text;
    }

    std::wstring Widen(std::string_view text) {
        if (text.empty()) return {};
        const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                             static_cast<int>(text.size()), nullptr, 0);
        std::wstring result(size, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                            result.data(), size);
        return result;
    }

    class IntegrationManager final : public SkyPromptAPI::PromptSink,
                                     public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static IntegrationManager& GetSingleton() {
            static IntegrationManager instance;
            return instance;
        }

        void Install(LightSourceManager* manager) {
            manager_ = manager;
            lorebox_available_ = GetModuleHandleW(L"LoreBox.dll") &&
                                 GetModuleHandleW(L"DynamicTranslationFrameworkSE.dll");
            if (lorebox_available_) {
                auto* keyword = RE::BGSKeyword::CreateKeyword(std::string(lorebox_key));
                if (keyword) {
                    for (auto& source : manager_->sources) {
                        if (!source.options->lorebox_enabled) continue;
                        if (auto* keywords = source.GetBoundObject()->As<RE::BGSKeywordForm>()) {
                            if (!keywords->HasKeyword(keyword)) {
                                static_cast<void>(keywords->AddKeyword(keyword));
                            }
                        } else {
                            logger::warn("LoreBox cannot attach a keyword to {} ({:08X})", source.GetName(), source.formid);
                        }
                    }
                }
            }

            if (!GetModuleHandleW(L"SkyPrompt.dll")) return;
            const auto uses_prompts = std::ranges::any_of(manager_->sources, [](const auto& source) {
                return source.options->skyprompt_enabled;
            });
            if (!uses_prompts) return;
            client_ = SkyPromptAPI::RequestClientID();
            if (!client_) return;
            if (!SkyPromptAPI::RequestTheme(client_, "FuelableFramework")) {
                logger::warn("SkyPrompt could not load the FuelableFramework theme");
            }
            std::set<SkyPromptAPI::HandshakeKey> requested;
            for (const auto& source : manager_->sources) {
                if (!source.options->skyprompt_enabled) continue;
                for (const auto peer : source.options->compatible_keys) {
                    if (requested.insert(peer).second && !SkyPromptAPI::RequestHandshake(client_, handshake_key, peer)) {
                        logger::warn("SkyPrompt declined handshake request for {:016X}", peer);
                    }
                }
            }
            RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(this);
            REL::Relocation<std::uintptr_t> table{RE::VTABLE_InventoryMenu[0]};
            advance_movie_ = table.write_vfunc(advance_movie_slot, AdvanceMovie);
        }

        std::span<const SkyPromptAPI::Prompt> GetPrompts() const override { return prompts_; }

        void ProcessEvent(SkyPromptAPI::PromptEvent event) const override {
            if (event.type != SkyPromptAPI::PromptEventType::kAccepted) return;
            // Source definitions stay fixed after configuration loading. Prompt events arrive on SkyPrompt's draw path.
            const auto index = static_cast<std::size_t>(event.prompt.actionID);
            if (index >= manager_->sources.size()) return;
            const auto& source = manager_->sources[index];
            const auto object = source.formid;
            const auto fuel = source.fuel;
            // SkyPrompt removes completed hold actions; restore them for the next press.
            static_cast<void>(SkyPromptAPI::SendPrompt(this, client_));
            SKSE::GetTaskInterface()->AddTask([object, fuel]() {
                auto& integration = GetSingleton();
                const auto item = SelectedItem();
                if (!item || item->GetFormID() != object) return;
                auto* source = integration.manager_->FindSource(object, fuel);
                if (source && source->options->skyprompt_enabled && integration.manager_->Refuel(*source)) {
                    if (!integration.lorebox_available_ || !source->options->lorebox_enabled) return;
                    const auto menu = RE::UI::GetSingleton()->GetMenu<RE::InventoryMenu>();
                    if (menu && menu->uiMovie) {
                        static_cast<void>(menu->uiMovie->Invoke("_global.LoreBox.instance.redraw", nullptr, nullptr, 0));
                    }
                }
            });
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
            if (event && event->menuName == RE::InventoryMenu::MENU_NAME && !event->opening) {
                Hide();
                selected_ = nullptr;
            }
            return RE::BSEventNotifyControl::kContinue;
        }

        std::wstring LoreText() const {
            if (!lorebox_available_ || !manager_) return {};
            const auto item = SelectedItem();
            if (!item) return {};
            std::vector<LoreGroup> groups;
            std::size_t prompt_number = 0;
            for (std::size_t index = 0; index < manager_->sources.size(); ++index) {
                auto& source = manager_->sources[index];
                if (source.formid != item->GetFormID()) continue;
                const auto remaining = manager_->GetRemaining(&source);
                const auto number = index <= std::numeric_limits<SkyPromptAPI::ActionID>::max() &&
                                    source.options->skyprompt_enabled &&
                                    !FormatText(source.options->prompt_text, source, remaining).empty()
                    ? ++prompt_number : 0;
                if (!source.options->lorebox_enabled) continue;
                auto group = std::ranges::find(groups, source.options.get(), &LoreGroup::options);
                if (group == groups.end()) {
                    groups.push_back({source.options.get()});
                    group = std::prev(groups.end());
                }
                group->sources.push_back({&source, remaining, number});
                group->remaining += remaining;
            }
            std::string text;
            for (const auto& group : groups) {
                const auto block = FormatLoreBlock(group);
                if (block.empty()) continue;
                if (!text.empty()) text += "<br><br>";
                text += block;
            }
            return Widen(text);
        }

    private:
        struct LoreRow {
            LightSource* source;
            float remaining;
            std::size_t number;
        };

        struct LoreGroup {
            const SourceOptions* options;
            std::vector<LoreRow> sources;
            float remaining = 0;
        };

        static std::string FormatLoreBlock(const LoreGroup& group) {
            const auto& options = *group.options;
            const auto& first = group.sources.front();
            const auto title = FormatText(options.lorebox_title, *first.source, first.remaining, group.remaining);
            const auto summary = FormatText(options.lorebox_summary, *first.source, first.remaining, group.remaining);
            std::string text;
            if (!title.empty()) {
                text = std::format("<b><font color=\"#{:06X}\">{}</font></b>", options.lorebox_title_color, title);
            }
            if (!summary.empty()) {
                constexpr auto summary_font_size = 16;
                if (!text.empty()) text += "<br>";
                text += std::format("<font color=\"#{:06X}\" size=\"{}\">{}</font>",
                                    options.lorebox_text_color, summary_font_size, summary);
            }
            constexpr auto option_font_size = 14;
            for (const auto& entry : group.sources) {
                auto row = FormatText(options.lorebox_text, *entry.source, entry.remaining, group.remaining);
                if (row.empty()) continue;
                if (entry.number) row.insert(0, std::format("{}. ", entry.number));
                if (!text.empty()) text += "<br>";
                text += std::format("<font color=\"#{:06X}\" size=\"{}\">{}</font>",
                                    options.lorebox_option_color, option_font_size, row);
            }
            return text;
        }

        static void AdvanceMovie(RE::InventoryMenu* menu, float interval, std::uint32_t current_time) {
            advance_movie_(menu, interval, current_time);
            GetSingleton().Refresh();
        }

        void Hide() {
            if (submitted_) SkyPromptAPI::RemovePrompt(this, client_);
            submitted_ = false;
        }

        void Refresh() {
            const auto item = SelectedItem();
            const auto preview = RE::Inventory3DManager::GetSingleton()->tempRef;
            const RE::FormID preview_id = preview ? preview->GetFormID() : 0;
            if (item == selected_ && preview_id == preview_ref_ && !dynamic_text_) return;
            if (item != selected_) {
                indices_.clear();
                dynamic_text_ = false;
                if (item) {
                    for (std::size_t index = 0; index < manager_->sources.size(); ++index) {
                        const auto& source = manager_->sources[index];
                        if (source.formid != item->GetFormID() || !source.options->skyprompt_enabled) continue;
                        if (index > std::numeric_limits<SkyPromptAPI::ActionID>::max()) {
                            logger::error("SkyPrompt source index exceeds ActionID range: {}", index);
                            break;
                        }
                        indices_.push_back(index);
                        dynamic_text_ |= source.options->prompt_text.find("{remaining}") != std::string::npos;
                    }
                }
            }
            std::vector<std::string> text;
            for (const auto index : indices_) {
                auto& source = manager_->sources[index];
                auto label = FormatText(source.options->prompt_text, source, manager_->GetRemaining(&source));
                text.push_back(std::move(label));
            }
            if (item == selected_ && preview_id == preview_ref_ && text == texts_) return;
            Hide();
            selected_ = item;
            preview_ref_ = preview_id;
            texts_ = std::move(text);
            prompts_.clear();
            for (std::size_t index = 0; index < texts_.size(); ++index) {
                if (!texts_[index].empty()) {
                    prompts_.emplace_back(texts_[index], 0, static_cast<SkyPromptAPI::ActionID>(indices_[index]),
                                          SkyPromptAPI::PromptType::kHold, preview_id);
                }
            }
            if (!prompts_.empty()) {
                static_cast<void>(SkyPromptAPI::SendPrompt(this, client_));
                submitted_ = true;
            }
        }

        LightSourceManager* manager_ = nullptr;
        bool lorebox_available_ = false;
        SkyPromptAPI::ClientID client_ = 0;
        RE::TESBoundObject* selected_ = nullptr;
        RE::FormID preview_ref_ = 0;
        bool submitted_ = false;
        bool dynamic_text_ = false;
        std::vector<std::size_t> indices_;
        std::vector<std::string> texts_;
        std::vector<SkyPromptAPI::Prompt> prompts_;
        static inline REL::Relocation<decltype(AdvanceMovie)> advance_movie_;
    };
}

void Integrations::Install(LightSourceManager* manager) {
    IntegrationManager::GetSingleton().Install(manager);
}

const wchar_t* OnDynamicTranslationRequest(std::string_view key) {
    thread_local std::wstring text;
    text = key == lorebox_key ? IntegrationManager::GetSingleton().LoreText() : L"";
    return text.c_str();
}
