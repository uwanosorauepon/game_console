#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <dinput.h>

namespace game_console {

typedef bool (*MainLoopFunction)();

// 最初に1回呼び出す
// @param screenSize スクリーンサイズ
void init(COORD screenSize, MainLoopFunction mainloopFunction);

// 定期的に実行する
bool mainloop();

// 最後に1回呼び出す
void end();

// バッファを指定された情報で埋める。
// バッファは自動的にはクリアされない。
void clearBuffer(const CHAR_INFO* clearInfo);

// xyがはみ出しているとnullptrを返す。
CHAR_INFO* getBufferXY(int x, int y);

void printString(int x0, int y0, _In_z_ const char* str);
void printString(int x0, int y0, _In_z_ const char* str, WORD attr);


// input //

enum {
	KeyState_PRESSED = 0x01,
	KeyState_RELEASED = 0x02,
	KeyState_PRESSING = 0x04
};

// キーの状態を取得する
int getKeyState(unsigned char keyCode);

}
