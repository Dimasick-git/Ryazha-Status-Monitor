#pragma once

#include <ini_funcs.hpp>

// Status Monitor historically used these helpers in the global namespace.
// The complete RyazhaTune renderer exposes the same API in namespace ult.
// Keep the application call sites stable while routing all INI work through
// the library implementation that ships with the renderer.
using ult::getParsedDataFromIniFile;
using ult::parseIni;
using ult::parseValueFromIniSection;
using ult::setIniFile;
using ult::setIniFileKey;
using ult::setIniFileValue;
