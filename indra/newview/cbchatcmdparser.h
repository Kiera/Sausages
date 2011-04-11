#ifndef CB_CHATCMDPARSER
#define CB_CHATCMDPARSER

#include "llmemory.h"
#include "llviewercontrol.h"
//#include <boost/signal.hpp>

class ChatCmdParser: public LLSingleton<ChatCmdParser>
{
	//friend class ChatCmdParserCommand();
public:
	ChatCmdParser();
	BOOL parse(const std::string& input); //returns TRUE if it handles the inputted command
};

#endif
