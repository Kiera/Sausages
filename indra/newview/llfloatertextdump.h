/**
 * @file LLFloaterTextDump.h
 * @brief Asset Text Editor floater made by Hazim Gazov (based on hex editor floater by Day Oh)
 * @author Hazim Gazov
 * 
 * $LicenseInfo:firstyear=2010&license=WTFPLV2$
 *  
 */


#ifndef LL_LLFLOATERTEXTDUMP_H
#define LL_LLFLOATERTEXTDUMP_H

#include "llfloater.h"
#include "lltexteditor.h"
#include "llinventory.h"
#include "llviewerimage.h"

class LLFloaterTextDump
: public LLFloater
{
public:
	LLFloaterTextDump(std::string& title, std::string& contents);
	static void show(std::string title, std::string contents);
	BOOL postBuild(void);
	void close(bool app_quitting);
	LLTextEditor* mEditor;
	std::string mContents;
	std::string mNewTitle;
private:
	virtual ~LLFloaterTextDump();
};

#endif
