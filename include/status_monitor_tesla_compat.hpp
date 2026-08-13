#pragma once

#include <tesla.hpp>

// RyazhaTune uses tsl::Color directly. Status Monitor's color editor used the
// legacy tsl::gfx::Color name, so retain that spelling without restoring the
// old renderer.
namespace tsl::gfx {
    using Color = ::tsl::Color;

    inline Color RGB888(const std::string& hexColor, const std::string& defaultHexColor) {
        return ::tsl::RGB888(hexColor, 15, defaultHexColor);
    }
}

namespace tsl::elm {
    // Status Monitor needs a list item that owns an RGBA4444 color for its
    // existing color editor. The adapter deliberately derives from the full
    // RyazhaTune ListItem, so focus, text layout and Switch 2 rendering remain
    // exactly those of the reference renderer.
    class ColorListItem final : public ListItem {
    public:
        ColorListItem(const std::string& text, u16 color, bool changeRandomlyColors = false)
            : ListItem(text), m_color(color), m_changeRandomlyColors(changeRandomlyColors) {}

        Color getColor() const noexcept { return m_color; }
        void setColor(Color color) noexcept { m_color = color; }

        void draw(gfx::Renderer* renderer) override {
            ListItem::draw(renderer);
            const s32 centerX = this->getX() + this->getWidth() - 32;
            const s32 centerY = this->getY() + this->getHeight() / 2;
            const Color border = m_changeRandomlyColors
                ? Color(0xFFFF)
                : Color(static_cast<u16>(0xF000 | ((~m_color.rgba) & 0x0FFF)));
            renderer->drawCircle(centerX, centerY, 17, true, renderer->a(border));
            renderer->drawUniformRoundedRect(centerX - 12, centerY - 12, 24, 24, renderer->a(Color(static_cast<u16>(0xF000 | (m_color.rgba & 0x0FFF)))));
        }

    private:
        Color m_color;
        bool m_changeRandomlyColors = false;
    };
}
