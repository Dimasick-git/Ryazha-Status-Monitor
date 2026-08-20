#pragma once

#include <tesla.hpp>
#include <ini_funcs.hpp>
#include <string_funcs.hpp>

#include <charconv>
#include <cctype>
#include <cstdint>
#include <string>
#include <system_error>
#include <unordered_map>

// Legacy Status Monitor called common libtesla helpers from the global namespace.
// Canonical libryazhahand exposes them through ult::. These adapters deliberately
// do not change renderer ownership, frame sizing, or input lifecycle.
inline std::string trim(std::string value) {
    ult::trim(value);
    return value;
}

using ult::getParsedDataFromIniFile;
using ult::parseValueFromIniSection;
using ult::removeIniSection;
using ult::setIniFile;
using ult::setIniFileKey;
using ult::setIniFileValue;

// Canonical INI utilities use ordered maps, while legacy Status Monitor stores
// its parsed configuration in unordered maps. Convert at this boundary so the
// old configuration model remains deterministic and source-compatible.
inline std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
parseIni(const std::string& contents) {
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> result;
    for (const auto& [section, values] : ult::parseIni(contents)) {
        result.emplace(section, std::unordered_map<std::string, std::string>(values.begin(), values.end()));
    }
    return result;
}

inline std::string removeQuotes(std::string value) {
    ult::removeQuotes(value);
    return value;
}

// Canonical OverlayFrame owns the visible footer. This value only preserves
// source compatibility for old editor views until each footer is migrated.
inline std::string defaultButtonView;

inline bool isNumeric(const std::string& value, int64_t* out) {
    if (out == nullptr || value.empty()) return false;

    const char* begin = value.data();
    const char* end = begin + value.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(*begin))) ++begin;
    while (begin < end && std::isspace(static_cast<unsigned char>(end[-1]))) --end;
    if (begin == end) return false;

    int64_t parsed = 0;
    const auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec != std::errc{} || ptr != end) return false;
    *out = parsed;
    return true;
}

inline bool isValid4444HexColor(const std::string& value) {
    if (value.empty()) return false;
    for (const unsigned char c : value) {
        if (!std::isxdigit(c)) return false;
    }
    return true;
}

namespace tsl::gfx {
    using Color = ::tsl::Color;

    inline Color RGB888(const std::string& hexColor, const std::string& fallback) {
        return ::tsl::RGB888(hexColor, 15, fallback);
    }
}

namespace tsl::elm {
    class ColorListItem final : public ListItem {
    public:
        ColorListItem(const std::string& text, u16 color, bool random = false)
            : ListItem(text), m_color(color), m_random(random) {}

        Color getColor() const noexcept { return m_color; }
        void setColor(Color color) noexcept { m_color = color; }

        void draw(gfx::Renderer* renderer) override {
            ListItem::draw(renderer);
            const s32 x = getX() + getWidth() - 32;
            const s32 y = getY() + getHeight() / 2;
            const Color border = m_random
                ? Color(0xFFFF)
                : Color(static_cast<u16>(0xF000 | ((~m_color.rgba) & 0x0FFF)));
            renderer->drawCircle(x, y, 17, true, renderer->a(border));
            renderer->drawUniformRoundedRect(
                x - 12, y - 12, 24, 24,
                renderer->a(Color(static_cast<u16>(0xF000 | (m_color.rgba & 0x0FFF))))
            );
        }

    private:
        Color m_color;
        bool m_random = false;
    };
}
