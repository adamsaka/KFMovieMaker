#pragma once
/********************************************************************************************
OS Specific functions

Author: Adam Sakareassen

Description:	Contains functions that rely on Windows.
				To port to another OS implement functions in OS.h in a seperate CPP file.
				Note: No checks for Endian type are included in any files.
********************************************************************************************/



#include <string>
void DebugMessage(const std::string & str);
void ShowMessageBox(const std::string & str);
std::string ShowFileOpenDialogKFR();
