#include "Utils.h"

float Utilities::Round(float number, int decimalPlaces) {
    auto rounded_number =
        static_cast<float>(std::round(number * std::pow(10, decimalPlaces))) / std::pow(10, decimalPlaces);
    return rounded_number;
}

int Utilities::Round2Int(float number) {
    auto rounded_number = static_cast<int>(Round(number, 0));
    return rounded_number;
}

std::vector<std::uint64_t> Utilities::ParseHandshakeKeys(std::string_view text) {
    std::vector<std::uint64_t> keys;
    while (!text.empty()) {
        const auto comma = text.find(',');
        auto key = text.substr(0, comma);
        const auto first = key.find_first_not_of(" \t");
        if (first != std::string_view::npos) {
            key = key.substr(first, key.find_last_not_of(" \t") - first + 1);
            if (key.starts_with("0x") || key.starts_with("0X")) key.remove_prefix(2);
            std::uint64_t value{};
            const auto [end, error] = std::from_chars(key.data(), key.data() + key.size(), value, 16);
            if (error == std::errc{} && end == key.data() + key.size()) {
                if (std::ranges::find(keys, value) == keys.end()) keys.push_back(value);
            } else logger::warn("Invalid SkyPrompt handshake key: {}", key);
        }
        if (comma == std::string_view::npos) break;
        text.remove_prefix(comma + 1);
    }
    return keys;
}

std::string Utilities::DecodeTypeCode(std::uint32_t typeCode) {
    char buf[4];
    buf[3] = char(typeCode);
    buf[2] = char(typeCode >> 8);
    buf[1] = char(typeCode >> 16);
    buf[0] = char(typeCode >> 24);
    return std::string(buf, buf + 4);
}

std::filesystem::path Utilities::GetLogPath() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    return logFilePath;
}

std::vector<std::string> Utilities::ReadLogFile() {
    std::vector<std::string> logLines;

    // Open the log file
    std::ifstream file(GetLogPath().c_str());
    if (!file.is_open()) {
        // Handle error
        return logLines;
    }

    // Read and store each line from the file
    std::string line;
    while (std::getline(file, line)) {
        logLines.push_back(line);
    }

    file.close();

    return logLines;
}

void MsgBoxesNotifs::InGame::Remaining(int remaining, std::string_view item) {
    if (remaining < 0.00001)
        return;
    else if (remaining > 1)
        RE::SendHUDMessage::ShowHUDMessage(std::format("I have about {} hours left in my {}.", remaining, item).c_str());
    else if (remaining > 0.5)
        RE::SendHUDMessage::ShowHUDMessage(std::format("I have about an hour left in my {}.", item).c_str());
    else
        RE::SendHUDMessage::ShowHUDMessage(std::format("I don't have much left in my {}...", item).c_str());
}

float almost_constant_with_accelerated_cutoff(float x, float start_value, float cutoff, int power) { // Ensure x is within [0, 1]
    x = std::clamp(x, 0.0f, 1.0f);

    // Calculate the nonlinear drop-off
    float drop = std::max(0.0f, 1.0f - std::pow((x - cutoff) / (1.0f - cutoff), static_cast<float>(power)));

    // Return the function value
    return (x < cutoff) ? start_value : start_value * drop;
}
