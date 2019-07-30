/********************************************************************************************
OS Specific function (Windows)

Author: Adam Sakareassen

Description:	Contains functions that rely on Windows.
				Header is OS-independant OS.h
********************************************************************************************/
#include "../OS.h"

#include <Windows.h>

/*******************************************************************************************************
Write a message to the debug stream.
*******************************************************************************************************/
void DebugMessage(const std::string & str){
	OutputDebugString(str.c_str());
}

/*******************************************************************************************************
Show Windows Message Box
*******************************************************************************************************/
void ShowMessageBox(const std::string& str)
{
	MessageBox(GetActiveWindow(), str.c_str(), "Error", 0);
}

/*******************************************************************************************************
Show Windows File Open Dialog (KFR file filter on)
This function contains windows specific code.
Returns a string with the filename, or an empty string if canceled by user
*******************************************************************************************************/
std::string ShowFileOpenDialogKFR() {
	OPENFILENAME ofn;
	char fileName[512]{};
	ZeroMemory(&ofn, sizeof(ofn));
	ZeroMemory(fileName, sizeof(fileName));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetActiveWindow();
	ofn.lpstrFilter = "Kalles Fraktaler Files\0*.kfr\0\0";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.nMaxFile = sizeof(fileName);
	ofn.lpstrFile = fileName;

	//Note: The default Adobe project settings from skeleton cause this to fail.
	//Change Project Settings -> Code Generation -> Struct Alignment = 4.  Change to default.
	auto result = GetOpenFileName(&ofn);

	if (result) return std::string(ofn.lpstrFile);
	return "";
}
