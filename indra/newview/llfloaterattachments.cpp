// <edit>
 
#include "llviewerprecompiledheaders.h"
#include "llfloaterexport.h"
#include "lluictrlfactory.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "llselectmgr.h"
#include "llscrolllistctrl.h"
#include "llchat.h"
#include "llfloaterchat.h"
#include "llfilepicker.h"
#include "llagent.h"
#include "llvoavatar.h"
#include "llvoavatardefines.h"
#include "llimportobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llwindow.h"
#include "llviewerimagelist.h"
#include "lltexturecache.h"
#include "llimage.h"
#include "llappviewer.h"
#include "llfloaterattachments.h"
#include "llworld.h"
#include "llviewerregion.h"

//half of this file is copy/pasted from llfloaterexport and I'm not unshitting it.

std::vector<LLFloaterAttachments*> LLFloaterAttachments::instances;
U32 LLFloaterAttachments::sCurrentRequestID = 1;

LLFloaterAttachments::LLFloaterAttachments()
:	LLFloater()
{
	mSelection = LLSelectMgr::getInstance()->getSelection();
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_attachments.xml");

	if(LLFloaterAttachments::instances.empty())
		gMessageSystem->addHandlerFuncFast(_PREHASH_ObjectPropertiesFamily, &processObjectPropertiesFamily);

	LLFloaterAttachments::instances.push_back(this);
}


LLFloaterAttachments::~LLFloaterAttachments()
{
	std::vector<LLFloaterAttachments*>::iterator pos = std::find(LLFloaterAttachments::instances.begin(), LLFloaterAttachments::instances.end(), this);
	if(pos != LLFloaterAttachments::instances.end())
	{
		LLFloaterAttachments::instances.erase(pos);
	}

	if(LLFloaterAttachments::instances.empty())
		gMessageSystem->delHandlerFuncFast(_PREHASH_ObjectPropertiesFamily, &processObjectPropertiesFamily);
}

BOOL LLFloaterAttachments::postBuild(void)
{
	if(!mSelection) return TRUE;
	if(mSelection->getRootObjectCount() < 1) return TRUE;

	// Older stuff

	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("attachment_list");

	std::map<LLViewerObject*, bool> avatars;

	for (LLObjectSelection::valid_root_iterator iter = mSelection->valid_root_begin();
		 iter != mSelection->valid_root_end(); iter++)
	{
		LLSelectNode* nodep = *iter;
		LLViewerObject* objectp = nodep->getObject();
		std::string objectp_id = llformat("%d", objectp->getLocalID());

		if(list->getItemIndex(objectp->getID()) == -1)
		{
			bool is_attachment = false;
			bool is_root = true;
			LLViewerObject* parentp = objectp->getSubParent();
			if(parentp)
			{
				if(!parentp->isAvatar())
				{
					// parent is a prim I guess
					is_root = false;
				}
				else
				{
					// parent is an avatar
					is_attachment = true;
					if(!avatars[parentp]) avatars[parentp] = true;
				}
			}

			bool is_prim = true;
			if(objectp->getPCode() >= LL_PCODE_APP)
			{
				is_prim = false;
			}

			bool is_avatar = objectp->isAvatar();

			
			if(is_root && is_prim && is_attachment)
			{
				LLSD element;
				element["id"] = objectp->getID();

				LLSD& check_column = element["columns"][LIST_CHECKED];
				check_column["column"] = "checked";
				check_column["type"] = "checkbox";
				check_column["value"] = true;

				LLSD& type_column = element["columns"][LIST_TYPE];
				type_column["column"] = "type";
				type_column["type"] = "icon";
				type_column["value"] = "inv_item_object.tga";

				LLSD& name_column = element["columns"][LIST_NAME];
				name_column["column"] = "name";
				if(is_attachment)
					name_column["value"] = nodep->mName + " (worn on " + utf8str_tolower(objectp->getAttachmentPointName()) + ")";
				else
					name_column["value"] = nodep->mName;

				LLSD& avatarid_column = element["columns"][LIST_AVATARID];
				avatarid_column["column"] = "avatarid";
				if(is_attachment)
					avatarid_column["value"] = parentp->getID();
				else
					avatarid_column["value"] = LLUUID::null;

				list->addElement(element, ADD_BOTTOM);

				addToPrimList(objectp);
			}
			else if(is_avatar)
			{
				if(!avatars[objectp])
				{
					avatars[objectp] = true;
				}
			}
		}
	}

	if(!avatars.empty())
	{
		std::string av_name;
		gCacheName->getFullName((*avatars.begin()).first->getID(), av_name);

		if(!av_name.empty())
		{
			//gObjectList.getUUIDFromLocal();
			this->setTitle(av_name + " attachments...");
		}
	}

	//try and find the prims for the HUD attachments
	std::map<LLViewerObject*, bool>::iterator avatar_iter = avatars.begin();
	std::map<LLViewerObject*, bool>::iterator avatars_end = avatars.end();
	for( ; avatar_iter != avatars_end; avatar_iter++)
	{
		LLViewerObject* avatar = (*avatar_iter).first;
		addAvatarStuff((LLVOAvatar*)avatar);

		mAvatarIDs.insert(avatar->getID());

		U32 avatarid = avatar->getLocalID();

		gMessageSystem->newMessageFast(_PREHASH_ObjectSelect);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, avatarid+1);
		int block_counter = 0;

		for (int i=2; i<500; ++i)
		{
			block_counter++;
			if(block_counter >= 254)
			{
				// start a new message
				gMessageSystem->sendReliable(avatar->getRegion()->getHost());
				gMessageSystem->newMessageFast(_PREHASH_ObjectSelect);
				gMessageSystem->nextBlockFast(_PREHASH_AgentData);
				gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
			}
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, i+avatarid);
		}
		gMessageSystem->sendReliable(avatar->getRegion()->getHost());

		gMessageSystem->newMessageFast(_PREHASH_ObjectDeselect);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, avatarid+1);
		block_counter = 0;
		for (int i=2; i<500; ++i)
		{
			block_counter++;
			if(block_counter >= 254)
			{
				// start a new message
				gMessageSystem->sendReliable(avatar->getRegion()->getHost());
				gMessageSystem->newMessageFast(_PREHASH_ObjectDeselect);
				gMessageSystem->nextBlockFast(_PREHASH_AgentData);
				gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
			}
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, avatarid+i);
		}
		gMessageSystem->sendReliable(avatar->getRegion()->getHost());
	}

	return TRUE;
}

void LLFloaterAttachments::addAvatarStuff(LLVOAvatar* avatarp)
{
	// Add attachments
	LLViewerObject::child_list_t child_list = avatarp->getChildren();
	for (LLViewerObject::child_list_t::iterator i = child_list.begin(); i != child_list.end(); ++i)
	{
		requestAttachmentProps(*i, avatarp);
	}
}

void LLFloaterAttachments::requestAttachmentProps(LLViewerObject* childp, LLVOAvatar* avatarp)
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("attachment_list");

	if(list->getItemIndex(childp->getID()) == -1)
	{
		LLSD element;
		element["id"] = childp->getID();

		LLSD& check_column = element["columns"][LIST_CHECKED];
		check_column["column"] = "checked";
		check_column["type"] = "checkbox";
		check_column["value"] = false;

		LLSD& type_column = element["columns"][LIST_TYPE];
		type_column["column"] = "type";
		type_column["type"] = "icon";
		type_column["value"] = "inv_item_object.tga";

		LLSD& name_column = element["columns"][LIST_NAME];
		name_column["column"] = "name";
		name_column["value"] = "Object (worn on " + utf8str_tolower(childp->getAttachmentPointName()) + ")";

		LLSD& avatarid_column = element["columns"][LIST_AVATARID];
		avatarid_column["column"] = "avatarid";
		avatarid_column["value"] = avatarp->getID();

		list->addElement(element, ADD_BOTTOM);

		addToPrimList(childp);
		//LLSelectMgr::getInstance()->selectObjectAndFamily(childp, false);
		//LLSelectMgr::getInstance()->deselectObjectAndFamily(childp, true, true);

		LLViewerObject::child_list_t select_list = childp->getChildren();
		LLViewerObject::child_list_t::iterator select_iter;
		int block_counter;

		gMessageSystem->newMessageFast(_PREHASH_ObjectSelect);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, childp->getLocalID());
		block_counter = 0;
		for (select_iter = select_list.begin(); select_iter != select_list.end(); ++select_iter)
		{
			block_counter++;
			if(block_counter >= 254)
			{
				// start a new message
				gMessageSystem->sendReliable(childp->getRegion()->getHost());
				gMessageSystem->newMessageFast(_PREHASH_ObjectSelect);
				gMessageSystem->nextBlockFast(_PREHASH_AgentData);
				gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
			}
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, (*select_iter)->getLocalID());
		}
		gMessageSystem->sendReliable(childp->getRegion()->getHost());

		gMessageSystem->newMessageFast(_PREHASH_ObjectDeselect);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, childp->getLocalID());
		block_counter = 0;
		for (select_iter = select_list.begin(); select_iter != select_list.end(); ++select_iter)
		{
			block_counter++;
			if(block_counter >= 254)
			{
				// start a new message
				gMessageSystem->sendReliable(childp->getRegion()->getHost());
				gMessageSystem->newMessageFast(_PREHASH_ObjectDeselect);
				gMessageSystem->nextBlockFast(_PREHASH_AgentData);
				gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
			}
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, (*select_iter)->getLocalID());
		}
		gMessageSystem->sendReliable(childp->getRegion()->getHost());
	}
}

void LLFloaterAttachments::addToPrimList(LLViewerObject* object)
{
	mPrimList.push_back(object->getLocalID());
	LLViewerObject::child_list_t child_list = object->getChildren();
	for (LLViewerObject::child_list_t::iterator i = child_list.begin(); i != child_list.end(); ++i)
	{
		LLViewerObject* child = *i;
		if(child->getPCode() < LL_PCODE_APP)
		{
			mPrimList.push_back(child->getLocalID());
		}
	}
}

void LLFloaterAttachments::receivePrimName(LLViewerObject* object, std::string name)
{
	LLUUID fullid = object->getID();
	U32 localid = object->getLocalID();

	if(std::find(mPrimList.begin(), mPrimList.end(), localid) != mPrimList.end())
	{
		LLScrollListCtrl* list = getChild<LLScrollListCtrl>("attachment_list");
		S32 item_index = list->getItemIndex(fullid);
		if(item_index != -1)
		{
			std::vector<LLScrollListItem*> items = list->getAllData();
			std::vector<LLScrollListItem*>::iterator iter = items.begin();
			std::vector<LLScrollListItem*>::iterator end = items.end();
			for( ; iter != end; ++iter)
			{
				if((*iter)->getUUID() == fullid)
				{
					(*iter)->getColumn(LIST_NAME)->setValue(name + " (worn on " + utf8str_tolower(object->getAttachmentPointName()) + ")");
					break;
				}
			}
		}
	}
}

void LLFloaterAttachments::receiveHUDPrimInfo(LLHUDAttachment* hud_attachment)
{
	//check that this is for an avatar we're monitoring
	if(std::find(mAvatarIDs.begin(), mAvatarIDs.end(), hud_attachment->mOwnerID) != mAvatarIDs.end())
	{
		//check that we don't have this already and that it really is a hud object
		if(mHUDAttachmentPrims.count(hud_attachment->mObjectID) == 0 && !gObjectList.findObject(hud_attachment->mObjectID))
		{
			mHUDAttachmentPrims[hud_attachment->mObjectID] = hud_attachment;

			LLSelectMgr::sObjectPropertiesFamilyRequests.insert(hud_attachment->mObjectID);

			mRootRequests[sCurrentRequestID] = hud_attachment->mObjectID;

			gMessageSystem->newMessageFast(_PREHASH_RequestObjectPropertiesFamily);

			gMessageSystem->nextBlockFast(_PREHASH_AgentData);
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			gMessageSystem->addUUID(_PREHASH_SessionID, gAgent.getSessionID());

			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_RequestFlags, sCurrentRequestID++);
			gMessageSystem->addUUID(_PREHASH_ObjectID, hud_attachment->mObjectID);

			gMessageSystem->sendReliable(gAgent.getRegionHost());
		}
	}
}

void LLFloaterAttachments::receiveHUDPrimRoot(LLHUDAttachment* hud_attachment)
{
	//there will probably be a reason for this if later. or maybe not. who knows.

	LLHUDAttachment* child = mHUDAttachmentPrims[mRootRequests[hud_attachment->mRequestID]];
	//the object requested was a root prim
	llinfos << "things" << llendl;
	if(std::find(mAvatarIDs.begin(), mAvatarIDs.end(), hud_attachment->mObjectID) != mAvatarIDs.end())
	{
		llinfos << hud_attachment->mName << " : " << child->mName << llendl;

		mHUDAttachmentHierarchy.insert(std::pair<LLUUID,LLUUID>(hud_attachment->mObjectID, mRootRequests[hud_attachment->mRequestID]));

		LLScrollListCtrl* list = getChild<LLScrollListCtrl>("attachment_list");

		if(list->getItemIndex(child->mObjectID) == -1)
		{
			LLSD element;
			element["id"] = child->mObjectID;

			LLSD& check_column = element["columns"][LIST_CHECKED];
			check_column["column"] = "checked";
			check_column["type"] = "checkbox";
			check_column["value"] = false;

			LLSD& type_column = element["columns"][LIST_TYPE];
			type_column["column"] = "type";
			type_column["type"] = "icon";
			type_column["value"] = "inv_item_object.tga";

			LLSD& name_column = element["columns"][LIST_NAME];
			name_column["column"] = "name";
			name_column["value"] = child->mName + " (worn on HUD??)";

			LLSD& avatarid_column = element["columns"][LIST_AVATARID];
			avatarid_column["column"] = "avatarid";
			avatarid_column["value"] = child->mOwnerID;

			list->addElement(element, ADD_BOTTOM);
		}
	}
	//the object requested was a child prim
	else if(mHUDAttachmentPrims.count(hud_attachment->mObjectID))
	{
		llinfos << hud_attachment->mName << " : " << child->mName << llendl;

		mHUDAttachmentHierarchy.insert(std::pair<LLUUID,LLUUID>(hud_attachment->mObjectID, mRootRequests[hud_attachment->mRequestID]));
	}
}

// static
void LLFloaterAttachments::dispatchObjectProperties(LLUUID fullid, std::string name, std::string desc)
{
	LLViewerObject* object = gObjectList.findObject(fullid);
	std::vector<LLFloaterAttachments*>::iterator iter = LLFloaterAttachments::instances.begin();
	std::vector<LLFloaterAttachments*>::iterator end = LLFloaterAttachments::instances.end();
	for( ; iter != end; ++iter)
	{
		(*iter)->receivePrimName(object, name);
	}
}

void LLFloaterAttachments::dispatchHUDObjectProperties(LLHUDAttachment* hud_attachment)
{
	std::vector<LLFloaterAttachments*>::iterator iter = LLFloaterAttachments::instances.begin();
	std::vector<LLFloaterAttachments*>::iterator end = LLFloaterAttachments::instances.end();
	for( ; iter != end; ++iter)
	{
		(*iter)->receiveHUDPrimInfo(hud_attachment);
	}
}

void LLFloaterAttachments::processObjectPropertiesFamily(LLMessageSystem* msg, void** user_data)
{
	U32 request_flags;
	LLUUID id;
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, id);

	LLUUID creator_id;
	LLUUID owner_id;
	LLUUID group_id;
	LLUUID extra_id;
	U32 base_mask, owner_mask, group_mask, everyone_mask, next_owner_mask;
	LLSaleInfo sale_info;
	LLCategory category;

	msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_RequestFlags,	request_flags );

	if(request_flags == 0x0)
	{
		return;
	}

	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_OwnerID,		owner_id );
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_GroupID,		group_id );
	msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_BaseMask,		base_mask );
	msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_OwnerMask,		owner_mask );
	msg->getU32Fast(_PREHASH_ObjectData,_PREHASH_GroupMask,		group_mask );
	msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_EveryoneMask,	everyone_mask );
	msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_NextOwnerMask, next_owner_mask);
	sale_info.unpackMessage(msg, _PREHASH_ObjectData);
	category.unpackMessage(msg, _PREHASH_ObjectData);

	LLUUID last_owner_id;
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_LastOwnerID, last_owner_id );

	// unpack name & desc
	std::string name;
	msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, name);

	std::string desc;
	msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, desc);

	// Now look through all of the hovered nodes
	struct f : public LLSelectedNodeFunctor
	{
		LLUUID mID;
		f(const LLUUID& id) : mID(id) {}
		virtual bool apply(LLSelectNode* node)
		{
			return (node->getObject() && node->getObject()->mID == mID);
		}
	} func(id);
	LLSelectNode* node = LLSelectMgr::getInstance()->getHoverObjects()->getFirstNode(&func);

	if(!node && request_flags != 0)
	{
		std::vector<LLFloaterAttachments*>::iterator iter = LLFloaterAttachments::instances.begin();
		std::vector<LLFloaterAttachments*>::iterator end = LLFloaterAttachments::instances.end();
		for( ; iter != end; ++iter)
		{
			(*iter)->receiveHUDPrimRoot(new LLHUDAttachment(name, desc, owner_id, id, LLUUID::null, std::vector<LLUUID>(), request_flags));
		}
	}
}

//this is dumb.
LLHUDAttachment::LLHUDAttachment(std::string name, std::string description, LLUUID owner_id, LLUUID object_id, LLUUID from_task_id, std::vector<LLUUID> textures, U32 request_id)
	: mName(name), mDescription(description), mOwnerID(owner_id), mObjectID(object_id), mFromTaskID(from_task_id), mTextures(textures), mRequestID(request_id)
{
}

// </edit>
