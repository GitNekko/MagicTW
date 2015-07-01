/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/keys.h>
#include "lineinput.h"

// MagicTW - Debug
#include <stdio.h>

CLineInput::CLineInput()
{
	Clear();
}

void CLineInput::Clear()
{
	mem_zero(m_Str, sizeof(m_Str));
	m_Len = 0;
	m_CursorPos = 0;
	m_NumChars = 0;
}

void CLineInput::Set(const char *pString)
{
	str_copy(m_Str, pString, sizeof(m_Str));
	m_Len = str_length(m_Str);
	m_CursorPos = m_Len;
	m_NumChars = 0;
	int Offset = 0;
	while(pString[Offset])
	{
		Offset = str_utf8_forward(pString, Offset);
		++m_NumChars;
	}
}

bool CLineInput::Manipulate(IInput::CEvent e, char *pStr, int StrMaxSize, int StrMaxChars, int *pStrLenPtr, int *pCursorPosPtr, int *pNumCharsPtr)
{
	int NumChars = *pNumCharsPtr;
	int CursorPos = *pCursorPosPtr;
	int Len = *pStrLenPtr;
	bool Changes = false;

	if(CursorPos > Len)
		CursorPos = Len;

	int Code = e.m_Unicode;
	int k = e.m_Key;

	// 127 is produced on Mac OS X and corresponds to the delete key
	if (!(Code >= 0 && Code < 32) && Code != 127)
	{
		char Tmp[8];
		int CharSize = str_utf8_encode(Tmp, Code);

		if (Len < StrMaxSize - CharSize && CursorPos < StrMaxSize - CharSize && NumChars < StrMaxChars)
		{
			mem_move(pStr + CursorPos + CharSize, pStr + CursorPos, Len-CursorPos+1); // +1 == null term
			for(int i = 0; i < CharSize; i++)
				pStr[CursorPos+i] = Tmp[i];
			CursorPos += CharSize;
			Len += CharSize;
			if(CharSize > 0)
				++NumChars;
			Changes = true;
		}
	}

	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		if (k == KEY_BACKSPACE && CursorPos > 0)
		{
			int NewCursorPos = str_utf8_rewind(pStr, CursorPos);
			int CharSize = CursorPos-NewCursorPos;
			mem_move(pStr+NewCursorPos, pStr+CursorPos, Len - NewCursorPos - CharSize + 1); // +1 == null term
			CursorPos = NewCursorPos;
			Len -= CharSize;
			if(CharSize > 0)
				--NumChars;
			Changes = true;
		}
		else if (k == KEY_DELETE && CursorPos < Len)
		{
			int p = str_utf8_forward(pStr, CursorPos);
			int CharSize = p-CursorPos;
			mem_move(pStr + CursorPos, pStr + CursorPos + CharSize, Len - CursorPos - CharSize + 1); // +1 == null term
			Len -= CharSize;
			if(CharSize > 0)
				--NumChars;
			Changes = true;
		}
		else if (k == KEY_LEFT && CursorPos > 0)
			CursorPos = str_utf8_rewind(pStr, CursorPos);
		else if (k == KEY_RIGHT && CursorPos < Len)
			CursorPos = str_utf8_forward(pStr, CursorPos);
		else if (k == KEY_HOME)
			CursorPos = 0;
		else if (k == KEY_END)
			CursorPos = Len;
	}

	*pNumCharsPtr = NumChars;
	*pCursorPosPtr = CursorPos;
	*pStrLenPtr = Len;

	return Changes;
}

void CLineInput::ProcessInput(IInput::CEvent e)
{
	Manipulate(e, m_Str, MAX_SIZE, MAX_CHARS, &m_Len, &m_CursorPos, &m_NumChars);
}

// MagicTW
void CLineInput::AddString(const char *pString)
{
	char buf[MAX_SIZE];
	str_copy(buf,m_Str+m_CursorPos,MAX_SIZE);
	m_Str[m_CursorPos]='\0';
	str_append(m_Str,pString,MAX_SIZE);
	str_append(m_Str,buf,MAX_SIZE);
	m_Len=str_length(m_Str);
	// Reset cursor to the end
	m_CursorPos = m_Len;
	int offset = 0;
	while(pString[offset])
	{
		offset = str_utf8_forward(pString, offset);
		++m_NumChars;
	}
}

void CLineInput::GetUnicode(char *pString,const char *pSymbol)
{
	pString[0]=(char)-30;

	if(!str_comp_nocase(pSymbol,"heart"))
	{
		pString[1]=(char)-103;
		pString[2]=(char)-91;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"round_heart"))
	{
		pString[1]=(char)-99;
		pString[2]=(char)-92;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"empty_heart"))
	{
		pString[1]=(char)-103;
		pString[2]=(char)-95;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"star"))
	{
		pString[1]=(char)-104;
		pString[2]=(char)-122;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"3_stars"))
	{
		pString[1]=(char)-127;
		pString[2]=(char)-126;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"club"))
	{
		pString[1]=(char)-104;
		pString[2]=(char)-104;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"shakti"))
	{
		pString[1]=(char)-104;
		pString[2]=(char)-84;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"yin_yang"))
	{
		pString[1]=(char)-104;
		pString[2]=(char)-81;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"peace"))
	{
		pString[1]=(char)-104;
		pString[2]=(char)-82;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"skull"))
	{
		pString[1]=(char)-104;
		pString[2]=(char)-96;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"radioactive"))
	{
		pString[1]=(char)-104;
		pString[2]=(char)-94;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"biohazard"))
	{
		pString[1]=(char)-104;
		pString[2]=(char)-93;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"black_smile"))
	{
		pString[1]=(char)-104;
		pString[2]=(char)-69;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"white_smile"))
	{
		pString[1]=(char)-104;
		pString[2]=(char)-70;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"note"))
	{
		pString[1]=(char)-103;
		pString[2]=(char)-85;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"recycling"))
	{
		pString[1]=(char)-103;
		pString[2]=(char)-78;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"east_syriac_cross"))
	{
		pString[1]=(char)-103;
		pString[2]=(char)-79;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"west_syriac_cross"))
	{
		pString[1]=(char)-103;
		pString[2]=(char)-80;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"wheelchair"))
	{
		pString[1]=(char)-103;
		pString[2]=(char)-65;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"medicine"))
	{
		pString[1]=(char)-102;
		pString[2]=(char)-107;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"atom"))
	{
		pString[1]=(char)-102;
		pString[2]=(char)-101;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"high_voltage"))
	{
		pString[1]=(char)-102;
		pString[2]=(char)-95;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"scissors"))
	{
		pString[1]=(char)-100;
		pString[2]=(char)-126;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"telephone"))
	{
		pString[1]=(char)-100;
		pString[2]=(char)-122;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"airplane"))
	{
		pString[1]=(char)-100;
		pString[2]=(char)-120;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"pencil"))
	{
		pString[1]=(char)-100;
		pString[2]=(char)-114;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"8_pointed_star"))
	{
		pString[1]=(char)-100;
		pString[2]=(char)-75;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"circle_arrow"))
	{
		pString[1]=(char)-97;
		pString[2]=(char)-77;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"diamond"))
	{
		pString[1]=(char)-103;
		pString[2]=(char)-90;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"spade"))
	{
		pString[1]=(char)-103;
		pString[2]=(char)-96;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"right_arrow"))
	{
		pString[1]=(char)-98;
		pString[2]=(char)-95;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"up_arrow"))
	{
		pString[1]=(char)-84;
		pString[2]=(char)-122;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"left_arrow"))
	{
		pString[1]=(char)-84;
		pString[2]=(char)-123;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"down_arrow"))
	{
		pString[1]=(char)-84;
		pString[2]=(char)-121;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"up_right_arrow"))
	{
		pString[1]=(char)-84;
		pString[2]=(char)-120;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"up_left_arrow"))
	{
		pString[1]=(char)-84;
		pString[2]=(char)-119;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"down_left_arrow"))
	{
		pString[1]=(char)-84;
		pString[2]=(char)-117;
		pString[3]='\0';
	}
	else if(!str_comp_nocase(pSymbol,"down_right_arrow"))
	{
		pString[1]=(char)-84;
		pString[2]=(char)-118;
		pString[3]='\0';
	}
/*
	else if(!str_comp_nocase(pSymbol,""))
	{
		pString[1]=(char)-;
		pString[2]=(char)-;
		pString[3]='\0';
	}*/
	else
		pString[0]='\0';
}

