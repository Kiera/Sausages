// <edit>
#ifndef LL_LLFLOATERATTACHMENTS_H
#define LL_LLFLOATERATTACHMENTS_H

#include "llfloater.h"
#include "llselectmgr.h"
#include "llvoavatar.h"
#include "llvoavatardefines.h"
#include "llfloaterexport.h"

class LLHUDAttachment
{
public:
	LLHUDAttachment(std::string name, std::string description, LLUUID owner_id, LLUUID object_id, LLUUID from_task_id, std::vector<LLUUID> textures, U32 request_id = 0);

	std::string mName;
	std::string mDescription;
	LLUUID mOwnerID;
	LLUUID mObjectID;
	LLUUID mFromTaskID;
	std::vector<LLUUID> mTextures;
	U32 mRequestID;
};

class LLFloaterAttachments
: public LLFloater
{
public:
	LLFloaterAttachments();
	BOOL postBuild(void);
	void addAvatarStuff(LLVOAvatar* avatarp);
	void updateNamesProgress();
	void receivePrimName(LLViewerObject* object, std::string name);
	void receiveHUDPrimInfo(LLHUDAttachment* hud_attachment);
	void receiveHUDPrimRoot(LLHUDAttachment* hud_attachment);
	void requestAttachmentProps(LLViewerObject* childp, LLVOAvatar* avatarp);

	static U32 sCurrentRequestID;

	std::set<LLUUID> mAvatarIDs;

	std::vector<U32> mPrimList;
	std::map<U32, LLUUID> mRootRequests;

	std::map<LLUUID, LLHUDAttachment*> mHUDAttachmentPrims;
	std::multimap<LLUUID, LLUUID> mHUDAttachmentHierarchy;

	static std::vector<LLFloaterAttachments*> instances; // for callback-type use

	static void dispatchObjectProperties(LLUUID fullid, std::string name, std::string desc);
	static void dispatchHUDObjectProperties(LLHUDAttachment* hud_attachment);

	static void processObjectPropertiesFamily(LLMessageSystem* msg, void** user_data);
	
private:
	virtual ~LLFloaterAttachments();
	void addToPrimList(LLViewerObject* object);

	enum LIST_COLUMN_ORDER
	{
		LIST_CHECKED,
		LIST_TYPE,
		LIST_NAME,
		LIST_AVATARID
	};
	
	LLObjectSelectionHandle mSelection;
};

#endif
// </edit>
