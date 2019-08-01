#pragma once
/********************************************************************************************
Render

Author: Adam Sakareassen

Rendering Functions
********************************************************************************************/
#include "KFMovieMaker.h"
#include "Parameters.h"
#include "LocalSequenceData.h"

constexpr unsigned char black8 = 0;
constexpr unsigned char white8 = 0xff;
constexpr unsigned short black16 = 0;
constexpr unsigned short white16 = 0x8000;
constexpr float black32 = 0.0;
constexpr float white32 = 1.0;
constexpr double pi = 3.14159265358979323846;

PF_Err SmartPreRender(PF_InData * in_data, PF_OutData * out_data, PF_PreRenderExtra* preRender);
PF_Err SmartRender(PF_InData * in_data, PF_OutData * out_data,  PF_SmartRenderExtra* smartRender);
double doModifier(long modifier, double it);
void GetBlendedDistanceMatrix(float matrix[][3], const LocalSequenceData * local, A_long x, A_long y);

double GetBlendedPixelValue(const LocalSequenceData* local, A_long x, A_long y);
PF_Err NonSmartRender(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef	*output);
unsigned char roundTo8Bit(double f);
unsigned short roundTo16Bit(double f);
void GetColours(const LocalSequenceData* local, double iCount, RGB & highColour, RGB & lowColour, double & mixWeight);

PF_Err SetInsideColour8(LocalSequenceData * local, PF_Pixel8 * out);
PF_Err SetInsideColour16(LocalSequenceData * local, PF_Pixel16 * out);
PF_Err SetInsideColour32(LocalSequenceData * local, PF_Pixel32 * out);


class RenderMethod {
	public:
	virtual PF_Err Render8(void *refcon, A_long xL, A_long yL, PF_Pixel8 *inP, PF_Pixel8 *outP) = 0;
	virtual PF_Err Render16(void *refcon, A_long xL, A_long yL, PF_Pixel16 *inP, PF_Pixel16 *outP) = 0;
	virtual PF_Err Render32(void *refcon, A_long xL, A_long yL, PF_Pixel32 *inP, PF_Pixel32 *outP) = 0;
};