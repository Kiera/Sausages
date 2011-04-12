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
	static LLCachedControl<bool> sChatCmdEnabled("ChatCmdEnabled", TRUE);
	if(!sChatCmdEnabled)
		return FALSE;
	//setup gSavedSetting statics here
	static LLCachedControl<std::string> sChatCmdLua("ChatCmdLua", "/lua");
	static LLCachedControl<std::string> sChatCmdLuaMacro("ChatCmdLuaMacro", "/macro");
	static LLCachedControl<std::string> sChatCmdLuaMacroShort("ChatCmdLuaMacroShort", "/m");
	std::istringstream stream(input);
	std::string cmd;
	if(stream >> cmd)
	{
		if(cmd.compare(sChatCmdLua) == 0)
		{
			FLLua::callCommand(input.substr(cmd.length()));
			return TRUE;
		}
		else if(cmd.compare(sChatCmdLuaMacroShort) == 0 || cmd.compare(sChatCmdLuaMacro) == 0)
		{
			FLLua::callCommand(input);
			return TRUE;
		}
	}
	return FALSE;
}