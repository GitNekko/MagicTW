/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>
#include <game/client/components/chat.h>
#include <game/client/components/menus.h>
#include <game/client/components/scoreboard.h>

#include "controls.h"

//MagicTW includes
#include <string.h>
#include <stdio.h>
#if defined(CONF_FAMILY_WINDOWS)
	#define _USE_MATH_DEFINES
	#include <math.h>
#endif

CControls::CControls()
{
	mem_zero(&m_LastData, sizeof(m_LastData));

	// MagicTW
	m_HookOn=false;
	m_Angle=0;
	m_SendHook=false;

	m_RecordInput=false;
	m_LoadInput=false;
	m_RepeatInput=false;
	m_Iter=0;
	m_ArraySize=0;
}

void CControls::OnReset()
{
	m_LastData.m_Direction = 0;
	m_LastData.m_Hook = 0;
	// simulate releasing the fire button
	if((m_LastData.m_Fire&1) != 0)
		m_LastData.m_Fire++;
	m_LastData.m_Fire &= INPUT_STATE_MASK;
	m_LastData.m_Jump = 0;
	m_InputData = m_LastData;

	m_InputDirectionLeft = 0;
	m_InputDirectionRight = 0;
}

void CControls::OnRelease()
{
	OnReset();
}

void CControls::OnPlayerDeath()
{
	m_LastData.m_WantedWeapon = m_InputData.m_WantedWeapon = 0;
}

static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	((int *)pUserData)[0] = pResult->GetInteger(0);
}

static void ConKeyInputCounter(IConsole::IResult *pResult, void *pUserData)
{
	int *v = (int *)pUserData;
	if(((*v)&1) != pResult->GetInteger(0))
		(*v)++;
	*v &= INPUT_STATE_MASK;
}

struct CInputSet
{
	CControls *m_pControls;
	int *m_pVariable;
	int m_Value;
};

static void ConKeyInputSet(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	if(pResult->GetInteger(0))
		*pSet->m_pVariable = pSet->m_Value;
}

static void ConKeyInputNextPrevWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	ConKeyInputCounter(pResult, pSet->m_pVariable);
	pSet->m_pControls->m_InputData.m_WantedWeapon = 0;
}

void CControls::OnConsoleInit()
{
	// game commands
	Console()->Register("+left", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputDirectionLeft, "Move left");
	Console()->Register("+right", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputDirectionRight, "Move right");
	Console()->Register("+jump", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputData.m_Jump, "Jump");
	Console()->Register("+hook", "", CFGFLAG_CLIENT, ConHook, this, "Hook"); // MagicTW
	//Console()->Register("+hook", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputData.m_Hook, "Hook");
	Console()->Register("+fire", "", CFGFLAG_CLIENT, ConKeyInputCounter, &m_InputData.m_Fire, "Fire");

	// MagicTW
	Console()->Register("MagicTW_hook_on", "", CFGFLAG_CLIENT, ConHookOn, this, "Hold hook");
	Console()->Register("MagicTW_record_input", "", CFGFLAG_CLIENT, ConRecordInput, this, "Records the keyboard and mouse input");
	Console()->Register("MagicTW_save_input", "s?i", CFGFLAG_CLIENT, ConSaveInput, this, "Save a recorded input in a file");
	Console()->Register("MagicTW_load_input", "?s?i", CFGFLAG_CLIENT, ConLoadInput, this, "Load a saved input");

	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 1}; Console()->Register("+weapon1", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to hammer"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 2}; Console()->Register("+weapon2", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to gun"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 3}; Console()->Register("+weapon3", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to shotgun"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 4}; Console()->Register("+weapon4", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to grenade"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 5}; Console()->Register("+weapon5", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to rifle"); }

	{ static CInputSet s_Set = {this, &m_InputData.m_NextWeapon, 0}; Console()->Register("+nextweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, (void *)&s_Set, "Switch to next weapon"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_PrevWeapon, 0}; Console()->Register("+prevweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, (void *)&s_Set, "Switch to previous weapon"); }
}

void CControls::OnMessage(int Msg, void *pRawMsg)
{
	if(Msg == NETMSGTYPE_SV_WEAPONPICKUP)
	{
		CNetMsg_Sv_WeaponPickup *pMsg = (CNetMsg_Sv_WeaponPickup *)pRawMsg;
		if(g_Config.m_ClAutoswitchWeapons)
			m_InputData.m_WantedWeapon = pMsg->m_Weapon+1;
	}
}

int CControls::SnapInput(int *pData)
{
	static int64 LastSendTime = 0;
	bool Send = false;
	// MagicTW
	if(m_LoadInput)
	{
		if((m_TabInputData+m_Iter)->m_Direction==9)
		{
			Send=false;
		  // send once a second just to be sure
		  if(time_get() > LastSendTime + time_freq())
			  Send = true;
	  }
		else
		{
			Send=true;
			mem_copy(&m_InputData, m_TabInputData+m_Iter, sizeof(*(m_TabInputData+m_Iter)));
		}
		
		m_Iter++;
		if(m_Iter>=m_ArraySize)
		{
			if(m_RepeatInput) m_Iter=0;
			else
			{
				m_LoadInput=false;
				DisplayInputStateInConsole(1);
			}
		}

		if(!m_InputData.m_TargetX && !m_InputData.m_TargetY)
		{
			m_InputData.m_TargetX = 1;
			m_MousePos.x = 1;
		}

		m_RealMousePos.x = m_InputData.m_TargetX;
		m_RealMousePos.y = m_InputData.m_TargetY;
		m_MousePos.x = m_InputData.m_TargetX;
		m_MousePos.y = m_InputData.m_TargetY;
	}
	else
	{

	  // update player state
	  if(m_pClient->m_pChat->IsActive())
		  m_InputData.m_PlayerFlags = PLAYERFLAG_CHATTING;
	  else if(m_pClient->m_pMenus->IsActive())
		  m_InputData.m_PlayerFlags = PLAYERFLAG_IN_MENU;
	  else
		  m_InputData.m_PlayerFlags = PLAYERFLAG_PLAYING;

	  if(m_pClient->m_pScoreboard->Active())
		  m_InputData.m_PlayerFlags |= PLAYERFLAG_SCOREBOARD;

	  if(m_LastData.m_PlayerFlags != m_InputData.m_PlayerFlags)
		  Send = true;

	  m_LastData.m_PlayerFlags = m_InputData.m_PlayerFlags;

	  // MagicTW
	  if(g_Config.m_MagicTWAutoSpinSpeed == 0)
		  g_Config.m_MagicTWAutoSpin=false;

	  // we freeze the input if chat or menu is activated
	  if(!(m_InputData.m_PlayerFlags&PLAYERFLAG_PLAYING))
	  {
		  OnReset();
		  // MagicTW
		  if(m_HookOn)
			  m_InputData.m_Hook=1;

		  if(g_Config.m_MagicTWAutoSpin)
		  {
    	  UpdateAngle();
		    // send at at least 10hz
		    if(time_get() > LastSendTime + time_freq()/25)
			    Send = true;
	    }
		  // send once a second just to be sure
		  else if(time_get() > LastSendTime + time_freq())
			  Send = true;
		  mem_copy(pData, &m_InputData, sizeof(m_InputData));
	  }
	  else
	  {
		  // MagicTW
		  if(g_Config.m_MagicTWAutoSpin)
		  {
		    UpdateAngle();
			  Send = true;
		  }
		  else
		  {
			  m_InputData.m_TargetX = (int)m_MousePos.x;
			  m_InputData.m_TargetY = (int)m_MousePos.y;
		  }
		  if(!m_InputData.m_TargetX && !m_InputData.m_TargetY)
		  {
			  m_InputData.m_TargetX = 1;
			  m_MousePos.x = 1;
		  }

		  // set direction
		  m_InputData.m_Direction = 0;
		  if(m_InputDirectionLeft && !m_InputDirectionRight)
			  m_InputData.m_Direction = -1;
		  if(!m_InputDirectionLeft && m_InputDirectionRight)
			  m_InputData.m_Direction = 1;

		  // stress testing
		  if(g_Config.m_DbgStress)
		  {
			  float t = Client()->LocalTime();
			  mem_zero(&m_InputData, sizeof(m_InputData));

			  m_InputData.m_Direction = ((int)t/2)&1;
			  m_InputData.m_Jump = ((int)t);
			  m_InputData.m_Fire = ((int)(t*10));
			  m_InputData.m_Hook = ((int)(t*2))&1;
			  m_InputData.m_WantedWeapon = ((int)t)%NUM_WEAPONS;
			  m_InputData.m_TargetX = (int)(sinf(t*3)*100.0f);
			  m_InputData.m_TargetY = (int)(cosf(t*3)*100.0f);
		  }

		  // check if we need to send input
		  if(m_InputData.m_Direction != m_LastData.m_Direction) Send = true;
		  else if(m_InputData.m_Jump != m_LastData.m_Jump) Send = true;
		  else if(m_InputData.m_Fire != m_LastData.m_Fire) Send = true;
		  else if(m_InputData.m_Hook != m_LastData.m_Hook) Send = true;
		  else if(m_InputData.m_WantedWeapon != m_LastData.m_WantedWeapon) Send = true;
		  else if(m_InputData.m_NextWeapon != m_LastData.m_NextWeapon) Send = true;
		  else if(m_InputData.m_PrevWeapon != m_LastData.m_PrevWeapon) Send = true;

		  // send at at least 10hz
		  if(time_get() > LastSendTime + time_freq()/25)
			  Send = true;
	  }
  }
  // MagicTW
  if(m_HookOn)
	  m_InputData.m_Hook=1;

	// copy and return size
	m_LastData = m_InputData;

	// MagicTW
	if(m_RecordInput)
	{
		if(Send) // send
			mem_copy(m_TabInputData+m_ArraySize, &m_InputData, sizeof(m_InputData));
		else // don't send
			(m_TabInputData+m_ArraySize)->m_Direction=9; // m_Direction usually takes 2 values : -1 and 1

		m_ArraySize++;

		if(m_ArraySize==MAX_INPUT_SIZE)
		{
			ConRecordInput(NULL,(void*)this);
			Console()->ExecuteLine("echo taille max atteinte");
		}
	}

	if(!Send)
		return 0;

	LastSendTime = time_get();
	mem_copy(pData, &m_InputData, sizeof(m_InputData));
	return sizeof(m_InputData);
}

void CControls::OnRender()
{
  vec2 addPos = m_MousePos;
	if(g_Config.m_MagicTWAutoSpin)
	{
	  addPos = m_RealMousePos;
	}
	// update target pos
	if(m_pClient->m_Snap.m_pGameInfoObj && !m_pClient->m_Snap.m_SpecInfo.m_Active)
		m_TargetPos = m_pClient->m_LocalCharacterPos + addPos;
	else if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
		m_TargetPos = m_pClient->m_Snap.m_SpecInfo.m_Position + addPos;
	else
		m_TargetPos = addPos;
}

bool CControls::OnMouseMove(float x, float y)
{
	if((m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED) ||
		(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_pChat->IsActive()))
		return false;

	// MagicTW
	if(!g_Config.m_MagicTWAutoSpin)
	{
		m_MousePos += vec2(x, y); // TODO: ugly
		ClampMousePos();
	}
	else 
	{
		m_RealMousePos += vec2(x, y); // Is it that ugly ??
		ClampRealMousePos();
	}

	return true;
}

void CControls::ClampMousePos()
{
	if(m_pClient->m_Snap.m_SpecInfo.m_Active && !m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
	{
		m_MousePos.x = clamp(m_MousePos.x, 200.0f, Collision()->GetWidth()*32-200.0f);
		m_MousePos.y = clamp(m_MousePos.y, 200.0f, Collision()->GetHeight()*32-200.0f);

	}
	else
	{
		float CameraMaxDistance = 200.0f;
		float FollowFactor = g_Config.m_ClMouseFollowfactor/100.0f;
		float MouseMax = min(CameraMaxDistance/FollowFactor + g_Config.m_ClMouseDeadzone, (float)g_Config.m_ClMouseMaxDistance);

		if(length(m_MousePos) > MouseMax)
			m_MousePos = normalize(m_MousePos)*MouseMax;
	}
}

// MagicTW
void CControls::ClampRealMousePos()
{
	if(m_pClient->m_Snap.m_SpecInfo.m_Active && !m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
	{
		m_RealMousePos.x = clamp(m_RealMousePos.x, 200.0f, Collision()->GetWidth()*32-200.0f);
		m_RealMousePos.y = clamp(m_RealMousePos.y, 200.0f, Collision()->GetHeight()*32-200.0f);
		
	}
	else
	{
		float CameraMaxDistance = 200.0f;
		float FollowFactor = g_Config.m_ClMouseFollowfactor/100.0f;
		float MouseMax = min(CameraMaxDistance/FollowFactor + g_Config.m_ClMouseDeadzone, (float)g_Config.m_ClMouseMaxDistance);
		
		if(length(m_RealMousePos) > MouseMax)
			m_RealMousePos = normalize(m_RealMousePos)*MouseMax;
	}
}

void CControls::UpdateAngle()
{
  m_Angle += g_Config.m_MagicTWAutoSpinSpeed*M_PI/99.9;
  if(m_Angle<0) m_Angle+=2*M_PI;
  else if(m_Angle > 2*M_PI) m_Angle-=2*M_PI;

  if(m_SendHook || m_InputData.m_Fire&1) // firing or hooking
  {
	  m_SendHook=false;
	  m_InputData.m_TargetX = (int)m_RealMousePos.x;
	  m_InputData.m_TargetY = (int)m_RealMousePos.y;
  }
  else
  {
    m_InputData.m_TargetX = (int)(50.0f*cosf(m_Angle));
    m_InputData.m_TargetY = (int)(50.0f*sinf(m_Angle));

    m_MousePos.x=m_InputData.m_TargetX;
    m_MousePos.y=m_InputData.m_TargetY;
  }
}

void CControls::ConHook(IConsole::IResult *pResult, void *pUserData)
{
	((CControls*)pUserData)->m_SendHook=true;
	((CControls*)pUserData)->m_InputData.m_Hook=pResult->GetInteger(0);
	if(pResult->GetInteger(0)==0)
	{
		((CControls*)pUserData)->m_HookOn=0;
		((CControls*)pUserData)->m_InputData.m_Hook=0;
	}
}

void CControls::ConHookOn(IConsole::IResult *pResult, void *pUserData)
{
	((CControls*)pUserData)->m_SendHook=true;
	((CControls*)pUserData)->m_HookOn = !((CControls*)pUserData)->m_HookOn;
	if(((CControls*)pUserData)->m_HookOn == 0) ((CControls*)pUserData)->m_InputData.m_Hook=0;
}

void CControls::ConRecordInput(IConsole::IResult *pResult, void *pUserData)
{
	if( !((CControls*)pUserData)->m_RecordInput ) // if non-recording -> set to 0 to overwrite previous record
	{
		((CControls*)pUserData)->m_Iter=0;
		((CControls*)pUserData)->m_ArraySize=0;
	}
	((CControls*)pUserData)->m_RecordInput = !((CControls*)pUserData)->m_RecordInput;
	((CControls*)pUserData)->DisplayInputStateInConsole(0);
}

void CControls::ConLoadInput(IConsole::IResult *pResult, void *pUserData)
{
	((CControls*)pUserData)->m_RepeatInput=false;
	if(pResult->GetInteger(1)!=0)
		((CControls*)pUserData)->m_RepeatInput=true;


	if(pResult->NumArguments() && pResult->GetString(0)!= NULL && pResult->GetString(0)!='\0') // if non-empty string
		if(((CControls*)pUserData)->LoadInput(pResult->GetString(0))==-1)return;

	((CControls*)pUserData)->m_LoadInput = !((CControls*)pUserData)->m_LoadInput;
	((CControls*)pUserData)->m_Iter=0;
	if(((CControls*)pUserData)->m_ArraySize==0)
	{
		((CControls*)pUserData)->Console()->ExecuteLine("echo no input recorded");
		((CControls*)pUserData)->m_LoadInput=false;
	}
	((CControls*)pUserData)->DisplayInputStateInConsole(1);
}

void CControls::ConSaveInput(IConsole::IResult *pResult, void *pUserData)
{	
	bool overwrite=false;
	if(pResult->GetString(0)== NULL || pResult->GetString(0)=='\0')
		return;

	if(pResult->NumArguments()==2 && pResult->GetInteger(1)!=0) // overwrite option
		overwrite=true;

	((CControls*)pUserData)->SaveInput(pResult->GetString(0),overwrite);
}

void CControls::SaveInput(const char *s, bool overwrite)
{
	if(m_ArraySize==0) // no input recorded
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MagicTW", "fail to save : no input recorded");
		return;
	}

	if (s==NULL || *s=='\0') // bad name
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MagicTW", "fail to save : please write the name of the save file");
		return;
	}

	FILE *f=NULL;
	f=fopen(s,"r");
	if(f!=NULL && !overwrite) // file already exists
	{
		fclose(f);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MagicTW", "fail to save : this file already exists");
		return;
	}

	f=fopen(s,"w+");
	if(f==NULL) // pbm while creating file
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MagicTW", "fail to save : error during file creation");
		return;
	}

	int i;
	fprintf(f,"%ld\n",m_ArraySize);
	
	for(i=0;i<m_ArraySize;i++)
	{
		if(m_TabInputData[i].m_Direction==9) // if send=false, no need to write anything else
			fprintf(f,"%d\n",9);
		else // else, write all
			fprintf(f,"%d %d %d %d %d %d %d %d %d %d\n",
				m_TabInputData[i].m_Direction,
				m_TabInputData[i].m_TargetX,
				m_TabInputData[i].m_TargetY,
				m_TabInputData[i].m_Jump,
				m_TabInputData[i].m_Fire,
				m_TabInputData[i].m_Hook,
				m_TabInputData[i].m_PlayerFlags,
				m_TabInputData[i].m_WantedWeapon,
				m_TabInputData[i].m_NextWeapon,
				m_TabInputData[i].m_PrevWeapon);
	}

	fclose(f);
}

int CControls::LoadInput(const char *s)
{
	if (s==NULL || *s=='\0') // bad name
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MagicTW", "fail to load : please write the name of the load file");
		return -1;
	}

	FILE *f=NULL;
	f=fopen(s,"r");
	if(f==NULL) // file doesn't exist
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MagicTW", "fail to load : this file doesn't exist");
		return -1;
	}

	fseek(f, 0, SEEK_SET);
	if(fscanf(f,"%ld",&m_ArraySize)==EOF)Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MagicTW", "fail to load : corrupted file");
	m_ArraySize=m_ArraySize>MAX_INPUT_SIZE?MAX_INPUT_SIZE:m_ArraySize; // check limit

	if(m_ArraySize==0) // no input recorded
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MagicTW", "fail to load : no input recorded");
		fclose(f);
		return -1;
	}

	int direction,result;
	for(int i=0;i<m_ArraySize;i++)
	{
		result=fscanf(f,"%d",&direction);
		if(result == 0 || result == EOF)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MagicTW", "fail to load : corrupted file");
			fclose(f);
			return -1;
		}
		
		if(direction==9)
		{
			m_TabInputData[i].m_Direction=9;
		}
		else
		{
			m_TabInputData[i].m_Direction=direction;

			result=fscanf(f,"%d %d %d %d %d %d %d %d %d\n",
				&(m_TabInputData[i].m_TargetX),
				&(m_TabInputData[i].m_TargetY),
				&(m_TabInputData[i].m_Jump),
				&(m_TabInputData[i].m_Fire),
				&(m_TabInputData[i].m_Hook),
				&(m_TabInputData[i].m_PlayerFlags),
				&(m_TabInputData[i].m_WantedWeapon),
				&(m_TabInputData[i].m_NextWeapon),
				&(m_TabInputData[i].m_PrevWeapon));

			if(result == 0 || result == EOF)
			{
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "MagicTW", "fail to load : corrupted file");
				fclose(f);
				return -1;
			}
		}
	}
	
	fclose(f);
	return 0;
}

void CControls::DisplayInputStateInConsole(int i)
{
	char s[40],unicode[4];
	CLineInput::GetUnicode(unicode,"high_voltage");
	s[0]='\0';

	if(i==0)
	{
		str_append(s,"echo MagicTW_record_input ",40);
		str_append(s,unicode,40);
		if(m_RecordInput)
			str_append(s,"ON",40);
		else
			str_append(s,"OFF",40);
	}
	else
	{
		str_append(s,"echo MagicTW_load_input ",40);
		str_append(s,unicode,40);
		if(m_LoadInput)
			str_append(s,"ON",40);
		else
			str_append(s,"OFF",40);
	}
	str_append(s,unicode,40);
	Console()->ExecuteLine(s);
}
