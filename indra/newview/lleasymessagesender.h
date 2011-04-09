#ifndef LL_EASYMESSAGE_SENDER_H
#define LL_EASYMESSAGE_SENDER_H

#include <llmessagetemplate.h>

class LLEasyMessageSender
{
public:
	LLEasyMessageSender();

	bool sendMessage(const LLHost& region_host, const std::string& str_message );
	bool sendMessage(const std::string& region_host, const std::string &str_message);

private:

	BOOL addField(e_message_variable_type var_type, const char* var_name, std::string input, BOOL hex);

	//a key->value pair in a message
	struct parts_var
	{
		std::string name;
		std::string value;
		BOOL hex;
		e_message_variable_type var_type;
	};

	//a block containing key->value pairs
	struct parts_block
	{
		std::string name;
		std::vector<parts_var> vars;
	};

	void printError(const std::string& error);
	std::string mvtstr(e_message_variable_type var_type);

	std::vector<std::string> split(std::string input, std::string separator);
};
#endif
