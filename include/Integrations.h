#pragma once

class LightSourceManager;

namespace Integrations {
    void Install(LightSourceManager* manager);
}

extern "C" __declspec(dllexport) const wchar_t* OnDynamicTranslationRequest(std::string_view key);
