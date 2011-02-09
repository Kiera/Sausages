/**
 * @file LLFloaterTextDump.cpp
 * @brief Asset Text Editor floater made by Hazim Gazov (based on hex editor floater by Day Oh)
 * @author Hazim Gazov
 * 
 * $LicenseInfo:firstyear=2010&license=WTFPLV2$
 *  
 */

// <edit>

#include "llviewerprecompiledheaders.h"

#include "llfloatertextdump.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h" // gSavedSettings

LLFloaterTextDump::LLFloaterTextDump(std::string& title, std::string& contents)
:	LLFloater()
{
	mNewTitle = title;
	mContents = contents;
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_text_dump.xml");
}

// static
void LLFloaterTextDump::show(std::string title, std::string contents)
{
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect = gSavedSettings.getRect("FloaterTextDumpRect");
	rect.translate(left - rect.mLeft, top - rect.mTop);
	LLFloaterTextDump* floaterp = new LLFloaterTextDump(title, contents);
	floaterp->setRect(rect);
	gFloaterView->adjustToFitScreen(floaterp, FALSE);
}

LLFloaterTextDump::~LLFloaterTextDump()
{
}

void LLFloaterTextDump::close(bool app_quitting)
{
	LLFloater::close(app_quitting);
}

BOOL LLFloaterTextDump::postBuild(void)
{
	LLTextEditor* editor = getChild<LLTextEditor>("text_editor");
	mEditor = editor;


	setTitle(mNewTitle);
	mEditor->setText(mContents);

	return TRUE;
}

// </edit>
