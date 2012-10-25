#pragma once

#include <list>
#include <windows.h>
#include <dinput.h>

#include <vector>
#include <functional>
#include "StringHash.h"
#include "LuaLibrary.h"


namespace KEngineCore {
	class LuaScheduler;
}

namespace KEngineWindows {

class DirectInputKeyBinding;

class DirectInput : private KEngineCore::LuaLibrary
{
public:
	DirectInput();
	~DirectInput();
	void Init(KEngineCore::LuaScheduler * scheduler, HINSTANCE hinstance, HWND hwindow, double repeatFrequency);
	void Deinit();

	void Update(double fTime);
	bool IsKeyDown(KEngineCore::StringHash keyName);
	KEngineCore::LuaScheduler * GetLuaScheduler() const;
	virtual void RegisterLibrary(lua_State * luaState, char const * name = "input") override;
private:
	void AddKeybinding(DirectInputKeyBinding * binding);
	void RemoveKeybinding(DirectInputKeyBinding * binding);

	LPDIRECTINPUT8 mDirectInputInterface;	
	LPDIRECTINPUTDEVICE8 mDirectInputKeyboardDevice;
	
	typedef std::list<DirectInputKeyBinding *> KeyBindingList;

	KEngineCore::LuaScheduler *				mScheduler;
	KeyBindingList							mKeyDownBindings;
	KeyBindingList							mKeyUpBindings;
	KeyBindingList							mKeyRepeatBindings;
	DirectInputKeyBinding *					mProcessingBinding;
	std::vector<KeyBindingList::iterator>	mSelfClearedBindings;
	double									mRepeatFrequency;
	int										mCurrentTime;
	char									mKeyboardState[256];

	friend class DirectInputKeyBinding;
};

enum KeyBindingType {
	onKeyDown,
	onKeyUp,
	onKeyRepeat
};

class DirectInputKeyBinding 
{
public:
	DirectInputKeyBinding();
	~DirectInputKeyBinding();

	void Init(DirectInput * inputSystem, KEngineCore::StringHash keyName, KeyBindingType type, std::function<void ()> callback, std::function<void()> cancelCallback = nullptr); //templatize later for memory management
	void Deinit();

	void Fire();
	void Cancel();

private:
	DirectInput *									mDirectInput;
	KEngineCore::StringHash							mKeyName;
	KeyBindingType									mType;
	int												mFiresAt;
	std::list<DirectInputKeyBinding *>::iterator	mPosition;
	std::function<void()>							mCallback;
	std::function<void()>							mCancelCallback;
	
	friend class DirectInput;
};

}

