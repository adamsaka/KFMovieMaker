#pragma once
/********************************************************************************************
Sequence Data

Author: Adam Sakareassen

Contains the sequence data that will be sent to the plug-in with each call.
This data will be saved within the After Effects Project
********************************************************************************************/

#include "LocalSequenceData.h"
#include "KFMovieMaker.h"


#include <atomic>
#include <experimental\filesystem>
namespace fs = std::experimental::filesystem;


//Because sequence data needs to be flat we use a fixed length string to hold filnames
//The Windows API generally supports 260 characters, so we will make that our limit for now. 
constexpr int maximumFilePathLength{ 260 };


struct SequenceData {
private:
	char confirm[4]{ "MT" };						//For Sanity Checking
	std::atomic<bool> isFlat {true};				//Indicates if pointers are valid
	char kfrFileName[maximumFilePathLength] {0};	//Name of the KFR file
	
	LocalSequenceData * local { nullptr };			//Pointer to data (that is not saved with file).
	PF_Handle localHandle {nullptr};				//AE Memory Handle

public:
	//Deconstructor must be manually called when releasing AE owned memory
	~SequenceData();

	//Get a pointer to the SequenceData from the AE supplied in_data
	static SequenceData * GetSequenceData(PF_InData	*in_data);  

	//Validate the SequenceData.  Should be called before accessing any data.
	bool Validate();
	
	//Set the filename of the KFR file.
	void SetFileName(std::string & str);

	//After Effects Event Management
	static PF_Err SequenceSetup(PF_InData * in_data, PF_OutData * out_data);
	static PF_Err SequenceFlatten(PF_InData * in_data, PF_OutData * out_data);
	static PF_Err SequenceResetup(PF_InData * in_data, PF_OutData * out_data);
	static PF_Err SequenceSetdown(PF_InData * in_data, PF_OutData * out_data);

	int getWidth();
	int getHeight();
	LocalSequenceData * getLocalSequenceData() {return local;}

	
private:
	PF_Err Unflatten(PF_InData * in_data);


};

