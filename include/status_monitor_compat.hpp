#pragma once

#include <tesla.hpp>
#include <ini_funcs.hpp>
#include <string_funcs.hpp>

// Legacy Status Monitor called common libtesla helpers from the global namespace.
// Canonical libryazhahand exposes them through ult::. Keep this adapter side-effect
// free: it must not change overlay ownership, frame size, or input lifecycle.
inline std::string trim(std::string value) {
    ult::trim(value);
    return value;
}

using ult::getParsedDataFromIniFile;
using ult::parseIni;
using ult::parseValueFromIniSection;
using ult::removeIniSection;
using ult::removeQuotes;
using ult::setIniFile;
using ult::setIniFileKey;
using ult::setIniFileValue;

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
            const Color border = m_random ? Color(0xFFFF) : Color(static_cast<u16>(0xF000 | ((~m_color.rgba) & 0x0FFF)));
            renderer->drawCircle(x, y, 17, true, renderer->a(border));
            renderer->drawUniformRoundedRect(x - 12, y - 12, 24, 24, renderer->a(Color(static_cast<u16>(0xF000 | (m_color.rgba & 0x0FFF))));
        }
    private:
        Color m_color;
        bool m_random = false;
    };
}
