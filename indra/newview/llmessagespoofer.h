#ifndef LL_MESSAGE_SPOOFER_H
#define LL_MESSAGE_SPOOFER_H

#include <llmessagetemplate.h>

class LLMessageSpoofer
{
public:
	LLMessageSpoofer();

	void spoofMessage(const LLHost& region_host, const std::string& str_message );

private:
	std::string mvtstr(e_message_variable_type var_type);
	BOOL addField(e_message_variable_type var_type, const char* var_name, std::string input, BOOL hex);

	struct parts_var
	{
		std::string name;
		std::string value;
		BOOL hex;
		e_message_variable_type var_type;
	};
	struct parts_block
	{
		std::string name;
		std::vector<parts_var> vars;
	};

	std::vector<std::string> split(std::string input, std::string separator);
};
#endif
