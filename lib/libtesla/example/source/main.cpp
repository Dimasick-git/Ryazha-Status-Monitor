#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header


class GuiSecondary : public tsl::Gui {
public:
    GuiSecondary() {}

    virtual tsl::elm::Element* createUI() override {
        auto *rootFrame = new tsl::elm::OverlayFrame("Tesla Example", "v1.3.2 - Secondary Gui");

        rootFrame->setContent(new tsl::elm::DebugRectangle(0x838F));

        return rootFrame;
    }
};

class GuiTest : public tsl::Gui {
public:
    TimeLocationName out;
    GuiTest(u8 arg1, u8 arg2, bool arg3) {
        tsl::hlp::requestForeground(false);
        out.name[0] = 0;
    }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("Tesla Example", "v1.3.2");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        // Custom Drawer, a element that gives direct access to the renderer
        auto it = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(out.name, false, x, y + 70, 20, 0xFFFF);
        });
        list->addItem(it, 100);


        // Add the list to the frame for it to be drawn
        frame->setContent(list);

        // Return the frame to have it become the top level element of this Gui
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override {
        setsysGetDeviceTimeZoneLocationName(&out);
    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & KEY_A) {
            tsl::hlp::requestForeground(false);
        }
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        return false;   // Return true here to signal the inputs have been consumed
    }
};

class OverlayTest : public tsl::Overlay {
public:
                                             // libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
    virtual void initServices() override {}  // Called at the start to initialize all services necessary for this Overlay
    virtual void exitServices() override {}  // Called at the end to clean up all services previously initialized

    virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<GuiTest>(1, 2, true);  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
    }
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}
