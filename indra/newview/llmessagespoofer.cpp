#include "llviewerprecompiledheaders.h"
#include "llmessagespoofer.h"
#include "llfloaterchat.h"
#include "llchat.h"
#include "llmessagetemplate.h"
#include "lltemplatemessagebuilder.h"
#include "lltemplatemessagereader.h"
#include "llagent.h"

LLMessageSpoofer::LLMessageSpoofer()
{
}

void LLMessageSpoofer::spoofMessage(const LLHost& region_host, const std::string& str_message)
{
	std::vector<std::string> lines = split(str_message, "\n");
	if(!lines.size())
	{
		LLFloaterChat::addChat(LLChat("Not enough information :O"));
		return;
	}
	std::vector<std::string> tokens = split(lines[0], " ");
	if(!tokens.size())
	{
		LLFloaterChat::addChat(LLChat("Not enough information :O"));
		return;
	}
	std::string dir_str = tokens[0];
	LLStringUtil::toLower(dir_str);
	// Direction
	BOOL outgoing;
	if(dir_str == "out")
		outgoing = TRUE;
	else if(dir_str == "in")
		outgoing = FALSE;
	else
	{
		LLFloaterChat::addChat(LLChat("Expected direction 'in' or 'out'"));
		return;
	}
	// Message
	std::string message = "Invalid";
	if(tokens.size() > 1)
	{
		if(tokens.size() > 2)
		{
			LLFloaterChat::addChat(LLChat("Unexpected extra stuff at the top"));
			return;
		}
		message = tokens[1];
		LLStringUtil::trim(message);
	}
	// Body
	std::vector<parts_block> parts;
	if(lines.size() > 1)
	{
		std::vector<std::string>::iterator line_end = lines.end();
		std::vector<std::string>::iterator line_iter = lines.begin();
		++line_iter;
		std::string current_block("");
		int current_block_index = -1;
		for( ; line_iter != line_end; ++line_iter)
		{
			std::string line = (*line_iter);
			LLStringUtil::trim(line);
			if(!line.length())
				continue;
			if(line.substr(0, 1) == "[" && line.substr(line.size() - 1, 1) == "]")
			{
				current_block = line.substr(1, line.length() - 2);
				LLStringUtil::trim(current_block);
				++current_block_index;
				parts_block pb;
				pb.name = current_block;
				parts.push_back(pb);
			}
			else
			{
				if(current_block.empty())
				{
					LLFloaterChat::addChat(LLChat("Unexpected field when no block yet"));
					return;
				}
				int eqpos = line.find("=");
				if(eqpos == line.npos)
				{
					LLFloaterChat::addChat(LLChat("Missing an equal sign"));
					return;
				}
				std::string field = line.substr(0, eqpos);
				LLStringUtil::trim(field);
				if(!field.length())
				{
					LLFloaterChat::addChat(LLChat("Missing name of field"));
					return;
				}
				std::string value = line.substr(eqpos + 1);
				LLStringUtil::trim(value);
				parts_var pv;
				if(value.substr(0, 1) == "|")
				{
					pv.hex = TRUE;
					value = value.substr(1);
					LLStringUtil::trim(value);
				}
				else
					pv.hex = FALSE;
				pv.name = field;
				pv.value = value;
				parts[current_block_index].vars.push_back(pv);
			}
		}
	}
	// Verification
	std::map<const char *, LLMessageTemplate*>::iterator template_iter;
	template_iter = gMessageSystem->mMessageTemplates.find( LLMessageStringTable::getInstance()->getString(message.c_str()) );
	if(template_iter == gMessageSystem->mMessageTemplates.end())
	{
		LLFloaterChat::addChat(LLChat(llformat("Don't know how to build a '%s' message", message.c_str())));
		return;
	}
	LLMessageTemplate* temp = (*template_iter).second;
	std::vector<parts_block>::iterator parts_end = parts.end();
	std::vector<parts_block>::iterator parts_iter = parts.begin();
	LLMessageTemplate::message_block_map_t::iterator blocks_end = temp->mMemberBlocks.end();
	for (LLMessageTemplate::message_block_map_t::iterator blocks_iter = temp->mMemberBlocks.begin();
		 blocks_iter != blocks_end; )
	{
		LLMessageBlock* block = (*blocks_iter);
		const char* block_name = block->mName;
		if(parts_iter == parts_end)
		{
			if(block->mType != MBT_VARIABLE)
				LLFloaterChat::addChat(LLChat(llformat("Expected '%s' block", block_name)));
			else
			{
				++blocks_iter;
				continue;
			}
			return;
		}
		else if((*parts_iter).name != block_name)
		{
			if(block->mType != MBT_VARIABLE)
				LLFloaterChat::addChat(LLChat(llformat("Expected '%s' block", block_name)));
			else
			{
				++blocks_iter;
				continue;
			}
			return;
		}
		std::vector<parts_var>::iterator part_var_end = (*parts_iter).vars.end();
		std::vector<parts_var>::iterator part_var_iter = (*parts_iter).vars.begin();
		LLMessageBlock::message_variable_map_t::iterator var_end = block->mMemberVariables.end();
		for (LLMessageBlock::message_variable_map_t::iterator var_iter = block->mMemberVariables.begin();
			 var_iter != var_end; ++var_iter)
		{
			LLMessageVariable* variable = (*var_iter);
			const char* var_name = variable->getName();
			if(part_var_iter == part_var_end)
			{
				LLFloaterChat::addChat(LLChat(llformat("Expected '%s' field under '%s' block", var_name, block_name)));
				return;
			}
			else if((*part_var_iter).name != var_name)
			{
				LLFloaterChat::addChat(LLChat(llformat("Expected '%s' field under '%s' block", var_name, block_name)));
				return;
			}
			(*part_var_iter).var_type = variable->getType();
			++part_var_iter;
		}
		if(part_var_iter != part_var_end)
		{
			LLFloaterChat::addChat(LLChat(llformat("Unexpected field(s) at end of '%s' block", block_name)));
			return;
		}
		++parts_iter;
		// test
		if((block->mType != MBT_SINGLE) && (parts_iter != parts_end) && ((*parts_iter).name == block_name))
		{
			// block will repeat
		}
		else ++blocks_iter;
	}
	if(parts_iter != parts_end)
	{
		LLFloaterChat::addChat(LLChat("Unexpected block(s) at end"));
		return;
	}
	// Build and send
	gMessageSystem->newMessage( message.c_str() );
	for(parts_iter = parts.begin(); parts_iter != parts_end; ++parts_iter)
	{
		const char* block_name = (*parts_iter).name.c_str();
		gMessageSystem->nextBlock(block_name);
		std::vector<parts_var>::iterator part_var_end = (*parts_iter).vars.end();
		for(std::vector<parts_var>::iterator part_var_iter = (*parts_iter).vars.begin();
			part_var_iter != part_var_end; ++part_var_iter)
		{
			parts_var pv = (*part_var_iter);
			if(!addField(pv.var_type, pv.name.c_str(), pv.value, pv.hex))
			{
				LLFloaterChat::addChat(LLChat(llformat("Error adding the provided data for %s '%s' to '%s' block", mvtstr(pv.var_type).c_str(), pv.name.c_str(), block_name)));
				gMessageSystem->clearMessage();
				return;
			}
		}
	}


	if(outgoing)
		gMessageSystem->sendMessage(region_host);
	else
	{
		U8 builtMessageBuffer[MAX_BUFFER_SIZE];

		S32 message_size = gMessageSystem->mTemplateMessageBuilder->buildMessage(builtMessageBuffer, MAX_BUFFER_SIZE, 0);
		gMessageSystem->clearMessage();
		gMessageSystem->checkMessages(0, true, builtMessageBuffer, region_host, message_size);

	}

	return;
}



std::string LLMessageSpoofer::mvtstr(e_message_variable_type var_type)
{
	switch(var_type)
	{
	case MVT_U8:
		return "U8";
		break;
	case MVT_U16:
		return "U16";
		break;
	case MVT_U32:
		return "U32";
		break;
	case MVT_U64:
		return "U64";
		break;
	case MVT_S8:
		return "S8";
		break;
	case MVT_S16:
		return "S16";
		break;
	case MVT_S32:
		return "S32";
		break;
	case MVT_S64:
		return "S64";
		break;
	case MVT_F32:
		return "F32";
		break;
	case MVT_F64:
		return "F64";
		break;
	case MVT_LLVector3:
		return "LLVector3";
		break;
	case MVT_LLVector3d:
		return "LLVector3d";
		break;
	case MVT_LLVector4:
		return "LLVector4";
		break;
	case MVT_LLQuaternion:
		return "LLQuaternion";
		break;
	case MVT_LLUUID:
		return "LLUUID";
		break;
	case MVT_BOOL:
		return "BOOL";
		break;
	case MVT_IP_ADDR:
		return "IPADDR";
		break;
	case MVT_IP_PORT:
		return "IPPORT";
		break;
	case MVT_VARIABLE:
		return "Variable";
		break;
	case MVT_FIXED:
		return "Fixed";
		break;
	default:
		return "Missingno.";
		break;
	}
}
// static
BOOL LLMessageSpoofer::addField(e_message_variable_type var_type, const char* var_name, std::string input, BOOL hex)
{
	LLStringUtil::trim(input);
	if(input.length() < 1 && var_type != MVT_VARIABLE)
		return FALSE;
	U8 valueU8;
	U16 valueU16;
	U32 valueU32;
	U64 valueU64;
	S8 valueS8;
	S16 valueS16;
	S32 valueS32;
	// S64 valueS64;
	F32 valueF32;
	F64 valueF64;
	LLVector3 valueVector3;
	LLVector3d valueVector3d;
	LLVector4 valueVector4;
	LLQuaternion valueQuaternion;
	LLUUID valueLLUUID;
	BOOL valueBOOL;
	std::string input_lower = input;
	LLStringUtil::toLower(input_lower);
	if(input_lower == "$agentid")
		input = gAgent.getID().asString();
	else if(input_lower == "$sessionid")
		input = gAgent.getSessionID().asString();
	else if(input_lower == "$uuid")
	{
		LLUUID id;
		id.generate();
		input = id.asString();
	}
	else if(input_lower == "$circuitcode")
	{
		std::stringstream temp_stream;
		temp_stream << gMessageSystem->mOurCircuitCode;
		input = temp_stream.str();
	}
	else if(input_lower == "$regionhandle")
	{
		std::stringstream temp_stream;
		temp_stream << (gAgent.getRegion() ? gAgent.getRegion()->getHandle() : 0);
		input = temp_stream.str();
	}
	else if(input_lower == "$position" || input_lower == "$pos")
	{
		std::stringstream temp_stream;
		valueVector3 = gAgent.getPositionAgent();
		temp_stream << "<" << valueVector3[0] << ", " << valueVector3[1] << ", " << valueVector3[2] << ">";
		input = temp_stream.str();
	}
	if(hex)
	{
		if(var_type != MVT_VARIABLE && var_type != MVT_FIXED)
			return FALSE;
		int len = input_lower.length();
		const char* cstr = input_lower.c_str();
		std::string new_input("");
		BOOL nibble = FALSE;
		char byte = 0;
		for(int i = 0; i < len; i++)
		{
			char c = cstr[i];
			if(c >= 0x30 && c <= 0x39)
				c -= 0x30;
			else if(c >= 0x61 && c <= 0x66)
				c -= 0x57;
			else if(c != 0x20)
				return FALSE;
			else
				continue;
			if(!nibble)
				byte = c << 4;
			else
				new_input.push_back(byte | c);
			nibble = !nibble;
		}
		if(nibble)
			return FALSE;
		input = new_input;
	}
	std::stringstream stream(input);
	std::vector<std::string> tokens;
	switch(var_type)
	{
	case MVT_U8:
		if(input.substr(0, 1) == "-")
			return FALSE;
		if((stream >> valueU32).fail())
			return FALSE;
		valueU8 = (U8)valueU32;
		gMessageSystem->addU8(var_name, valueU8);
		return TRUE;
		break;
	case MVT_U16:
		if(input.substr(0, 1) == "-")
			return FALSE;
		if((stream >> valueU16).fail())
			return FALSE;
		gMessageSystem->addU16(var_name, valueU16);
		return TRUE;
		break;
	case MVT_U32:
		if(input.substr(0, 1) == "-")
			return FALSE;
		if((stream >> valueU32).fail())
			return FALSE;
		gMessageSystem->addU32(var_name, valueU32);
		return TRUE;
		break;
	case MVT_U64:
		if(input.substr(0, 1) == "-")
			return FALSE;
		if((stream >> valueU64).fail())
			return FALSE;
		gMessageSystem->addU64(var_name, valueU64);
		return TRUE;
		break;
	case MVT_S8:
		if((stream >> valueS8).fail())
			return FALSE;
		gMessageSystem->addS8(var_name, valueS8);
		return TRUE;
		break;
	case MVT_S16:
		if((stream >> valueS16).fail())
			return FALSE;
		gMessageSystem->addS16(var_name, valueS16);
		return TRUE;
		break;
	case MVT_S32:
		if((stream >> valueS32).fail())
			return FALSE;
		gMessageSystem->addS32(var_name, valueS32);
		return TRUE;
		break;
	/*
	case MVT_S64:
		if((stream >> valueS64).fail())
			return FALSE;
		gMessageSystem->addS64(var_name, valueS64);
		return TRUE;
		break;
	*/
	case MVT_F32:
		if((stream >> valueF32).fail())
			return FALSE;
		gMessageSystem->addF32(var_name, valueF32);
		return TRUE;
		break;
	case MVT_F64:
		if((stream >> valueF64).fail())
			return FALSE;
		gMessageSystem->addF64(var_name, valueF64);
		return TRUE;
		break;
	case MVT_LLVector3:
		LLStringUtil::trim(input);
		if(input.substr(0, 1) != "<" || input.substr(input.length() - 1, 1) != ">")
			return FALSE;
		tokens = split(input.substr(1, input.length() - 2), ",");
		if(tokens.size() != 3)
			return FALSE;
		for(int i = 0; i < 3; i++)
		{
			stream.clear();
			stream.str(tokens[i]);
			if((stream >> valueF32).fail())
				return FALSE;
			valueVector3.mV[i] = valueF32;
		}
		gMessageSystem->addVector3(var_name, valueVector3);
		return TRUE;
		break;
	case MVT_LLVector3d:
		LLStringUtil::trim(input);
		if(input.substr(0, 1) != "<" || input.substr(input.length() - 1, 1) != ">")
			return FALSE;
		tokens = split(input.substr(1, input.length() - 2), ",");
		if(tokens.size() != 3)
			return FALSE;
		for(int i = 0; i < 3; i++)
		{
			stream.clear();
			stream.str(tokens[i]);
			if((stream >> valueF64).fail())
				return FALSE;
			valueVector3d.mdV[i] = valueF64;
		}
		gMessageSystem->addVector3d(var_name, valueVector3d);
		return TRUE;
		break;
	case MVT_LLVector4:
		LLStringUtil::trim(input);
		if(input.substr(0, 1) != "<" || input.substr(input.length() - 1, 1) != ">")
			return FALSE;
		tokens = split(input.substr(1, input.length() - 2), ",");
		if(tokens.size() != 4)
			return FALSE;
		for(int i = 0; i < 4; i++)
		{
			stream.clear();
			stream.str(tokens[i]);
			if((stream >> valueF32).fail())
				return FALSE;
			valueVector4.mV[i] = valueF32;
		}
		gMessageSystem->addVector4(var_name, valueVector4);
		return TRUE;
		break;
	case MVT_LLQuaternion:
		LLStringUtil::trim(input);
		if(input.substr(0, 1) != "<" || input.substr(input.length() - 1, 1) != ">")
			return FALSE;
		tokens = split(input.substr(1, input.length() - 2), ",");
		if(tokens.size() == 3)
		{
			for(int i = 0; i < 3; i++)
			{
				stream.clear();
				stream.str(tokens[i]);
				if((stream >> valueF32).fail())
					return FALSE;
				valueVector3.mV[i] = valueF32;
			}
			valueQuaternion.unpackFromVector3(valueVector3);
		}
		else if(tokens.size() == 4)
		{
			for(int i = 0; i < 4; i++)
			{
				stream.clear();
				stream.str(tokens[i]);
				if((stream >> valueF32).fail())
					return FALSE;
				valueQuaternion.mQ[i] = valueF32;
			}
		}
		else
			return FALSE;

		gMessageSystem->addQuat(var_name, valueQuaternion);
		return TRUE;
		break;
	case MVT_LLUUID:
		if((stream >> valueLLUUID).fail())
			return FALSE;
		gMessageSystem->addUUID(var_name, valueLLUUID);
		return TRUE;
		break;
	case MVT_BOOL:
		if(input_lower == "true")
			valueBOOL = TRUE;
		else if(input_lower == "false")
			valueBOOL = FALSE;
		else if((stream >> valueBOOL).fail())
			return FALSE;
		gMessageSystem->addBOOL(var_name, valueBOOL);
		//gMessageSystem->addU8(var_name, (U8)valueBOOL);
		return TRUE;
		break;
	case MVT_IP_ADDR:
		if((stream >> valueU32).fail())
			return FALSE;
		gMessageSystem->addIPAddr(var_name, valueU32);
		return TRUE;
		break;
	case MVT_IP_PORT:
		if((stream >> valueU16).fail())
			return FALSE;
		gMessageSystem->addIPPort(var_name, valueU16);
		return TRUE;
		break;
	case MVT_VARIABLE:
		if(!hex)
		{
			char* buffer = new char[input.size() + 1];
			strncpy(buffer, input.c_str(), input.size());
			buffer[input.size()] = '\0';
			gMessageSystem->addBinaryData(var_name, buffer, input.size() + 1);
			delete[] buffer;
		}
		else
			gMessageSystem->addBinaryData(var_name, input.c_str(), input.size());
		return TRUE;
		break;
	case MVT_FIXED:
		if(!hex)
		{
			char* buffer = new char[input.size() + 1];
			strncpy(buffer, input.c_str(), input.size());
			buffer[input.size()] = '\0';
			gMessageSystem->addBinaryData(var_name, buffer, input.size());
			delete[] buffer;
		}
		else
			gMessageSystem->addBinaryData(var_name, input.c_str(), input.size());
		return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

inline std::vector<std::string> LLMessageSpoofer::split(std::string input, std::string separator)
{
	S32 size = input.length();
	char* buffer = new char[size + 1];
	strncpy(buffer, input.c_str(), size);
	buffer[size] = '\0';
	std::vector<std::string> lines;
	char* result = strtok(buffer, separator.c_str());
	while(result)
	{
		lines.push_back(result);
		result = strtok(NULL, separator.c_str());
	}
	delete[] buffer;
	return lines;
}
