#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <dinput.h>

namespace game_console {

typedef bool (*MainLoopFunction)();

// �ŏ���1��Ăяo��
// @param screenSize �X�N���[���T�C�Y
void init(COORD screenSize, MainLoopFunction mainloopFunction);

// ����I�Ɏ��s����
bool mainloop();

// �Ō��1��Ăяo��
void end();

// �o�b�t�@���w�肳�ꂽ���Ŗ��߂�B
// �o�b�t�@�͎����I�ɂ̓N���A����Ȃ��B
void clearBuffer(const CHAR_INFO* clearInfo);

// xy���͂ݏo���Ă����nullptr��Ԃ��B
CHAR_INFO* getBufferXY(int x, int y);

void printString(int x0, int y0, _In_z_ const char* str);
void printString(int x0, int y0, _In_z_ const char* str, WORD attr);


// input //

enum {
	KeyState_PRESSED = 0x01,
	KeyState_RELEASED = 0x02,
	KeyState_PRESSING = 0x04
};

// �L�[�̏�Ԃ��擾����
int getKeyState(unsigned char keyCode);

}
