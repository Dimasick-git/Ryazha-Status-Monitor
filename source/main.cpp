#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include "Utils.hpp"
#include <malloc.h>
#include <set>
#include "Extensions/smse.hpp"
#include <cstdlib>

std::list<ServiceExtensions> serviceExt;

tsl::elm::OverlayFrame* rootFrame = nullptr;

std::string file_to_load = "";
HidSixAxisSensorHandle sixaxisHandles[Controller_Max];

#include "rendering_pipeline.hpp"
#include "DataTypes.hpp"
#include "Configuration/EditConfigInt.hpp"
#include "Configuration/ConfigurationMainMenu.hpp"
#include "Configuration/Configuration.hpp"
#include "RenderingPipelineDummy.hpp"
#ifdef DEBUG
#include "MemoryDebug.hpp"
#endif

static void ApplyRyazhaTheme();

extern "C" {
	//This is done to save some space as they have no practical use in our case
	void* __real___cxa_throw();
	void* __real___cxa_rethrow();
	void* __real___cxa_allocate_exception();
	void* __real___cxa_free_exception();
	void* __real___cxa_begin_catch();
	void* __real___cxa_end_catch();
	void* __real___cxa_call_unexpected();
	void* __real___cxa_call_terminate();
	void* __real__ZSt19__throw_logic_errorPKc();
	void* __real__ZSt20__throw_length_errorPKc();
	void* __real__ZNSt11logic_errorC2EPKc();
	void* __real__Unwind_Resume();
	void* __real___gxx_personality_v0();
	void __wrap___cxa_throw() {__builtin_unreachable();}
	void __wrap___cxa_rethrow() {__builtin_unreachable();}
	void __wrap___cxa_allocate_exception() {__builtin_unreachable();}
	void __wrap___cxa_free_exception() {__builtin_unreachable();}
	void __wrap___cxa_begin_catch() {__builtin_unreachable();}
	void __wrap___cxa_end_catch() {__builtin_unreachable();}
	void __wrap___cxa_call_unexpected() {__builtin_unreachable();}
	void __wrap___cxa_call_terminate() {__builtin_unreachable();}
	void __wrap__ZSt19__throw_logic_errorPKc() {__builtin_unreachable();}
	void __wrap__ZSt20__throw_length_errorPKc() {__builtin_unreachable();}
	void __wrap__ZNSt11logic_errorC2EPKc() {__builtin_unreachable();}
	void __wrap__Unwind_Resume() {__builtin_unreachable();}
	void __wrap___gxx_personality_v0() {__builtin_unreachable();}
}

class MainMenu : public tsl::Gui {
public:
    
    const std::string root_path = "sdmc:/config/status-monitor/modes/";
    std::string standard_path = root_path;
	std::vector<Designs> filesChecked;
	std::string formattedKeyCombo;
	std::string m_folderName;
	std::string footerBackup;
	bool isMainMenu = false;

	// Ryazha quick-launch combos: a mode section in config.ini may define
	// quick_combo=ZL+ZR+DUP — holding it in any menu jumps straight into
	// that SMD file.
	struct QuickCombo {
		uint64_t mask;
		std::string rel;
	};
	std::vector<QuickCombo> quickCombos;

	void collectQuickCombos() {
		for (const auto& [section, values] : config) {
			if (section.size() < 4 || section.compare(section.size() - 4, 4, ".smd") != 0)
				continue;
			auto it = values.find("quick_combo");
			if (it == values.end() || it->second.empty())
				continue;
			std::string combo = it->second;
			removeSpaces(combo);
			convertToUpper(combo);
			uint64_t mask = MapButtons(combo);
			if (mask != 0)
				quickCombos.push_back({mask, section});
		}
	}

	bool handleQuickCombos(uint64_t keysDown, uint64_t keysHeld) {
		for (const auto& qc : quickCombos) {
			if ((keysHeld & qc.mask) != qc.mask || !(keysDown & qc.mask))
				continue;
			struct stat st;
			std::string full_path = root_path + qc.rel;
			if (stat(full_path.c_str(), &st) != 0)
				continue;
			std::string args = "--file " + qc.rel + " --submenu";
			tsl::setNextOverlay(filepath, args);
			tsl::Overlay::get()->close();
			return true;
		}
		return false;
	}

	bool FindConfigs(const char* data, size_t size) {
		size_t lineStart = 0;
		for (size_t i = 0; i < size; ++i) {
			if (data[i] != '\n') continue;

			// Slice the current line [lineStart, i), peeling off a trailing \r.
			size_t end = i;
			while (end > lineStart && data[end - 1] == '\r') --end;
			std::string rawLine(data + lineStart, end - lineStart);
			lineStart = i + 1;

			std::string trimmedRaw = trim(rawLine);

			// 2. Standard configuration line processing
			std::string line = StripLineComment(rawLine);
			line = trim(line);
			if (line.empty()) continue;

			if (line == "Start:" || line == "Start: ") break;

			size_t sep = std::string::npos;
			{
				int depth = 0; bool inStr = false;
				for (size_t j = 0; j < line.size(); ++j) {
					char c = line[j];
					if (inStr) {
						if (c == '\\' && j + 1 < line.size()) { ++j; continue; }
						if (c == '"') inStr = false;
						continue;
					}
					if (c == '"') { inStr = true; continue; }
					if (c == '{') ++depth;
					else if (c == '}') --depth;
					else if (depth == 0 && c == '=') { sep = j; break; }
				}
			}
			if (sep == std::string::npos) continue;

			std::string key  = trim(line.substr(0, sep));
			std::string rest = trim(line.substr(sep + 1));

			if (key.starts_with("User_") == true) return true;
		}

		return false;
	}

	bool itHasCustomExitCombo(const char* data, size_t size) {
		size_t lineStart = 0;
		for (size_t i = 0; i < size; ++i) {
			if (data[i] != '\n') continue;

			// Slice the current line [lineStart, i), peeling off a trailing \r.
			size_t end = i;
			while (end > lineStart && data[end - 1] == '\r') --end;
			std::string rawLine(data + lineStart, end - lineStart);
			lineStart = i + 1;

			std::string trimmedRaw = trim(rawLine);

			// 2. Standard configuration line processing
			std::string line = StripLineComment(rawLine);
			line = trim(line);
			if (line.empty()) continue;

			if (line == "Start:" || line == "Start: ") break;

			size_t sep = std::string::npos;
			{
				int depth = 0; bool inStr = false;
				for (size_t j = 0; j < line.size(); ++j) {
					char c = line[j];
					if (inStr) {
						if (c == '\\' && j + 1 < line.size()) { ++j; continue; }
						if (c == '"') inStr = false;
						continue;
					}
					if (c == '"') { inStr = true; continue; }
					if (c == '{') ++depth;
					else if (c == '}') --depth;
					else if (depth == 0 && c == '=') { sep = j; break; }
				}
			}
			if (sep == std::string::npos) continue;

			std::string key  = trim(line.substr(0, sep));
			std::string rest = trim(line.substr(sep + 1));

			if (key.compare("UseCustomExitCombo") == 0 && rest.compare("true") == 0) return true;
		}

		return false;
	}

    MainMenu(std::string rel_path, std::string folderName = "") {
		footerBackup = defaultButtonView;
		formattedKeyCombo = keyCombo;
		formatButtonCombination(formattedKeyCombo);
        if (!rel_path.empty()) {
            standard_path = rel_path;
        }
        find_smd_files(standard_path, filesChecked);
		collectQuickCombos();
		if (folderName.length() != 0) {
			m_folderName = folderName;
			defaultButtonView = locale["Footer"];
		}
		else {
			isMainMenu = true;
			defaultButtonView = locale["MainMenuFooter"];
		}
    }

	~MainMenu() {
		defaultButtonView = footerBackup;
	}

    virtual tsl::elm::Element* createUI() override {

		if (jumpImmediatelyToSingleSmd == true && filesChecked.size() == 1) {
			if (filesChecked[0].is_directory == false && standard_path.compare(root_path) == 0) {		
				std::string full_path = standard_path + filesChecked[0].name;

				smd::Document::PeekInfo info;

				if (smd::Document::Peek(full_path.c_str(), info, overrideLanguage.c_str())) {
					std::string args = "--file " + filesChecked[0].name;
					tsl::setNextOverlay(filepath, args);
					tsl::Overlay::get()->close();
						rootFrame = new tsl::elm::OverlayFrame("", "");
					return rootFrame;
				}
			}
		}

		std::string version = APP_VERSION;
		if (m_folderName.length() > 0) version += "\n\n" + m_folderName;

		rootFrame = new tsl::elm::OverlayFrame(APP_TITLE, version.c_str());
		auto list = new tsl::elm::List();


        if (!filesChecked.empty()) {
            for (const auto& item : filesChecked) {
                if (item.is_directory) {
					std::string localPath = standard_path + item.name + "/";
					std::string localName = lookupSMF(localPath);
					std::string name = localName.length() == 0 ? item.name : localName;
                    auto folderItem = new tsl::elm::ListItem(name, "\uE133", true);
                    folderItem->setClickListener([this, localPath, name](uint64_t keys) {
                        if (keys & KEY_A) {
                            tsl::changeTo<MainMenu>(localPath, name);
                            return true;
                        }
						#ifdef DEBUG
						else if (keys & KEY_Y) {
							tsl::changeTo<MemoryCheck>();
							return true;							
						}
						#endif
						else if (isMainMenu && (keys & KEY_PLUS)) {
							tsl::changeTo<ConfigurationMainMenu>();
							return true;
						}
                        return false;
                    });
                    list->addItem(folderItem);
                } 
                else {
                    std::string full_path = standard_path + item.name;
                    
                    smd::Document::PeekInfo info;
                    smd::Document::Peek(full_path.c_str(), info, overrideLanguage.c_str());
					FILE* file = fopen(full_path.c_str(), "rb");
					bool doesHaveConfig = false;
					bool customExitCombo = false;
					if (file) {
						fseek(file, 0, 2);
						size_t size = ftell(file);
						fseek(file, 0, 0);
						char* buffer = 0;
						buffer = (char*)malloc(size);
						if (buffer) {
							fread(buffer, 1, size, file);
							doesHaveConfig = FindConfigs(buffer, size);
							customExitCombo = itHasCustomExitCombo(buffer, size);
							free(buffer);
						}
						fclose(file);
					}
					std::string second = "";
					if (info.name.empty()) second = "\uE098";
					else {
						if (customExitCombo) second = "\uE136";
						if (doesHaveConfig) second += "\uE04F";
					} 
                    auto fileItem = new tsl::elm::ListItem(info.name.empty() ? item.name : info.name, second.c_str(), info.name.empty() ? true : false);
                    fileItem->setClickListener([this, info, full_path, doesHaveConfig](uint64_t keys) {
						if (info.name.empty() == false) {
							if (keys & KEY_A) {
								// SMD dimensions describe widget geometry only. Changing the
								// canonical framebuffer or restarting the overlay per mode breaks
								// compact layouts and their return/lifecycle path.
								tsl::changeTo<RenderingPipeline>(full_path);
								return true;
							}
							else if (doesHaveConfig == true && (keys & KEY_Y)) {
								tsl::changeTo<Configuration>(full_path, info.name);
								return true;
							}
							else if (isMainMenu && (keys & KEY_PLUS)) {
								tsl::changeTo<ConfigurationMainMenu>();
								return true;
							}
						}
                        return false;
                    });
                    list->addItem(fileItem);
                }
            }
            rootFrame->setContent(list);
        }
        else {
            auto Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
                renderer->drawString("No folders or .smd files found!", false, 20, 20, 20, renderer->a(0xFFFF));
            });
            rootFrame->setContent(Status);
        }

        return rootFrame;
    }

	virtual void update() override {}

	virtual bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
		if (handleQuickCombos(keysDown, keysHeld))
			return true;
		if (keysDown & KEY_B) {
			tsl::hlp::requestForeground(true);
			tsl::goBack();
			return true;
		}
		return false;
	}
};

class MonitorOverlay : public tsl::Overlay {
public:

	virtual void initServices() override {
		//Initialize services
		tsl::hlp::doWithSmSession([this]{

			if (hosversionAtLeast(8,0,0)) clkrstCheck = clkrstInitialize();
			else pcvCheck = pcvInitialize();

			if (hosversionAtLeast(5,0,0)) tcCheck = tcInitialize();

			if (hosversionAtLeast(6,0,0) && R_SUCCEEDED(pwmInitialize())) {
				pwmCheck = pwmOpenSession2(&g_ICon, 0x3D000001);
			}

			if (R_SUCCEEDED(nvInitialize())) nvCheck = nvOpen(&fd, "/dev/nvhost-ctrl-gpu");

			psmCheck = psmInitialize();
			i2cCheck = i2cInitialize();

			SaltySD = CheckPort();

			if (SaltySD) {
				LoadSharedMemoryAndRefreshRate();
			}

			smseLoadFolder("sdmc:/config/status-monitor/extensions/");
			smseExecuteAll();
		});
			Hinted = envIsSyscallHinted(0x6F);
		hidGetSixAxisSensorHandles(&sixaxisHandles[Controller_ProController], 1, HidNpadIdType_No1,      HidNpadStyleTag_NpadFullKey);
		hidGetSixAxisSensorHandles(&sixaxisHandles[Controller_JoyConL], 2, HidNpadIdType_No1,      HidNpadStyleTag_NpadJoyDual);
	}

	virtual void exitServices() override {
		for (auto& se : serviceExt) {
			serviceClose(&se.service);
		}

		shmemClose(&_sharedmemory);
		//Exit services
		clkrstExit();
		pcvExit();
		tcExit();
		pwmChannelSessionClose(&g_ICon);
		pwmExit();
		nvClose(fd);
		nvExit();
		psmExit();
		i2cExit();
	}

    virtual void onShow() override {}
    virtual void onHide() override {}

	virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
		// `tsl::loop` has already parsed [ryazhahand] here. Refresh the theme
		// bindings used by SMD after that canonical bootstrap, not only before it.
		ApplyRyazhaTheme();
		// Ryazha Status Monitor uses the library's standard click and navigation
		// vibration once libryazhahand has loaded its global configuration.
		ult::useHapticFeedback = true;

		//Get actual time without using time service
		remove("sdmc:/dddd.dddd");
		FsFileSystem* filesystem = fsdevGetDeviceFileSystem("sdmc");
		char out_path[FS_MAX_PATH] = "";
		fsdevTranslatePath("sdmc:/dddd.dddd", &filesystem, out_path);
		LocalTime.relative_tick = svcGetSystemTick();
		Result rc = fsFsCreateFile(filesystem, out_path, 0, 0);
		if (R_SUCCEEDED(rc)) {
			struct stat attr;
			stat("sdmc:/dddd.dddd", &attr);
			remove("sdmc:/dddd.dddd");
			LocalTime.timestamp = attr.st_mtime;
		}
		if (file_to_load.length() == 0)
        	return initially<MainMenu>("");
		else {
			return initially<RenderingPipelineDummy>(file_to_load);
		}
    }
};

// Canonical libryazhahand owns theme loading. Status Monitor deliberately
// keeps the mode background opaque so every SMD layout has a stable backdrop.
static void ApplyRyazhaTheme() {
	tsl::initializeTheme();
	tsl::initializeThemeVars();
	tsl::defaultBackgroundColor.a = 0xF;

	// SMD mode colors must follow the *same loaded canonical colors* as the
	// renderer. Reading raw INI values before `tsl::loop` parses [ryazhahand]
	// leaves these bindings stale when the selected theme path changes.
	ThemeData.TextColor_int     = tsl::defaultTextColor.rgba;
	ThemeData.CategoryColor_int = tsl::highlightColor1.rgba;
	ThemeData.AccentColor_int   = tsl::highlightColor2.rgba;
	ThemeData.BoxColor_int      = tsl::defaultBackgroundColor.rgba;
}

int main(int argc, char **argv) {
	#if !defined(__SWITCH__) && !defined(__OUNCE__)
		systemtickfrequency = armGetSystemTickFreq();
	#endif

	ParseIniFile();
	ApplyRyazhaTheme();
    
	if (argc > 0) {
		filename = argv[0];
		filepath = folderpath + filename;
	}
	auto loadSmdFile = [](const char* smd_filename) {
		std::string path = "sdmc:/config/status-monitor/modes/";
		path += smd_filename;

			struct stat filedata;
			if (stat(path.c_str(), &filedata) == 0) {
				file_to_load = path;
			}
	};

	// Legacy Ryazha/Status-Monitor-Overlay mode arguments are mapped to the
	// bundled SMD files so pre-Deux shortcuts keep working after the update.
	auto legacySmdFor = [](const char* arg) -> const char* {
		if (strcasecmp(arg, "-micro") == 0
		 || strcasecmp(arg, "--microOverlay") == 0
		 || strcasecmp(arg, "--microOverlay_") == 0) return "03.Micro.smd";
		if (strcasecmp(arg, "-mini") == 0)             return "02.Mini.smd";
		if (strcasecmp(arg, "-full") == 0)             return "01.Full.smd";
		if (strcasecmp(arg, "-fps_graph") == 0)        return "FPS/01.FPSGraph.smd";
		if (strcasecmp(arg, "-fps_counter") == 0)      return "FPS/02.FPSCounter.smd";
		if (strcasecmp(arg, "-game_resolutions") == 0) return "Other/03.GameResolutions.smd";
		return nullptr;
	};

	int arg = 1;
	while (arg < argc) {
		if (strcasecmp(argv[arg], "--file") == 0) {
			if (arg + 1 < argc) {
				loadSmdFile(argv[arg+1]);
			}
		}
		else if (strcasecmp(argv[arg], "--submenu") == 0) {
			tsl::setNextOverlay(filepath, "");
		}
		else if (const char* legacy = legacySmdFor(argv[arg])) {
			loadSmdFile(legacy);
		};
		arg++;
	}
    return tsl::loop<MonitorOverlay>(argc, argv);
}