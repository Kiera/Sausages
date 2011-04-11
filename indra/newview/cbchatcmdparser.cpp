#include "llviewerprecompiledheaders.h"

#include "cbchatcmdparser.h" //include self

//other library includes
#include <istream>
//newview includes
#include "llchat.h"
#include "llfloaterchat.h"
#include "llviewercontrol.h"
#include "LunaLua.h"

ChatCmdParser::ChatCmdParser()
{
}

BOOL ChatCmdParser::parse(const std::string& input){
	static BOOL* sChatCmdEnabled = rebind_llcontrol<BOOL>("ChatCmdEnabled", &gSavedSettings, 1);
	if(!(*sChatCmdEnabled))
		return FALSE;
	//setup gSavedSetting statics here
	static std::string* sChatCmdLua = rebind_llcontrol<std::string>("ChatCmdLua", &gSavedSettings, 1);
	static std::string* sChatCmdLuaMacro = rebind_llcontrol<std::string>("ChatCmdLuaMacro", &gSavedSettings, 1);
	static std::string* sChatCmdLuaMacroShort = rebind_llcontrol<std::string>("ChatCmdLuaMacroShort", &gSavedSettings, 1);
	std::istringstream stream(input);
	std::string cmd;
	if(stream >> cmd)
	{
		if(cmd == (*sChatCmdLua))
		{
			FLLua::callCommand(input.substr(cmd.length()));
			return TRUE;
		}
		else if(cmd == (*sChatCmdLuaMacroShort) || cmd == (*sChatCmdLuaMacro))
		{
			FLLua::callCommand(input);
			return TRUE;
		}
	}
	return FALSE;
}