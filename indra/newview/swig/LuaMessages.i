%{
#include "lleasymessagesender.h"
%}

%rename (sendRawMessage) LLEasyMessageSender::luaSendRawMessage(const std::string&, const std::string);
%rename (sendRawMessage) LLEasyMessageSender::luaSendRawMessage(const std::string&);

%rename (sendMessage) LLEasyMessageSender::luaSendMessage(const std::string&);
%rename (sendMessage) LLEasyMessageSender::luaSendMessage();

%rename (newMessage) LLEasyMessageSender::luaNewMessage(const std::string&, const std::string&, bool);
%rename (newMessage) LLEasyMessageSender::luaNewMessage(const std::string&, const std::string&);

%rename (clearMessage) LLEasyMessageSender::luaClearMessage();

%rename (addBlock) LLEasyMessageSender::luaAddBlock(const std::string&);

%rename (addField) LLEasyMessageSender::luaAddField(const std::string&, const std::string&);
%rename (addHexField) LLEasyMessageSender::luaAddHexField(const std::string&, const std::string&);

%rename (MessageSender) LLEasyMessageSender;

%include "../lleasymessagesender.h"
