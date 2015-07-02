/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CONTROLS_H
#define GAME_CLIENT_COMPONENTS_CONTROLS_H
#include <base/vmath.h>
#include <game/client/component.h>

// MagicTW
#define MAX_INPUT_SIZE 500

class CControls : public CComponent
{
public:
	vec2 m_MousePos;
	vec2 m_TargetPos;

	CNetObj_PlayerInput m_InputData;
	CNetObj_PlayerInput m_LastData;
	int m_InputDirectionLeft;
	int m_InputDirectionRight;

	// MagicTW
	bool m_SendHook;
	bool m_HookOn;
	vec2 m_RealMousePos;
	double m_Angle;

	CNetObj_PlayerInput m_TabInputData[MAX_INPUT_SIZE];
	bool m_RecordInput;
	bool m_LoadInput;
	bool m_RepeatInput;
	long m_Iter;
	long m_ArraySize;

	CControls();

	virtual void OnReset();
	virtual void OnRelease();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnMouseMove(float x, float y);
	virtual void OnConsoleInit();
	virtual void OnPlayerDeath();

	// MagicTW methods
	static void ConHook(IConsole::IResult *pResult, void *pUserData);
	static void ConHookOn(IConsole::IResult *pResult, void *pUserData);
	static void ConAutoSpin(IConsole::IResult *pResult, void *pUserData);
	static void ConRecordInput(IConsole::IResult *pResult, void *pUserData);
	static void ConSaveInput(IConsole::IResult *pResult, void *pUserData);
	static void ConLoadInput(IConsole::IResult *pResult, void *pUserData);

	// MagicTW fonctions
	void SaveInput(const char *s, bool overwrite);
	int LoadInput(const char *s);
	void DisplayInputStateInConsole(int i);

	int SnapInput(int *pData);
	void ClampMousePos();
	void ClampRealMousePos();
	void UpdateAngle();
};
#endif
