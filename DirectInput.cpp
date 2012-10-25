#include "DirectInput.h"
#include "LuaScheduler.h"
#include <dinput.h>
#include <assert.h>
#include <map>

using KEngineCore::StringHash;

static class KeyMap : public std::map<KEngineCore::StringHash, int>
{
public:
	KeyMap() {
		std::map<KEngineCore::StringHash, int> & map = *this;
		map[StringHash("ESC")] = DIK_ESCAPE;
		map[StringHash("W")] = DIK_W;
		map[StringHash("A")] = DIK_A;
		map[StringHash("S")] = DIK_S;
		map[StringHash("D")] = DIK_D;
	}
};

static KeyMap keyMap;

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
	RegisterLibrary(scheduler->GetMainState());
}

void KEngineWindows::DirectInput::Update(double time) {
	
	mDirectInputKeyboardDevice->GetDeviceState(sizeof(mKeyboardState),(LPVOID)&mKeyboardState);
	
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
	return (bool)(mKeyboardState[keyMap[keyName]] & 0x80);
}

void KEngineWindows::DirectInput::AddKeybinding(DirectInputKeyBinding * binding)
{
	switch (binding->mType) {
	case onKeyDown:
		mKeyDownBindings.push_front(binding);
		binding->mPosition = mKeyDownBindings.begin();
		binding->mFiresAt = 0;
		break;
	case onKeyUp:		
		mKeyUpBindings.push_front(binding);
		binding->mPosition = mKeyUpBindings.begin();
		binding->mFiresAt = 0;
		break;
	case onKeyRepeat:
		mKeyRepeatBindings.push_front(binding);
		binding->mPosition = mKeyRepeatBindings.begin();
		binding->mFiresAt = 0;
		break;
	}
}

void KEngineWindows::DirectInput::RemoveKeybinding(DirectInputKeyBinding * binding)
{

	binding->mPosition = mKeyDownBindings.end();

	if (mProcessingBinding == binding) {
		mSelfClearedBindings.push_back(binding->mPosition);
	} else {
		switch (binding->mType) {
		case onKeyDown:
			mKeyDownBindings.erase(binding->mPosition);
			break;
		case onKeyUp:		
			mKeyUpBindings.erase(binding->mPosition);
			break;
		case onKeyRepeat:
			mKeyRepeatBindings.erase(binding->mPosition);
			break;
		}	
		binding->mPosition = mKeyDownBindings.end();
	}
}

 KEngineCore::LuaScheduler *  KEngineWindows::DirectInput::GetLuaScheduler() const {
	 return mScheduler;
 }

static int setOnKey(lua_State * luaState, KEngineWindows::KeyBindingType bindingType) {
	KEngineWindows::DirectInput * inputSystem = (KEngineWindows::DirectInput *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::LuaScheduler * scheduler = inputSystem->GetLuaScheduler();
	KEngineCore::StringHash keyName(luaL_checkstring(luaState, 1));
	
	luaL_checktype(luaState, 2, LUA_TFUNCTION);
	
	KEngineWindows::DirectInputKeyBinding * binding = new (lua_newuserdata(luaState, sizeof(KEngineWindows::DirectInputKeyBinding))) KEngineWindows::DirectInputKeyBinding;
	luaL_getmetatable(luaState, "KEngineWindows.DirectInputKeyBinding");
	lua_setmetatable(luaState, -2);

	KEngineCore::ScheduledLuaCallback callback = scheduler->CreateCallback(luaState, 2);
	binding->Init(inputSystem, keyName, bindingType, callback.mCallback, callback.mCancelCallback);
	return 1;
}

static int setOnKeyDown(lua_State * luaState) {
	return setOnKey(luaState, KEngineWindows::onKeyDown);
}
static int setOnKeyUp(lua_State * luaState) {
	return setOnKey(luaState, KEngineWindows::onKeyUp);
}
static int setOnKeyRepeat(lua_State * luaState) {
	return setOnKey(luaState, KEngineWindows::onKeyRepeat);
}

static int clearBinding(lua_State * luaState) {
	KEngineWindows::DirectInputKeyBinding * binding = (KEngineWindows::DirectInputKeyBinding *)luaL_checkudata(luaState, 1, "KEngineWindows.DirectInputKeyBinding"); 
	binding->Cancel();
	return 0;
}

static int waitForKey(lua_State * luaState, KEngineWindows::KeyBindingType bindingType) {
	KEngineWindows::DirectInput * inputSystem = (KEngineWindows::DirectInput *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::LuaScheduler * scheduler = inputSystem->GetLuaScheduler();
	KEngineCore::StringHash keyName(luaL_checkstring(luaState, 1));
	
	KEngineWindows::DirectInputKeyBinding * binding = new (lua_newuserdata(luaState, sizeof(KEngineWindows::DirectInputKeyBinding))) KEngineWindows::DirectInputKeyBinding;
	luaL_getmetatable(luaState, "KEngineWindows.DirectInputKeyBinding");
	lua_setmetatable(luaState, -2);

	KEngineCore::ScheduledLuaThread * scheduledThread = scheduler->GetScheduledThread(luaState);
	scheduledThread->Pause();

	binding->Init(inputSystem, keyName, bindingType, [scheduledThread] () {
		scheduledThread->Resume();
	});
	return lua_yield(luaState, 1);  //Pretend we're going to pass the timeout to resume so it doesn't get GC'd
}

static int waitForKeyDown(lua_State * luaState) {
	return waitForKey(luaState, KEngineWindows::onKeyDown);
}
static int waitForKeyUp(lua_State * luaState) {
	return waitForKey(luaState, KEngineWindows::onKeyUp);
}
static int waitForKeyRepeat(lua_State * luaState) {
	return waitForKey(luaState, KEngineWindows::onKeyRepeat);
}

static int deleteBinding(lua_State * luaState) {
	KEngineWindows::DirectInputKeyBinding * binding = (KEngineWindows::DirectInputKeyBinding *)luaL_checkudata(luaState, 1, "KEngineWindows.DirectInputKeyBinding"); 
	binding->~DirectInputKeyBinding();
	return 0;
}

static int isKeyDown(lua_State * luaState) {
	KEngineWindows::DirectInput * inputSystem = (KEngineWindows::DirectInput *)lua_touserdata(luaState, lua_upvalueindex(1));
	KEngineCore::StringHash keyName(luaL_checkstring(luaState, 1));
	lua_pushboolean(luaState, inputSystem->IsKeyDown(keyName));
	return 1;
}

static const struct luaL_Reg inputLibrary [] = {
	{"setOnKeyDown", setOnKeyDown},
	{"setOnKeyUp", setOnKeyUp},
	{"setOnKeyRepeat", setOnKeyRepeat},
	{"clearBinding", clearBinding},
	{"waitForKeyDown", waitForKeyDown},
	{"waitForKeyUp", waitForKeyUp},
	{"waitForKeyRepeat", waitForKeyRepeat},
	{"isKeyDown", isKeyDown},
	{nullptr, nullptr}
};

static int luaopen_input (lua_State * luaState) {
	luaL_newlibtable(luaState, inputLibrary);
	lua_pushvalue(luaState, lua_upvalueindex(1));
	luaL_setfuncs(luaState, inputLibrary, 1);

	luaL_newmetatable(luaState, "KEngineWindows.DirectInputKeyBinding");
	lua_pushstring(luaState, "__gc");
	lua_pushcfunction(luaState, deleteBinding);
	lua_settable(luaState, -3);
	lua_pop(luaState, 1);
	
	return 1;
};

void KEngineWindows::DirectInput::RegisterLibrary(lua_State * luaState, const char * name) {
	PreloadLibrary(luaState, name, luaopen_input);
}

KEngineWindows::DirectInputKeyBinding::DirectInputKeyBinding()
{
	mDirectInput = nullptr;
	mFiresAt = 0;
}

KEngineWindows::DirectInputKeyBinding::~DirectInputKeyBinding()
{
	Deinit();
}

void KEngineWindows::DirectInputKeyBinding::Init(DirectInput * inputSystem, KEngineCore::StringHash keyName, KeyBindingType type, std::function<void ()> callback, std::function<void()> cancelCallback)
{
	assert(mDirectInput == nullptr);
	mDirectInput = inputSystem;
	mKeyName = keyName;
	mType = type;
	mCallback = callback;
	mCancelCallback = cancelCallback;
	inputSystem->AddKeybinding(this);
}

void KEngineWindows::DirectInputKeyBinding::Deinit()
{
	if (mDirectInput && mPosition != mDirectInput->mKeyDownBindings.end()) {
		Cancel();
	}
	mDirectInput = nullptr;
}

void KEngineWindows::DirectInputKeyBinding::Fire()
{
	assert(mCallback);
	mCallback();
}

void KEngineWindows::DirectInputKeyBinding::Cancel() {
	if (mDirectInput != nullptr && mPosition != mDirectInput->mKeyDownBindings.end()) {
		mDirectInput->RemoveKeybinding(this);
		if (mCancelCallback) {
			mCancelCallback();
		}
	}
}