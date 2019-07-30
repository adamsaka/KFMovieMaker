#pragma once
/********************************************************************************************
KFBData.h

Author: Adam Sakareassen

Credits: KeyFrameMovieMaker from the Kalles Fraktaler project was consulted for the KFB
		 file format.

Description:	Holds the contents of a .kfb file.  
				.kfb file includes iteration data, smooth data, and colour data.
				Has handels to hold (and destroy) a chached pre-rendered image of this data.
				This class will clean-up chache handles and its own memory allocations.
********************************************************************************************/
#include "KFMovieMaker.h"
#include <string>
constexpr int paddingSize = 2;		//The edge of the image is padded by this amount.			

class KFBData {
	public:
		int maxIterations				{0};			//Maximum iterations as read from kfb
		unsigned int colourDiv			{0};			//Colour division stored in kfb (note integer)
		unsigned int numColours			{0};			//Num colours as read from kfb
		RGB kfbColours[maxKFRColours];					//Colour data as read from the .kfb file
		
		bool isImageCached				{false};		//Is the image cache valid
		PF_EffectWorld cachedImage		{};				//The EffectWorld handle. pre-rendered copy of this .kfb data
		AEGP_WorldH cachedImageAEGP		{};				//The AEGP handle of the cached image
		


	private:
		PF_Handle handle				{nullptr};		//AE memory handle
		int * data						{nullptr};		//The actual iteration data
		float * smoothData				{nullptr};		//Float containing offsets for smooth shading
		
		long memSize					{0};			//Size of the data array
		long width						{0};			//Width of kfb
		long height						{0};			//Height (in AE orientation)
		long memWidth					{0};			//Width in actual memory (includes padding)
		long memHeight {0};
	public:
		KFBData(int w, int h);
		~KFBData();

		long dataSize() {return width*height * sizeof(int);}
		long getWidth() {return width;} 
		long getHeight() {return height;} 
		int * getIterationData() {return data;}
		float * getSmoothData() {return smoothData;}
		
		
		int getIterationCount(long x, long y);
		double getIterationCountSmooth(long x, long y);
		float calculateIterationCountBiCubic(float x, float y, bool smooth = true);
		float calculateIterationCountBiLinear(float x, float y);
		float calculateIterationCountBiLinearNoPad(float x, float y);
		void getDistanceMatrix(float p[][3], float x, float y, float step);
		
		
		void DisposeOfCache();
		void ReadKFBFile(std::string fileName);

	private:
		long makeIndex(long x, long y) {return  y*memWidth + x;}
		
		long calcIndexAndClamp(long x, long y) {
			x = (x < 0) ? 0 : ((x > width - 1) ? width - 1 : x);
			y = (y < 0) ? 0 : ((y > height - 1) ? height - 1 : y);
			return  y*memWidth + x;
		}

		float smoothValue(long x, long y) {	return smoothData[y*memWidth + x];}
		int iterationValue(long x, long y) { return data[y*memWidth + x]; }
		
};


