#include "DirectInput.h"
#include <dinput.h>
#include <assert.h>



KEngineWindows::DirectInput::DirectInput(void)
{
}


KEngineWindows::DirectInput::~DirectInput(void)
{
}

void KEngineWindows::DirectInput::Init(KEngineCore::LuaScheduler * scheduler, HINSTANCE hinstance, HWND hwindow, double repeatFrequency) {
	mScheduler = scheduler;
	DirectInput8Create(hinstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID *)&mDirectInputInterface, nullptr); 
	mDirectInputInterface->CreateDevice(GUID_SysKeyboard, &mDirectInputKeyboardDevice, NULL);
	mDirectInputKeyboardDevice->SetDataFormat(&c_dfDIKeyboard);
	mDirectInputKeyboardDevice->SetCooperativeLevel(hwindow, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
	mDirectInputKeyboardDevice->Acquire();
	mRepeatFrequency = repeatFrequency;
	mCurrentTime = 0;
}

void KEngineWindows::DirectInput::Update(double time) {
	char chr_KeybState[256];
	mDirectInputKeyboardDevice->GetDeviceState(sizeof(chr_KeybState),(LPVOID)&chr_KeybState);
	
	mCurrentTime += (int)(time * 1000);
	
	//Process the KeyDownBindings
	for (auto iterator = mKeyDownBindings.begin(); iterator != mKeyDownBindings.end(); iterator++) {
		mProcessingBinding = *iterator;

		if (IsKeyDown(mProcessingBinding->mKeyName)) {
			if (mProcessingBinding->mFiresAt == 0) {
				mProcessingBinding->mCallback();
				mProcessingBinding->mFiresAt = mCurrentTime;
			}
		} else {
			mProcessingBinding->mFiresAt = 0;
		}
	}
	mProcessingBinding = nullptr;
	for (auto iterator = mSelfClearedBindings.begin(); iterator != mSelfClearedBindings.end(); iterator++) {
		auto selfClearedPosition = *iterator;
		mKeyDownBindings.erase(selfClearedPosition);
	}
	mSelfClearedBindings.clear();

	//Process the KeyRepeatBindings
	for (auto iterator = mKeyRepeatBindings.begin(); iterator != mKeyRepeatBindings.end(); iterator++) {
		mProcessingBinding = *iterator;

		if (IsKeyDown(mProcessingBinding->mKeyName)) {
			if (mProcessingBinding->mFiresAt <= mCurrentTime) {
				mProcessingBinding->mCallback();
				mProcessingBinding->mFiresAt = mCurrentTime + (int)(mRepeatFrequency * 1000);
			}
		} else {
			mProcessingBinding->mFiresAt = 0;
		}
	}
	mProcessingBinding = nullptr;
	for (auto iterator = mSelfClearedBindings.begin(); iterator != mSelfClearedBindings.end(); iterator++) {
		auto selfClearedPosition = *iterator;
		mKeyRepeatBindings.erase(selfClearedPosition);
	}
	mSelfClearedBindings.clear();


	//Process the KeyUpBindings
	for (auto iterator = mKeyUpBindings.begin(); iterator != mKeyUpBindings.end(); iterator++) {
		mProcessingBinding = *iterator;

		if (!IsKeyDown(mProcessingBinding->mKeyName)) {
			if (mProcessingBinding->mFiresAt == 0) {
				mProcessingBinding->mCallback();
				mProcessingBinding->mFiresAt = mCurrentTime;
			}
		} else {
			mProcessingBinding->mFiresAt = 0;
		}
	}
	mProcessingBinding = nullptr;
	for (auto iterator = mSelfClearedBindings.begin(); iterator != mSelfClearedBindings.end(); iterator++) {
		auto selfClearedPosition = *iterator;
		mKeyUpBindings.erase(selfClearedPosition);
	}
	mSelfClearedBindings.clear();


}

bool KEngineWindows::DirectInput::IsKeyDown(KEngineCore::StringHash keyName) {
	return false;
}

void KEngineWindows::DirectInput::RegisterLibrary(lua_State * luaState, const char * name) {
	//TODO
}