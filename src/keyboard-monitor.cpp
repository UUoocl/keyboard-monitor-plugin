#include "keyboard-monitor.hpp"
#include "keyboard-settings-dialog.hpp"
#include "version.h"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <thread>
#include <atomic>
#include <cmath>
#include <QMainWindow>
#include <QAction>
#include <QPointer>
#include <QMetaObject>
#include <QTimer>
#include <util/platform.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Andilippi")
OBS_MODULE_USE_DEFAULT_LOCALE("keyboard-monitor", "en-US")

#ifdef _WIN32
HHOOK keyboardHook = NULL;
#endif

#ifdef __linux__
Display *display = nullptr;
std::thread linuxHookThread;
std::atomic<bool> linuxHookRunning{false};
#endif

std::unordered_set<int> pressedKeys;
std::unordered_set<int> activeModifiers;
std::mutex keyStateMutex; // Protects pressedKeys, activeModifiers, and loggedCombinations

#ifdef _WIN32
std::unordered_set<int> modifierKeys = {VK_CONTROL, VK_LCONTROL, VK_RCONTROL, VK_MENU, VK_LMENU, VK_RMENU,
					VK_SHIFT,   VK_LSHIFT,   VK_RSHIFT,   VK_LWIN, VK_RWIN};

std::unordered_set<int> singleKeys = {VK_INSERT, VK_DELETE, VK_HOME, VK_END, VK_PRIOR, VK_NEXT, VK_F1,  VK_F2,  VK_F3,
				      VK_F4,     VK_F5,     VK_F6,   VK_F7,  VK_F8,    VK_F9,   VK_F10, VK_F11, VK_F12,
				      VK_F13,    VK_F14,    VK_F15,  VK_F16, VK_F17,   VK_F18,  VK_F19, VK_F20, VK_F21,
				      VK_F22,    VK_F23,    VK_F24};

std::unordered_set<int> numpadKeys = {VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4,
				      VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9,
				      VK_MULTIPLY, VK_ADD, VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE};

std::unordered_set<int> numberKeys = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

std::unordered_set<int> letterKeys = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
				      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

std::unordered_set<int> punctuationKeys = {VK_OEM_1,      VK_OEM_PLUS,   VK_OEM_COMMA, VK_OEM_MINUS, VK_OEM_PERIOD,
					   VK_OEM_2,      VK_OEM_3,      VK_OEM_4,     VK_OEM_5,     VK_OEM_6,
					   VK_OEM_7,      VK_OEM_102,    VK_SPACE,     VK_TAB,       VK_OEM_8,
					   VK_OEM_AX,     VK_OEM_CLEAR,  VK_BACK};
#endif

#ifdef __APPLE__
std::unordered_set<int> modifierKeys = {kVK_Control,      kVK_Command,      kVK_Option,      kVK_Shift,
					kVK_RightControl, kVK_RightCommand, kVK_RightOption, kVK_RightShift};

std::unordered_set<int> singleKeys = {
	kVK_ANSI_Keypad0, kVK_ANSI_Keypad1, kVK_ANSI_Keypad2, kVK_ANSI_Keypad3, kVK_ANSI_Keypad4,     kVK_ANSI_Keypad5,
	kVK_ANSI_Keypad6, kVK_ANSI_Keypad7, kVK_ANSI_Keypad8, kVK_ANSI_Keypad9, kVK_ANSI_KeypadClear, kVK_ANSI_KeypadEnter,
	kVK_Escape,       kVK_Delete,       kVK_Home,         kVK_End,          kVK_PageUp,           kVK_PageDown,
	kVK_Return,       kVK_F1,           kVK_F2,           kVK_F3,           kVK_F4,               kVK_F5,
	kVK_F6,           kVK_F7,           kVK_F8,           kVK_F9,           kVK_F10,              kVK_F11,
	kVK_F12,          kVK_F13,          kVK_F14,          kVK_F15,          kVK_F16,              kVK_F17,
	kVK_F18,          kVK_F19,          kVK_F20};

std::unordered_set<int> numpadKeys = {kVK_ANSI_Keypad0,     kVK_ANSI_Keypad1,    kVK_ANSI_Keypad2,
				      kVK_ANSI_Keypad3,     kVK_ANSI_Keypad4,    kVK_ANSI_Keypad5,
				      kVK_ANSI_Keypad6,     kVK_ANSI_Keypad7,    kVK_ANSI_Keypad8,
				      kVK_ANSI_Keypad9,     kVK_ANSI_KeypadClear, kVK_ANSI_KeypadEnter,
				      kVK_ANSI_KeypadPlus,  kVK_ANSI_KeypadMinus, kVK_ANSI_KeypadMultiply,
				      kVK_ANSI_KeypadDivide, kVK_ANSI_KeypadDecimal, kVK_ANSI_KeypadEquals};

std::unordered_set<int> numberKeys = {kVK_ANSI_0, kVK_ANSI_1, kVK_ANSI_2, kVK_ANSI_3, kVK_ANSI_4,
				      kVK_ANSI_5, kVK_ANSI_6, kVK_ANSI_7, kVK_ANSI_8, kVK_ANSI_9};

std::unordered_set<int> letterKeys = {kVK_ANSI_A, kVK_ANSI_B, kVK_ANSI_C, kVK_ANSI_D, kVK_ANSI_E, kVK_ANSI_F,
				      kVK_ANSI_G, kVK_ANSI_H, kVK_ANSI_I, kVK_ANSI_J, kVK_ANSI_K, kVK_ANSI_L,
				      kVK_ANSI_M, kVK_ANSI_N, kVK_ANSI_O, kVK_ANSI_P, kVK_ANSI_Q, kVK_ANSI_R,
				      kVK_ANSI_S, kVK_ANSI_T, kVK_ANSI_U, kVK_ANSI_V, kVK_ANSI_W, kVK_ANSI_X,
				      kVK_ANSI_Y, kVK_ANSI_Z};

std::unordered_set<int> punctuationKeys = {
	kVK_ANSI_Semicolon,     kVK_ANSI_Quote,        kVK_ANSI_Comma,       kVK_ANSI_Period,
	kVK_ANSI_Slash,         kVK_ANSI_Backslash,    kVK_ANSI_LeftBracket, kVK_ANSI_RightBracket,
	kVK_ANSI_Grave,         kVK_ANSI_Equal,        kVK_ANSI_Minus,       kVK_Space,
	kVK_Tab,                kVK_Delete,            kVK_ForwardDelete};

static const std::unordered_map<char, int> macLetterKeycodes = {
	{'A', kVK_ANSI_A}, {'B', kVK_ANSI_B}, {'C', kVK_ANSI_C}, {'D', kVK_ANSI_D},
	{'E', kVK_ANSI_E}, {'F', kVK_ANSI_F}, {'G', kVK_ANSI_G}, {'H', kVK_ANSI_H},
	{'I', kVK_ANSI_I}, {'J', kVK_ANSI_J}, {'K', kVK_ANSI_K}, {'L', kVK_ANSI_L},
	{'M', kVK_ANSI_M}, {'N', kVK_ANSI_N}, {'O', kVK_ANSI_O}, {'P', kVK_ANSI_P},
	{'Q', kVK_ANSI_Q}, {'R', kVK_ANSI_R}, {'S', kVK_ANSI_S}, {'T', kVK_ANSI_T},
	{'U', kVK_ANSI_U}, {'V', kVK_ANSI_V}, {'W', kVK_ANSI_W}, {'X', kVK_ANSI_X},
	{'Y', kVK_ANSI_Y}, {'Z', kVK_ANSI_Z}
};
static const std::unordered_map<char, int> macDigitKeycodes = {
	{'0', kVK_ANSI_0}, {'1', kVK_ANSI_1}, {'2', kVK_ANSI_2}, {'3', kVK_ANSI_3},
	{'4', kVK_ANSI_4}, {'5', kVK_ANSI_5}, {'6', kVK_ANSI_6}, {'7', kVK_ANSI_7},
	{'8', kVK_ANSI_8}, {'9', kVK_ANSI_9}
};
#endif

#ifdef __linux__
std::unordered_set<int> modifierKeys = {XK_Control_L, XK_Control_R, XK_Super_L, XK_Super_R,
					XK_Alt_L,     XK_Alt_R,     XK_Shift_L, XK_Shift_R};

std::unordered_set<int> singleKeys = {XK_Insert,  XK_Delete,  XK_Home,    XK_End,    XK_Page_Up, XK_Page_Down, XK_F1,      XK_F2,
				      XK_F3,      XK_F4,      XK_F5,      XK_F6,     XK_F7,      XK_F8,        XK_F9,      XK_F10,
				      XK_F11,     XK_F12,     XK_F13,     XK_F14,    XK_F15,     XK_F16,       XK_F17,     XK_F18,
				      XK_F19,     XK_F20,     XK_F21,     XK_F22,    XK_F23,     XK_F24,       XK_Return};

std::unordered_set<int> numpadKeys = {XK_KP_0,        XK_KP_1,      XK_KP_2,       XK_KP_3,
				      XK_KP_4,        XK_KP_5,      XK_KP_6,       XK_KP_7,
				      XK_KP_8,        XK_KP_9,      XK_KP_Add,     XK_KP_Subtract,
				      XK_KP_Multiply, XK_KP_Divide, XK_KP_Decimal, XK_KP_Enter};

std::unordered_set<int> numberKeys = {XK_0, XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9};

std::unordered_set<int> letterKeys = {XK_a, XK_b, XK_c, XK_d, XK_e, XK_f, XK_g, XK_h, XK_i,
				      XK_j, XK_k, XK_l, XK_m, XK_n, XK_o, XK_p, XK_q, XK_r,
				      XK_s, XK_t, XK_u, XK_v, XK_w, XK_x, XK_y, XK_z,
				      XK_A, XK_B, XK_C, XK_D, XK_E, XK_F, XK_G, XK_H, XK_I,
				      XK_J, XK_K, XK_L, XK_M, XK_N, XK_O, XK_P, XK_Q, XK_R,
				      XK_S, XK_T, XK_U, XK_V, XK_W, XK_X, XK_Y, XK_Z};

std::unordered_set<int> punctuationKeys = {
	XK_semicolon, XK_comma,     XK_period,    XK_slash,      XK_backslash,   XK_apostrophe,
	XK_grave,     XK_bracketleft, XK_bracketright, XK_equal,      XK_minus,       XK_space,
	XK_Tab,       XK_BackSpace, XK_Return};
#endif

std::unordered_set<std::string> loggedCombinations;

// Settings globals
bool captureNumpad = false;
bool captureNumbers = false;
bool captureLetters = false;
bool capturePunctuation = false;
std::unordered_set<int> whitelistedKeySet;
bool enableLogging = false;
std::string keySeparator = " + ";
std::string lastKeyCombination;
static bool sendKeyboard = true;
bool startWithOBS = false;

static QPointer<KeyboardSettingsDialog> settingsDialog;
obs_hotkey_id toggleHotkeyId = OBS_INVALID_HOTKEY_ID;
static bool deferredHookEnable = false;

// Key name lookup tables
#ifdef _WIN32
static const std::unordered_map<int, const char *> keyNameMap = {
	{VK_CONTROL, "Ctrl"},       {VK_LCONTROL, "Ctrl"},        {VK_RCONTROL, "Ctrl"},        {VK_MENU, "Alt"},
	{VK_LMENU, "Alt"},           {VK_RMENU, "Alt"},            {VK_SHIFT, "Shift"},         {VK_LSHIFT, "Shift"},
	{VK_RSHIFT, "Shift"},        {VK_LWIN, "Win"},            {VK_RWIN, "Win"},            {VK_RETURN, "Enter"},
	{VK_SPACE, "Space"},         {VK_BACK, "Backspace"},      {VK_TAB, "Tab"},              {VK_ESCAPE, "Escape"},
	{VK_PRIOR, "Page Up"},       {VK_NEXT, "Page Down"},       {VK_END, "End"},              {VK_HOME, "Home"},
	{VK_LEFT, "Left Arrow"},      {VK_UP, "Up Arrow"},          {VK_RIGHT, "Right Arrow"},   {VK_DOWN, "Down Arrow"},
	{VK_INSERT, "Insert"},       {VK_DELETE, "Delete"},       {VK_F1, "F1"},                {VK_F2, "F2"},
	{VK_F3, "F3"},               {VK_F4, "F4"},                {VK_F5, "F5"},                {VK_F6, "F6"},
	{VK_F7, "F7"},               {VK_F8, "F8"},                {VK_F9, "F9"},                {VK_F10, "F10"},
	{VK_F11, "F11"},             {VK_F12, "F12"}};
#endif

#ifdef __APPLE__
static const std::unordered_map<int, const char *> keyNameMap = {
	{kVK_Control, "Ctrl"},          {kVK_RightControl, "Ctrl"},   {kVK_Command, "Cmd"},
	{kVK_RightCommand, "Cmd"},      {kVK_Option, "Alt"},          {kVK_RightOption, "Alt"},
	{kVK_Shift, "Shift"},           {kVK_RightShift, "Shift"},    {kVK_ANSI_KeypadEnter, "Enter"},
	{kVK_Return, "Enter"},          {kVK_Space, "Space"},         {kVK_Delete, "Backspace"},
	{kVK_ForwardDelete, "Delete"},  {kVK_Tab, "Tab"},               {kVK_Escape, "Escape"},
	{kVK_PageUp, "Page Up"},        {kVK_PageDown, "Page Down"},    {kVK_End, "End"},
	{kVK_Home, "Home"},             {kVK_LeftArrow, "Left Arrow"},  {kVK_UpArrow, "Up Arrow"},
	{kVK_RightArrow, "Right Arrow"}, {kVK_DownArrow, "Down Arrow"}, {kVK_Help, "Insert"},
	{kVK_ANSI_Keypad0, "Numpad 0"}, {kVK_ANSI_Keypad1, "Numpad 1"}, {kVK_ANSI_Keypad2, "Numpad 2"},
	{kVK_ANSI_Keypad3, "Numpad 3"}, {kVK_ANSI_Keypad4, "Numpad 4"}, {kVK_ANSI_Keypad5, "Numpad 5"},
	{kVK_ANSI_Keypad6, "Numpad 6"}, {kVK_ANSI_Keypad7, "Numpad 7"}, {kVK_ANSI_Keypad8, "Numpad 8"},
	{kVK_ANSI_Keypad9, "Numpad 9"}, {kVK_ANSI_KeypadDecimal, "Numpad ."}, {kVK_ANSI_KeypadMultiply, "Numpad *"},
	{kVK_ANSI_KeypadPlus, "Numpad +"}, {kVK_ANSI_KeypadClear, "Num Lock"}, {kVK_ANSI_KeypadDivide, "Numpad /"},
	{kVK_ANSI_KeypadMinus, "Numpad -"}, {kVK_ANSI_KeypadEquals, "Numpad ="}};
#endif

#ifdef __linux__
static const std::unordered_map<int, const char *> keyNameMap = {
	{XK_Control_L, "Ctrl"},    {XK_Control_R, "Ctrl"},   {XK_Super_L, "Super"},
	{XK_Super_R, "Super"},     {XK_Alt_L, "Alt"},        {XK_Alt_R, "Alt"},
	{XK_Shift_L, "Shift"},     {XK_Shift_R, "Shift"},    {XK_Return, "Enter"},
	{XK_space, "Space"},       {XK_BackSpace, "Backspace"}, {XK_Tab, "Tab"},
	{XK_Escape, "Escape"}};
#endif

static bool isModifierKeyPressedLocked()
{
	for (const int key : activeModifiers) {
		if (pressedKeys.count(key)) {
			return true;
		}
	}
	return false;
}

std::string getKeyName(int vkCode)
{
	auto it = keyNameMap.find(vkCode);
	if (it != keyNameMap.end()) {
		return it->second;
	}

#ifdef _WIN32
	UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
	char keyName[128];
	if (GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName)) > 0) {
		return std::string(keyName);
	}
#endif

#ifdef __APPLE__
	TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
	if (currentKeyboard) {
		CFDataRef layoutData = (CFDataRef)TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
		if (layoutData) {
			const UCKeyboardLayout *keyboardLayout = (const UCKeyboardLayout *)CFDataGetBytePtr(layoutData);
			UInt32 deadKeyState = 0;
			UniChar chars[4];
			UniCharCount actualLength;

			OSStatus status = UCKeyTranslate(keyboardLayout, vkCode, kUCKeyActionDisplay, 0, LMGetKbdType(),
							 kUCKeyTranslateNoDeadKeysBit, &deadKeyState,
							 sizeof(chars) / sizeof(chars[0]), &actualLength, chars);

			if (status == noErr && actualLength > 0) {
				CFStringRef cfString = CFStringCreateWithCharacters(kCFAllocatorDefault, chars, actualLength);
				char buffer[64];
				if (CFStringGetCString(cfString, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
					std::string result(buffer);
					if (result.length() == 1 && result[0] >= 'a' && result[0] <= 'z') {
						result[0] = result[0] - ('a' - 'A');
					}
					CFRelease(cfString);
					CFRelease(currentKeyboard);
					return result;
				}
				CFRelease(cfString);
			}
		}
		CFRelease(currentKeyboard);
	}
#endif

	return "Unknown";
}

static std::string getCurrentCombinationLocked()
{
	std::vector<int> orderedKeys;
#ifdef _WIN32
	orderedKeys = {VK_CONTROL, VK_LCONTROL, VK_RCONTROL, VK_LWIN,   VK_RWIN,  VK_MENU,
		       VK_LMENU,   VK_RMENU,    VK_SHIFT,    VK_LSHIFT, VK_RSHIFT};
#elif defined(__APPLE__)
	orderedKeys = {kVK_Control,      kVK_Command,      kVK_Option,      kVK_Shift,
		       kVK_RightControl, kVK_RightCommand, kVK_RightOption, kVK_RightShift};
#elif defined(__linux__)
	orderedKeys = {XK_Control_L, XK_Control_R, XK_Super_L, XK_Super_R, XK_Alt_L, XK_Alt_R, XK_Shift_L, XK_Shift_R};
#endif

	std::vector<std::string> keys;
	std::unordered_set<std::string> addedModifierNames;
	for (const int key : orderedKeys) {
		if (pressedKeys.count(key)) {
			std::string name = getKeyName(key);
			if (addedModifierNames.insert(name).second) {
				keys.push_back(name);
			}
		}
	}

	for (const int key : pressedKeys) {
		if (modifierKeys.find(key) == modifierKeys.end()) {
			keys.push_back(getKeyName(key));
		}
	}

	std::string combination;
	if (!keys.empty()) {
		combination = keys[0];
		for (size_t i = 1; i < keys.size(); ++i) {
			combination += keySeparator + keys[i];
		}
	}

	return combination;
}

bool shouldCaptureSingleKey(int keyCode)
{
	if (whitelistedKeySet.count(keyCode) > 0) return true;
	if (captureNumpad && numpadKeys.count(keyCode) > 0) return true;
	if (captureNumbers && numberKeys.count(keyCode) > 0) return true;
	if (captureLetters && letterKeys.count(keyCode) > 0) return true;
	if (capturePunctuation && punctuationKeys.count(keyCode) > 0) return true;
	if (singleKeys.count(keyCode) > 0) return true;
	return false;
}

static bool shouldLogCombinationLocked()
{
	bool onlyShiftPressed = false;
#ifdef _WIN32
	onlyShiftPressed = activeModifiers.size() == 1 &&
	    (activeModifiers.count(VK_SHIFT) > 0 || activeModifiers.count(VK_LSHIFT) > 0 || activeModifiers.count(VK_RSHIFT) > 0);
#elif defined(__APPLE__)
	onlyShiftPressed = activeModifiers.size() == 1 && (activeModifiers.count(kVK_Shift) > 0 || activeModifiers.count(kVK_RightShift) > 0);
#elif defined(__linux__)
	onlyShiftPressed = activeModifiers.size() == 1 && (activeModifiers.count(XK_Shift_L) > 0 || activeModifiers.count(XK_Shift_R) > 0);
#endif

	if (onlyShiftPressed) {
		for (const int key : pressedKeys) {
			if (modifierKeys.find(key) == modifierKeys.end()) return true;
		}
		return false;
	}
	return true;
}

void broadcastKeyboardSignal(const std::string &keyCombination)
{
	if (!sendKeyboard || keyCombination.empty()) {
		return;
	}

	obs_data_t *packet = obs_data_create();
	obs_data_set_string(packet, "t", "keyboard");
	obs_data_set_string(packet, "a", "input");
	obs_data_set_string(packet, "v", keyCombination.c_str());

	obs_data_array_t *key_presses_array = obs_data_array_create();
	{
		std::lock_guard<std::mutex> lock(keyStateMutex);
		for (const int key : pressedKeys) {
			obs_data_t *key_data = obs_data_create();
			obs_data_set_string(key_data, "key", getKeyName(key).c_str());
			obs_data_array_push_back(key_presses_array, key_data);
			obs_data_release(key_data);
		}
	}
	obs_data_set_array(packet, "keys", key_presses_array);
	obs_data_array_release(key_presses_array);

	if (enableLogging) {
		const char *json_data = obs_data_get_json(packet);
		blog(LOG_INFO, "[Keyboard Monitor] Broadcasting signal: %s", json_data);
	}

	signal_handler_t *sh = obs_get_signal_handler();
	if (sh) {
		calldata_t cd = {0};
		calldata_set_ptr(&cd, "packet", packet);
		signal_handler_signal(sh, "media_warp_transmit", &cd);
	}

	obs_data_release(packet);
}

static void on_media_warp_receive(void *data, calldata_t *cd)
{
	(void)data;
	const char *json_str = calldata_string(cd, "json_str");
	if (!json_str) return;

	obs_data_t *msg = obs_data_create_from_json(json_str);
	if (!msg) return;

	const char *type = obs_data_get_string(msg, "t");
	const char *addr = obs_data_get_string(msg, "a");

	if (type && std::string(type) == "control") {
		if (addr && std::string(addr) == "log_toggle") {
			enableLogging = !enableLogging;
			blog(LOG_INFO, "[Keyboard Monitor] Logging toggled via remote: %s", enableLogging ? "ON" : "OFF");
		}
	}

	obs_data_release(msg);
}

#ifdef _WIN32
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			std::string keyCombination;
			bool shouldLog = false;
			{
				std::lock_guard<std::mutex> lock(keyStateMutex);
				pressedKeys.insert(p->vkCode);
				if (modifierKeys.count(p->vkCode)) activeModifiers.insert(p->vkCode);

				bool comboTrigger = pressedKeys.size() > 1 && isModifierKeyPressedLocked() && shouldLogCombinationLocked();
				bool singleTrigger = !comboTrigger && shouldCaptureSingleKey(p->vkCode) && !isModifierKeyPressedLocked();

				if (comboTrigger) keyCombination = getCurrentCombinationLocked();
				else if (singleTrigger) keyCombination = getKeyName(p->vkCode);

				if (!keyCombination.empty()) {
					if (loggedCombinations.find(keyCombination) == loggedCombinations.end()) {
						loggedCombinations.insert(keyCombination);
						shouldLog = true;
					}
				}
			}
			if (shouldLog) {
				if (enableLogging) blog(LOG_INFO, "[Keyboard Monitor] Keys pressed: %s", keyCombination.c_str());
				broadcastKeyboardSignal(keyCombination);
			}
		} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
			std::lock_guard<std::mutex> lock(keyStateMutex);
			pressedKeys.erase(p->vkCode);
			if (modifierKeys.count(p->vkCode)) activeModifiers.erase(p->vkCode);
			loggedCombinations.clear();
		}
	}
	return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}
#endif

#ifdef __APPLE__
CFMachPortRef eventTap = nullptr;

bool checkMacOSAccessibilityPermissions()
{
	bool hasPermission = AXIsProcessTrusted();
	if (!hasPermission) {
		blog(LOG_WARNING, "[Keyboard Monitor] Accessibility permissions not granted!");
	}
	return hasPermission;
}

CGEventRef CGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

void startMacOSKeyboardHook()
{
	if (!checkMacOSAccessibilityPermissions()) return;

	CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp) | CGEventMaskBit(kCGEventFlagsChanged);
	eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, eventMask, CGEventCallback, nullptr);

	if (!eventTap) {
		blog(LOG_ERROR, "[Keyboard Monitor] Failed to create event tap!");
		return;
	}

	CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
	CGEventTapEnable(eventTap, true);
	CFRelease(runLoopSource);
	blog(LOG_INFO, "[Keyboard Monitor] macOS keyboard hook started successfully");
}

void stopMacOSKeyboardHook()
{
	if (eventTap) {
		CFRelease(eventTap);
		eventTap = nullptr;
	}
}

CGEventRef CGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
	(void)proxy;
	(void)refcon;

	if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
		if (eventTap) CGEventTapEnable(eventTap, true);
		return event;
	}

	if (type == kCGEventKeyDown || type == kCGEventKeyUp) {
		CGKeyCode keyCode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
		std::string keyCombination;
		bool shouldLog = false;

		if (type == kCGEventKeyDown) {
			std::lock_guard<std::mutex> lock(keyStateMutex);
			pressedKeys.insert(keyCode);
			if (modifierKeys.count(keyCode)) activeModifiers.insert(keyCode);

			bool comboTrigger = pressedKeys.size() > 1 && isModifierKeyPressedLocked() && shouldLogCombinationLocked();
			bool singleTrigger = !comboTrigger && shouldCaptureSingleKey(keyCode) && !isModifierKeyPressedLocked();

			if (comboTrigger) keyCombination = getCurrentCombinationLocked();
			else if (singleTrigger) keyCombination = getKeyName(keyCode);

			if (!keyCombination.empty() && loggedCombinations.find(keyCombination) == loggedCombinations.end()) {
				loggedCombinations.insert(keyCombination);
				shouldLog = true;
			}
		} else {
			std::lock_guard<std::mutex> lock(keyStateMutex);
			pressedKeys.erase(keyCode);
			if (modifierKeys.count(keyCode)) activeModifiers.erase(keyCode);
			loggedCombinations.clear();
		}

		if (shouldLog) {
			if (enableLogging) blog(LOG_INFO, "[Keyboard Monitor] Keys pressed: %s", keyCombination.c_str());
			broadcastKeyboardSignal(keyCombination);
		}
	} else if (type == kCGEventFlagsChanged) {
		CGKeyCode keyCode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
		CGEventFlags flags = CGEventGetFlags(event);
		bool isDown = false;
		if (keyCode == kVK_Shift || keyCode == kVK_RightShift) isDown = (flags & kCGEventFlagMaskShift);
		else if (keyCode == kVK_Control || keyCode == kVK_RightControl) isDown = (flags & kCGEventFlagMaskControl);
		else if (keyCode == kVK_Option || keyCode == kVK_RightOption) isDown = (flags & kCGEventFlagMaskAlternate);
		else if (keyCode == kVK_Command || keyCode == kVK_RightCommand) isDown = (flags & kCGEventFlagMaskCommand);

		std::lock_guard<std::mutex> lock(keyStateMutex);
		if (isDown) {
			pressedKeys.insert(keyCode);
			activeModifiers.insert(keyCode);
		} else {
			pressedKeys.erase(keyCode);
			activeModifiers.erase(keyCode);
			loggedCombinations.clear();
		}
	}
	return event;
}
#endif

#ifdef __linux__
void linuxKeyboardHookThreadFunc()
{
	display = XOpenDisplay(nullptr);
	if (!display) {
		blog(LOG_ERROR, "[Keyboard Monitor] Failed to open X display!");
		linuxHookRunning = false;
		return;
	}
	Window root = DefaultRootWindow(display);
	XSelectInput(display, root, KeyPressMask | KeyReleaseMask);
	blog(LOG_INFO, "[Keyboard Monitor] Linux keyboard hook thread started");
	XEvent event;
	while (linuxHookRunning) {
		while (XPending(display)) {
			XNextEvent(display, &event);
			if (event.type == X11_KeyPress) {
				KeySym keysym = XLookupKeysym(&event.xkey, 0);
				std::string keyCombination;
				bool shouldLog = false;
				{
					std::lock_guard<std::mutex> lock(keyStateMutex);
					pressedKeys.insert(keysym);
					if (modifierKeys.count(keysym)) activeModifiers.insert(keysym);
					bool comboTrigger = pressedKeys.size() > 1 && isModifierKeyPressedLocked() && shouldLogCombinationLocked();
					bool singleTrigger = !comboTrigger && shouldCaptureSingleKey(keysym) && !isModifierKeyPressedLocked();
					if (comboTrigger) keyCombination = getCurrentCombinationLocked();
					else if (singleTrigger) keyCombination = getKeyName(keysym);
					if (!keyCombination.empty() && loggedCombinations.find(keyCombination) == loggedCombinations.end()) {
						loggedCombinations.insert(keyCombination);
						shouldLog = true;
					}
				}
				if (shouldLog) {
					if (enableLogging) blog(LOG_INFO, "[Keyboard Monitor] Keys pressed: %s", keyCombination.c_str());
					broadcastKeyboardSignal(keyCombination);
				}
			} else if (event.type == X11_KeyRelease) {
				KeySym keysym = XLookupKeysym(&event.xkey, 0);
				std::lock_guard<std::mutex> lock(keyStateMutex);
				pressedKeys.erase(keysym);
				if (modifierKeys.count(keysym)) activeModifiers.erase(keysym);
				loggedCombinations.clear();
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	XCloseDisplay(display);
}
void startLinuxKeyboardHook() { linuxHookRunning = true; linuxHookThread = std::thread(linuxKeyboardHookThreadFunc); }
void stopLinuxKeyboardHook() { linuxHookRunning = false; if (linuxHookThread.joinable()) linuxHookThread.join(); }
#endif

bool monitoringEnabled = false;

bool EnableMonitoring()
{
	if (monitoringEnabled) return true;
#ifdef _WIN32
	keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
	monitoringEnabled = (keyboardHook != NULL);
#elif defined(__APPLE__)
	startMacOSKeyboardHook();
	monitoringEnabled = (eventTap != NULL);
#elif defined(__linux__)
	startLinuxKeyboardHook();
	monitoringEnabled = linuxHookRunning;
#endif
	return monitoringEnabled;
}

void DisableMonitoring()
{
	if (!monitoringEnabled) return;
#ifdef _WIN32
	if (keyboardHook) UnhookWindowsHookEx(keyboardHook);
	keyboardHook = NULL;
#elif defined(__APPLE__)
	stopMacOSKeyboardHook();
#elif defined(__linux__)
	stopLinuxKeyboardHook();
#endif
	monitoringEnabled = false;
}

bool IsMonitoringEnabled()
{
	return monitoringEnabled;
}

obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving)
{
	static obs_data_t *settings = nullptr;
	if (saving && save_data) {
		if (settings != save_data) {
			if (settings) obs_data_release(settings);
			settings = obs_data_newref(save_data);
		}
		
		char *config_dir = obs_module_config_path(nullptr);
		if (config_dir) {
			os_mkdirs(config_dir);
			bfree(config_dir);
		}

		char *path = obs_module_config_path("settings.json");
		if (path) {
			blog(LOG_INFO, "[Keyboard Monitor] Saving settings to: %s", path);
			obs_data_save_json(settings, path);
			bfree(path);
		}
	} else if (!saving) {
		if (!settings) {
			char *path = obs_module_config_path("settings.json");
			if (path) {
				blog(LOG_INFO, "[Keyboard Monitor] Loading settings from: %s", path);
				settings = obs_data_create_from_json_file(path);
				bfree(path);
			}
			if (!settings) {
				blog(LOG_INFO, "[Keyboard Monitor] No settings file found, creating defaults");
				settings = obs_data_create();
			}
		}
		return obs_data_newref(settings);
	}
	return nullptr;
}

void loadSingleKeyCaptureSettings(obs_data_t *settings)
{
	captureNumpad = obs_data_get_bool(settings, "captureNumpad");
	captureNumbers = obs_data_get_bool(settings, "captureNumbers");
	captureLetters = obs_data_get_bool(settings, "captureLetters");
	capturePunctuation = obs_data_get_bool(settings, "capturePunctuation");
	const char *whitelistedKeys = obs_data_get_string(settings, "whitelistedKeys");
	whitelistedKeySet.clear();
	std::string wl(whitelistedKeys);
	std::string current;
	for (char c : wl) {
		if (c == ',') {
			if (!current.empty()) {
#ifdef _WIN32
				int vk = (int)current[0];
				if (current.length() > 1 && current[0] >= '0' && current[0] <= '9') vk = std::stoi(current);
#elif defined(__APPLE__)
				int vk = (int)current[0];
				if (macLetterKeycodes.count(std::toupper(current[0]))) vk = macLetterKeycodes.at(std::toupper(current[0]));
				else if (macDigitKeycodes.count(current[0])) vk = macDigitKeycodes.at(current[0]);
#else
				int vk = (int)current[0];
#endif
				whitelistedKeySet.insert(vk);
				current.clear();
			}
		} else if (c != ' ') {
			current += c;
		}
	}
	if (!current.empty()) {
#ifdef _WIN32
		int vk = (int)current[0];
#elif defined(__APPLE__)
		int vk = (int)current[0];
		if (macLetterKeycodes.count(std::toupper(current[0]))) vk = macLetterKeycodes.at(std::toupper(current[0]));
		else if (macDigitKeycodes.count(current[0])) vk = macDigitKeycodes.at(current[0]);
#else
		int vk = (int)current[0];
#endif
		whitelistedKeySet.insert(vk);
	}
	enableLogging = obs_data_get_bool(settings, "enableLogging");
	keySeparator = obs_data_get_string(settings, "keySeparator");
	if (keySeparator.empty()) keySeparator = " + ";
	startWithOBS = obs_data_get_bool(settings, "startWithOBS");
}

static void toggleHotkeyCallback(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	(void)id; (void)hotkey; (void)data;
	if (pressed && settingsDialog) QMetaObject::invokeMethod(settingsDialog, "toggleHook", Qt::QueuedConnection);
}

static void frontendEventCallback(enum obs_frontend_event event, void *data)
{
	(void)data;
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING && deferredHookEnable) {
		EnableMonitoring();
		deferredHookEnable = false;
	} else if (event == OBS_FRONTEND_EVENT_EXIT) {
		DisableMonitoring();
	}
}

static void OpenSettingsDialog()
{
	if (!settingsDialog) {
		QMainWindow *main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
		settingsDialog = new KeyboardSettingsDialog(main_window);
	}
	settingsDialog->show();
	settingsDialog->raise();
	settingsDialog->activateWindow();
}

bool obs_module_load()
{
	blog(LOG_INFO, "[Keyboard Monitor] loaded version %s", PROJECT_VERSION);
	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);
	if (settings) {
		loadSingleKeyCaptureSettings(settings);
		if (startWithOBS) deferredHookEnable = true;
		obs_data_release(settings);
	}
	auto action = (QAction *)obs_frontend_add_tools_menu_qaction(obs_module_text("Settings.Title"));
	QObject::connect(action, &QAction::triggered, []() { OpenSettingsDialog(); });
	toggleHotkeyId = obs_hotkey_register_frontend("keyboard_monitor_toggle", obs_module_text("Hotkey.Toggle"), toggleHotkeyCallback, nullptr);
	
	signal_handler_t *sh = obs_get_signal_handler();
	if (sh) {
		signal_handler_connect(sh, "media_warp_receive", on_media_warp_receive, nullptr);
	}

	obs_frontend_add_event_callback(frontendEventCallback, nullptr);
	return true;
}

void obs_module_unload()
{
	obs_frontend_remove_event_callback(frontendEventCallback, nullptr);
#ifdef _WIN32
	if (keyboardHook) UnhookWindowsHookEx(keyboardHook);
#elif defined(__APPLE__)
	stopMacOSKeyboardHook();
#elif defined(__linux__)
	stopLinuxKeyboardHook();
#endif
	if (toggleHotkeyId != OBS_INVALID_HOTKEY_ID) obs_hotkey_unregister(toggleHotkeyId);
	if (settingsDialog) delete settingsDialog;
}

MODULE_EXPORT const char *obs_module_description(void) { return obs_module_text("Plugin.Description"); }
MODULE_EXPORT const char *obs_module_name(void) { return obs_module_text("Plugin.Name"); }
