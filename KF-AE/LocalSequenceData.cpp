/********************************************************************************************
Local Sequence Data

Author: Adam Sakareassen

Credits: "Key Frame Movie Maker"  from the "Kalles Fraktaler" project was consulted for the 
         .kfb file format details.

Contains data that is relevent for the current sequence (ie the specific instance of the plug-in)
Contains data that is not saved within the AE Project file.
********************************************************************************************/
#include "LocalSequenceData.h"
#include "KFMovieMaker.h"
#include "OS.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::experimental::filesystem;

/*******************************************************************************************************
Constructor
*******************************************************************************************************/
LocalSequenceData::LocalSequenceData()
{
}

/*******************************************************************************************************
Setup the file date for rendering.  
Input is a kfr file.  The kfb files in the same directory will provide the data.
fileName may be an empty string, in which case it will clear the data.
Note: It is quite possible that the file does not exist (eg. loading an old project)
*******************************************************************************************************/
void LocalSequenceData::SetupFileData(const std::string & fileName)
{
	this->clear();
	if (fileName.empty()) return;
	this->kfrFileName = fileName;
	if (!fs::exists(fileName)) throw (std::exception("KFR file not found \n"));
	this->readKFRfile();
	this->getKFBlist();
	this->getKFBStats();
	if (this->width == 0 || this->height == 0) return;
	this->readyToRender = true;
}



/*******************************************************************************************************
Clear data for this class.
*******************************************************************************************************/
void LocalSequenceData::clear()
{
	this->readyToRender = false;
	this->kfrFileName.clear();
	this->kfbFiles.clear();
	this->width = 0;
	this->height = 0;
	
	this->activeFrameNumber = -1;
	this->activeKFB = nullptr;
	this->nextFrameNumber = -1;
	this->nextFrameKFB = nullptr;
	this->kfrColours.fill(RGB(0,0,0));

}

/*******************************************************************************************************
Reads the colour and iteration division information from the .kfr file.
*******************************************************************************************************/
void LocalSequenceData::readKFRfile() {
	std::ifstream file {this->kfrFileName};
	if(!file) throw (std::exception("Unable to open KFR file\n"));

	std::string line;
	while(!file.eof()) {
		std::getline(file,line);
		auto ss = std::istringstream(line);
		std::string lineHeader;
		ss >> lineHeader;
				
		if(lineHeader == "IterDiv:") ss >> this->kfrIterationDivision;
		if(lineHeader == "Colors:") {
			int cIndex = 0;
			while(!ss.eof() && cIndex < kfrColours.size()) {
				int red = 0, green = 0, blue = 0;
				char c1 = 0, c2 = 0, c3 = 0;
				ss >> blue >> c1 >> green >>c2 >> red >> c3;
				if(!ss.eof() && c1==',' && c2==',' && c3==',') {
					kfrColours[cIndex] = RGB(red, green, blue);
					cIndex++;
				}
			}
			numKFRColours = cIndex;
		}

	}

}


/*******************************************************************************************************
Get a list of all the KFB files in the same directory as the kfr file.
*******************************************************************************************************/
void LocalSequenceData::getKFBlist()
{
	auto path = fs::path(this->kfrFileName).parent_path();
	for (const auto & entry : fs::directory_iterator(path)) {
		if (entry.path().extension().compare(".kfb") == 0) {
			this->kfbFiles.push_back(entry.path().string());
		}
	}
	std::sort(this->kfbFiles.rbegin(), this->kfbFiles.rend());
}

/*******************************************************************************************************

*******************************************************************************************************/
void LocalSequenceData::getKFBStats() {
	if(this->kfbFiles.empty()) return;
	auto fileName = this->kfbFiles[0];
	if(!fs::exists(fileName)) throw (std::exception("KFB file missing\n"));

	//Readfile
	std::ifstream file {fileName, std::ios::binary | std::ios::in};

	if(!file) throw (std::exception("Unable to open KFB file\n"));

	//Check ID
	char id[3];
	file.read(id, 3);
	if(!(id[0] == 'K' && id[1] == 'F' && id[2] == 'B')) throw (std::exception("KFB file has invalid ID\n"));


	//Read Size
	file.read((char *) &this->width, sizeof(width));
	file.read((char *) &this->height, sizeof(height));

}

/*******************************************************************************************************

*******************************************************************************************************/
void LocalSequenceData::SetupActiveKFB(long keyFrame, PF_InData *in_data) {
	if(!readyToRender) return;
	if(activeFrameNumber == keyFrame) return;
	
	if(nextFrameNumber == keyFrame) {
		this->activeKFB = this->nextFrameKFB;
		this->activeFrameNumber = keyFrame;
	}else{ 
		auto data = std::make_shared<KFBData>(this->width, this->height);
		LoadKFB(data, keyFrame);
		activeFrameNumber = keyFrame;
		this->activeKFB = data;
	}

	//Setup next frame
	this->nextFrameNumber = -1;
	this->nextFrameKFB = nullptr;

	if(keyFrame + 1 < this->kfbFiles.size() ) {
		auto data = std::make_shared<KFBData>(this->width, this->height);
		LoadKFB(data, keyFrame + 1);
		nextFrameNumber = keyFrame  + 1;
		this->nextFrameKFB = data;
	}
	else {
		this->nextFrameKFB = nullptr;
		this->nextFrameNumber = -1;

	}
	
	
}

/*******************************************************************************************************

*******************************************************************************************************/
void LocalSequenceData::LoadKFB(std::shared_ptr<KFBData> & data, long keyFrame) {
	if(keyFrame >= this->kfbFiles.size()) throw(std::exception("Invalid keyFrame requested in LoadKFB()"));
	DebugMessage("Reading KFB File:"); DebugMessage(this->kfbFiles[keyFrame]); DebugMessage("\n");

	auto fileName = this->kfbFiles[keyFrame];
	data->ReadKFBFile(fileName);
}

 