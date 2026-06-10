#pragma once
// Using this dummy because if we go directly to RenderingPipeline, we can't get out of focus. Why? Trivago.
class RenderingPipelineDummy : public tsl::Gui {
public:
	std::string filepath;
	char* buffer = 0;
	RenderingPipelineDummy(std::string path) {
		filepath = path;
	}

	virtual tsl::elm::Element* createUI() override {
		rootFrame = new tsl::elm::OverlayFrame("", "");
		return rootFrame;
	}

	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		tsl::changeTo<RenderingPipeline>(filepath, true);
		return true;
	}
};