// <edit>
#include "llviewerprecompiledheaders.h"
#include "llfloatervfs.h"
#include "lluictrlfactory.h"
#include "llscrolllistctrl.h"
#include "llcheckboxctrl.h"
#include "llfilepicker.h"
#include "lllocalinventory.h"
#include "llviewerwindow.h"
#include "llassetconverter.h"
LLFloaterVFS* LLFloaterVFS::sInstance;
std::list<LLFloaterVFS::entry> LLFloaterVFS::mFiles;
LLFloaterVFS::LLFloaterVFS()
:	LLFloater(),
	mEditID(LLUUID::null)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_vfs.xml");
}
LLFloaterVFS::~LLFloaterVFS()
{
	sInstance = NULL;
}
// static
void LLFloaterVFS::show()
{
	if(sInstance)
		sInstance->open();
	else
	{
		sInstance = new LLFloaterVFS();
		sInstance->open();
	}
}
BOOL LLFloaterVFS::postBuild()
{
	childSetAction("add_btn", onClickAdd, this);
	childSetAction("clear_btn", onClickClear, this);
	childSetAction("reload_all_btn", onClickReloadAll, this);
	childSetCommitCallback("file_list", onCommitFileList, this);
	childSetCommitCallback("name_edit", onCommitEdit, this);
	childSetCommitCallback("id_edit", onCommitEdit, this);
	childSetCommitCallback("type_combo", onCommitEdit, this);
	childSetAction("copy_uuid_btn", onClickCopyUUID, this);
	childSetAction("item_btn", onClickItem, this);
	childSetAction("reload_btn", onClickReload, this);
	childSetAction("remove_btn", onClickRemove, this);
	refresh();
	return TRUE;
}
void LLFloaterVFS::refresh()
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("file_list");
	list->clearRows();
	std::list<entry>::iterator end = mFiles.end();
	for(std::list<entry>::iterator iter = mFiles.begin(); iter != end; ++iter)
	{
		entry file = (*iter);
		LLSD element;
		element["id"] = file.mID;
		LLSD& name_column = element["columns"][0];
		name_column["column"] = "name";
		name_column["value"] = file.mName.empty() ? file.mID.asString() : file.mName;
		LLSD& type_column = element["columns"][1];
		type_column["column"] = "type";
		type_column["value"] = std::string(LLAssetType::lookup(file.mType));
		list->addElement(element, ADD_BOTTOM);
	}
	setMassEnabled(!mFiles.empty());
	setEditID(mEditID);
}
void LLFloaterVFS::add(entry file)
{
	mFiles.push_back(file);
	refresh();
}
void LLFloaterVFS::clear()
{
	std::list<entry>::iterator end = mFiles.end();
	for(std::list<entry>::iterator iter = mFiles.begin(); iter != end; ++iter)
	{
		gVFS->removeFile((*iter).mID, (*iter).mType);
	}
	mFiles.clear();
	refresh();
}
void LLFloaterVFS::reloadAll()
{
	std::list<entry>::iterator end = mFiles.end();
	for(std::list<entry>::iterator iter = mFiles.begin(); iter != end; ++iter)
	{
		entry file = (*iter);
		reloadEntry(file);
	}
	refresh();
}
void LLFloaterVFS::reloadEntry(entry file)
{
	gVFS->removeFile(file.mID, file.mType);
	std::string file_name = file.mFilename;
	S32 file_size;
	LLAPRFile fp;
	fp.open(file_name, LL_APR_RB, LLAPRFile::global, &file_size);
	if(fp.getFileHandle())
	{
		LLVFile file(gVFS, file.mFileID, file.mFileType, LLVFile::WRITE);
		file.setMaxSize(file_size);
		const S32 buf_size = 65536;
		U8 copy_buf[buf_size];
		while ((file_size = fp.read(copy_buf, buf_size)))
			file.write(copy_buf, file_size);
		fp.close();
	}
	else
	{
		// todo: show a warning, couldn't open the original file
		return;
	}
	refresh();
}
void LLFloaterVFS::setEditID(LLUUID edit_id)
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("file_list");
	bool found_in_list = (list->getItemIndex(edit_id) != -1);
	bool found_in_files = false;
	entry file;
	std::list<entry>::iterator end = mFiles.end();
	for(std::list<entry>::iterator iter = mFiles.begin(); iter != end; ++iter)
	{
		if((*iter).mID == edit_id)
		{
			found_in_files = true;
			file = (*iter);
			break;
		}
	}
	if(found_in_files && found_in_list)
	{
		mEditID = edit_id;
		list->selectByID(edit_id);
		setEditEnabled(true);
		childSetText("name_edit", file.mName);
		childSetText("id_edit", file.mID.asString());
		childSetValue("type_combo", std::string(LLAssetType::lookup(file.mType)));
	}
	else
	{
		mEditID = LLUUID::null;
		list->deselectAllItems(TRUE);
		setEditEnabled(false);
		childSetText("name_edit", std::string(""));
		childSetText("id_edit", std::string(""));
		childSetValue("type_combo", std::string("animatn"));
	}
}
LLFloaterVFS::entry LLFloaterVFS::getEditEntry()
{
	std::list<entry>::iterator end = mFiles.end();
	for(std::list<entry>::iterator iter = mFiles.begin(); iter != end; ++iter)
	{
		if((*iter).mID == mEditID)
			return (*iter);
	}
	entry file;
	file.mID = LLUUID::null;
	return file;
}
void LLFloaterVFS::commitEdit()
{
	bool found = false;
	entry file;
	std::list<entry>::iterator iter;
	std::list<entry>::iterator end = mFiles.end();
	for(iter = mFiles.begin(); iter != end; ++iter)
	{
		if((*iter).mID == mEditID)
		{
			found = true;
			file = (*iter);
			break;
		}
	}
	if(!found) return;
	entry edited_file;
	edited_file.mName = childGetValue("name_edit").asString();
	edited_file.mID = LLUUID(childGetValue("id_edit").asString());
	edited_file.mType = LLAssetType::lookup(getChild<LLComboBox>("type_combo")->getValue().asString());
	if((edited_file.mID != file.mID) || (edited_file.mType != file.mType))
	{
		gVFS->renameFile(file.mID, file.mType, edited_file.mID, edited_file.mType);
		mEditID = edited_file.mID;
	}
	(*iter) = edited_file;
	refresh();
}
void LLFloaterVFS::removeEntry()
{
	for(std::list<entry>::iterator iter = mFiles.begin(); iter != mFiles.end(); )
	{
		if((*iter).mID == mEditID)
		{
			gVFS->removeFile((*iter).mID, (*iter).mType);
			iter = mFiles.erase(iter);
		}
		else ++iter;
	}
	refresh();
}
void LLFloaterVFS::setMassEnabled(bool enabled)
{
	childSetEnabled("clear_btn", enabled);
	childSetEnabled("reload_all_btn", false); // DOESN'T WORK
}
void LLFloaterVFS::setEditEnabled(bool enabled)
{
	childSetEnabled("name_edit", enabled);
	childSetEnabled("id_edit", false); // DOESN'T WORK
	childSetEnabled("type_combo", false); // DOESN'T WORK
	childSetEnabled("copy_uuid_btn", enabled);
	childSetEnabled("item_btn", enabled);
	childSetEnabled("reload_btn", false); // DOESN'T WORK
	childSetEnabled("remove_btn", enabled);
}
// static
void LLFloaterVFS::onClickAdd(void* user_data)
{
	LLFloaterVFS* floaterp = (LLFloaterVFS*)user_data;
	if(!floaterp) return;
	LLUUID asset_id;
	LLAssetType::EType asset_type = LLAssetType::AT_NONE;
	asset_id.generate();
	LLFilePicker& file_picker = LLFilePicker::instance();
	if(file_picker.getOpenFile(LLFilePicker::FFLOAD_ALL))
	{
		std::string file_name = file_picker.getFirstFile();
		std::string temp_filename = file_name + ".tmp";
		asset_type = LLAssetConverter::convert(file_name, temp_filename);
		if(asset_type == LLAssetType::AT_NONE)
		{
			// todo: show a warning
			return;
		}
		S32 file_size;
		LLAPRFile fp;
		fp.open(temp_filename, LL_APR_RB, LLAPRFile::global, &file_size);
		if(fp.getFileHandle())
		{
			LLVFile file(gVFS, asset_id, asset_type, LLVFile::WRITE);
			file.setMaxSize(file_size);
			const S32 buf_size = 65536;
			U8 copy_buf[buf_size];
			while ((file_size = fp.read(copy_buf, buf_size)))
				file.write(copy_buf, file_size);
			fp.close();
		}
		else
		{
			if(temp_filename != file_name)
			{
				LLFile::remove(temp_filename);
			}
			// todo: show a warning, couldn't open the selected file
			return;
		}
		if(temp_filename != file_name)
		{
			LLFile::remove(temp_filename);
		}
		entry file;
		file.mFilename = file_name;
		file.mID = asset_id;
		file.mType = asset_type;
		file.mName = gDirUtilp->getBaseFileName(file_name, true);
		floaterp->add(file);
		if(floaterp->getChild<LLCheckBoxCtrl>("create_pretend_item")->get())
		{
			LLLocalInventory::addItem(file.mName, (int)file.mType, file.mID, true);
		}
	}
}
// static
void LLFloaterVFS::onClickClear(void* user_data)
{
	LLFloaterVFS* floaterp = (LLFloaterVFS*)user_data;
	if(!floaterp) return;
	floaterp->clear();
}
// static
void LLFloaterVFS::onClickReloadAll(void* user_data)
{
	LLFloaterVFS* floaterp = (LLFloaterVFS*)user_data;
	if(!floaterp) return;
	floaterp->reloadAll();
}
// static
void LLFloaterVFS::onCommitFileList(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterVFS* floaterp = (LLFloaterVFS*)user_data;
	LLScrollListCtrl* list = floaterp->getChild<LLScrollListCtrl>("file_list");
	LLUUID selected_id(LLUUID::null);
	if(list->getFirstSelected())
		selected_id = list->getFirstSelected()->getUUID();
	floaterp->setEditID(selected_id);
}
// static
void LLFloaterVFS::onCommitEdit(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterVFS* floaterp = (LLFloaterVFS*)user_data;
	floaterp->commitEdit();
}
// static
void LLFloaterVFS::onClickCopyUUID(void* user_data)
{
	LLFloaterVFS* floaterp = (LLFloaterVFS*)user_data;
	entry file = floaterp->getEditEntry();
	gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(file.mID.asString()));
}
// static
void LLFloaterVFS::onClickItem(void* user_data)
{
	LLFloaterVFS* floaterp = (LLFloaterVFS*)user_data;
	entry file = floaterp->getEditEntry();
	LLLocalInventory::addItem(file.mName, (int)file.mType, file.mID, true);
}
// static
void LLFloaterVFS::onClickReload(void* user_data)
{
	LLFloaterVFS* floaterp = (LLFloaterVFS*)user_data;
	entry file = floaterp->getEditEntry();
	floaterp->reloadEntry(file);
}
// static
void LLFloaterVFS::onClickRemove(void* user_data)
{
	LLFloaterVFS* floaterp = (LLFloaterVFS*)user_data;
	floaterp->removeEntry();
}
// </edit>
