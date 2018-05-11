#include "game_console.h"

#include <conio.h>
#include <vector>

#pragma comment (lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace game_console {

namespace { //----------------------------------------------------------

			// ゲーム実行中フラグ
bool _gaming = false;

COORD _screenSize;
SMALL_RECT _screenRect;
MainLoopFunction _mainloopFunc = nullptr;

HINSTANCE _hinstance = nullptr;
IDirectInput8 *_directInput8 = nullptr;
IDirectInputDevice8 *_keyboard = nullptr;
char _keyBuffer[256];
char _keyBufferOld[256];

CONSOLE_CURSOR_INFO _initialConsoleCursorInfo;  // カーソルをもとに戻すために使う
HANDLE _consoleOut1 = nullptr;  // スクリーンバッファ1
HANDLE _consoleOut2 = nullptr;  // スクリーンバッファ2
bool _consoleAllocated = false;
bool _bufferFlag = true; // true: fore=1, false: fore=2
std::vector<CHAR_INFO> _buffer;  // 書き込み用バッファ

HANDLE _getBackScreenBuffer()
{
	if (_bufferFlag) return _consoleOut2;
	else return _consoleOut1;
}

void _cleanUp()
{
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &_initialConsoleCursorInfo);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	if (_consoleOut1) CloseHandle(_consoleOut1);
	if (_consoleOut2) CloseHandle(_consoleOut2);

	if (_consoleAllocated) {
		FreeConsole();
	}

	if (_keyboard) {
		_keyboard->Unacquire();
		_keyboard->Release();
		_keyboard = nullptr;
	}

	if (_directInput8) {
		_directInput8->Release();
		_directInput8 = nullptr;
	}

	// 入力バッファをクリアする
	while (_kbhit()) _getch();

	_buffer.clear();
	_mainloopFunc = nullptr;

	_gaming = false;
}

BOOL WINAPI _consoleEventProc(_In_ DWORD in)
{
	if (in == CTRL_C_EVENT || in == CTRL_CLOSE_EVENT) {
		_cleanUp();
	}
	return 0;
}

void _printString(int x0, int y0, _In_z_ const char* str, WORD attr, bool attrValid)
{
	int x = x0;
	int y = y0;
	if (y0 < 0) return;

	while (*str != '\0') {
		if (x >= _screenSize.X) {  // 画面端を超えたら次へ
			++str;
			continue;
		}

		if (x >= 0) {
			// 描画
			if (*str == '\n') {
				x = x0;
				if (++y >= _screenSize.Y) break;
			} else {
				auto* c = getBufferXY(x, y);
				if (attrValid) c->Attributes = attr;
				c->Char.AsciiChar = *str;
				x++;
			}
		}
		++str;
	}
}

} // end unnamed namespace -----------------------------------------------



  // 初期化
void init(COORD screenSize, MainLoopFunction mainloopFunc)
{
	HRESULT hr;

	if (_gaming || screenSize.X <= 0 || screenSize.Y <= 0 || mainloopFunc == nullptr) {
		std::exception();
		return;
	}

	try {
		_gaming = true;

		_screenSize = screenSize;
		_mainloopFunc = mainloopFunc;

		_screenRect.Left = 0;
		_screenRect.Top = 0;
		_screenRect.Right = _screenSize.X - 1;
		_screenRect.Bottom = _screenSize.Y - 1;

		// hinstanceの取得
		_hinstance = GetModuleHandle(nullptr);
		if (_hinstance == nullptr) throw std::exception();

		// DirectInputの初期化
		hr = DirectInput8Create(_hinstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&_directInput8, nullptr);
		if (FAILED(hr)) throw std::exception();

		hr = _directInput8->CreateDevice(GUID_SysKeyboard, &_keyboard, nullptr);
		if (FAILED(hr)) throw std::exception();

		hr = _keyboard->SetDataFormat(&c_dfDIKeyboard);
		if (FAILED(hr)) throw std::exception();

		ZeroMemory(_keyBuffer, sizeof(_keyBuffer));
		ZeroMemory(_keyBufferOld, sizeof(_keyBufferOld));

		HWND hwnd = nullptr;
		TCHAR buf[256];
		GetConsoleTitle(buf, sizeof(buf));
		hwnd = FindWindow(nullptr, buf);
		if (hwnd == nullptr) throw std::exception();

		hr = _keyboard->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
		if (hwnd == nullptr) throw std::exception();

		_keyboard->Acquire();


		// コンソールの初期化
		//BOOL allocated = AllocConsole();
		//g_consoleAllocated = allocated != 0;


		// 色々設定
		HANDLE defaultConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleCursorInfo(defaultConsoleOut, &_initialConsoleCursorInfo);
		SetConsoleCtrlHandler(_consoleEventProc, TRUE);

		// バックバッファの準備
		_consoleOut1 = CreateConsoleScreenBuffer(
			GENERIC_READ | GENERIC_WRITE,
			0,
			nullptr,
			CONSOLE_TEXTMODE_BUFFER,
			nullptr
		);
		_consoleOut2 = CreateConsoleScreenBuffer(
			GENERIC_READ | GENERIC_WRITE,
			0,
			nullptr,
			CONSOLE_TEXTMODE_BUFFER,
			nullptr
		);

		_buffer.resize(_screenSize.X * _screenSize.Y);

		SetConsoleActiveScreenBuffer(_consoleOut1);
		_bufferFlag = true;
	} catch (...) {
		_cleanUp();
		throw;
	}
}

// @return falseでゲーム終了
bool mainloop()
{
	// スクリーンバッファの前処理
	HANDLE backBuffer = _getBackScreenBuffer();
	SetConsoleScreenBufferSize(backBuffer, _screenSize);
	SetConsoleWindowInfo(backBuffer, TRUE, &_screenRect);

	CONSOLE_CURSOR_INFO cci;   // カーソルを非表示にする
	cci.bVisible = false;
	cci.dwSize = 1;
	SetConsoleCursorInfo(backBuffer, &cci);

	// g_buffer のクリアはしない

	// 入力更新
	CopyMemory(_keyBufferOld, _keyBuffer, sizeof(_keyBuffer));

	// 入力バッファをクリアする
	while (_kbhit()) _getch();

	HRESULT hr = _keyboard->GetDeviceState(sizeof(_keyBuffer), (void*)_keyBuffer);
	if (FAILED(hr)) {
		_keyboard->Acquire();
	}

	// ゲームロジック更新とレンダリング
	if (!_mainloopFunc()) return false;

	// g_buffer から スクリーンバッファ への書き込み
	COORD coord = { 0, 0 };
	SMALL_RECT rc = _screenRect;
	WriteConsoleOutputA(backBuffer, _buffer.data(), _screenSize, coord, &rc);

	// バッファを入れ替える
	SetConsoleActiveScreenBuffer(backBuffer);
	_bufferFlag = !_bufferFlag;
	// SetConsoleScreenBufferSize(backBuffer, SCREEN_SIZE);

	return true;
}

// 終了
void end()
{
	_cleanUp();
}

void clearBuffer(const CHAR_INFO* clearInfo)
{
	for (auto& info : _buffer) {
		info = *clearInfo;
	}
}

CHAR_INFO* getBufferXY(int x, int y)
{
	if (x < 0 || x >= _screenSize.X || y < 0 || y >= _screenSize.Y) {
		return nullptr;
	}
	return _buffer.data() + (_screenSize.X * y + x);
}


void printString(int x0, int y0, _In_z_ const char* str)
{
	_printString(x0, y0, str, 0, false);
}

void printString(int x0, int y0, _In_z_ const char* str, WORD attr)
{
	_printString(x0, y0, str, attr, true);
}

int getKeyState(unsigned char keyCode)
{
	int st = 0;
	if (_keyBuffer[keyCode]) st |= KeyState_PRESSING;
	if (_keyBuffer[keyCode] && !_keyBufferOld[keyCode]) st |= KeyState_PRESSED;
	if (!_keyBuffer[keyCode] && _keyBufferOld[keyCode]) st |= KeyState_RELEASED;
	return st;
}

} // end namespace game_console