/********************************************************************************************
Render

Author: Adam Sakareassen

Rendering Functions
********************************************************************************************/
#include "Render.h"
#include "SequenceData.h"
#include "LocalSequenceData.h"
#include "OS.h"
#include "Parameters.h"
#include "Render-DarkLightWave.h"
#include "Render-KFRColouring.h"
#include "Render-KFRDistance.h"

typedef PF_Err(*PixelFunction8)(void *refcon, A_long x, A_long y, PF_Pixel8 *in, PF_Pixel8 *out);
typedef PF_Err(*PixelFunction16)(void* refcon, A_long x, A_long y, PF_Pixel16 *in, PF_Pixel16 *out);
typedef PF_Err(*PixelFunction32)(void* refcon, A_long x, A_long y, PF_Pixel32 *in, PF_Pixel32 *out);

static void setMaxOuputRectangle(PF_PreRenderExtra* preRender, long left, long right, long top, long bottom);
static void setOuputRectangle(PF_PreRenderExtra* preRender, long left, long right, long top, long bottom);
static PF_Err SetToBlack8(void *refcon, A_long xL, A_long yL, PF_Pixel8 *inP, PF_Pixel8 *outP);
static PF_Err SetToBlack16(void *refcon, A_long xL, A_long yL, PF_Pixel16 *inP, PF_Pixel16 *outP);
unsigned char roundTo8Bit(float f);
static PF_Err Pixel8Linear(void *refcon, A_long x, A_long y, PF_Pixel8 *in, PF_Pixel8 *out);


/*******************************************************************************************************
Pre Render
Gives AE the maximum output rectangle (always the size of the kfb).
Gives AE an output rectangle, based on the reqest.
Note: It is quite possible that AE will reqest negative locations, which we won't render.
*******************************************************************************************************/
PF_Err SmartPreRender(PF_InData *in_data, PF_OutData *out_data,  PF_PreRenderExtra* preRender) {
	auto sd = SequenceData::GetSequenceData(in_data);
	if(!sd) throw ("Sequence Data invalid");

	if (!sd->Validate() ) {
		setMaxOuputRectangle(preRender, 0, 0, 0, 0);
		setOuputRectangle(preRender, 0, 0, 0, 0);
		return PF_Err_NONE;
	}

	auto w = sd->getWidth();
	auto h = sd->getHeight();
	setMaxOuputRectangle(preRender, 0, w, 0, h);
	
	//Calculate extents of the requested render
	auto r = preRender->input->output_request.rect;
	if (r.top > w || r.left > h || r.bottom < 0 || r.right < 0) {
		setOuputRectangle(preRender, 0, 0, 0, 0);  //Outside area covered by our fractal
		return PF_Err_NONE;
	}
	
	setOuputRectangle(preRender, std::max(0, r.left), std::min(w, r.right), std::max(0, r.top), std::min(h, r.bottom));
	
	preRender->output->solid = true;
	return PF_Err_NONE;
}

/*******************************************************************************************************
Sets the max output rectangle in smart pre render.
*******************************************************************************************************/
inline void setMaxOuputRectangle(PF_PreRenderExtra* preRender, long left, long right, long top, long bottom) {
	preRender->output->max_result_rect.top = top;
	preRender->output->max_result_rect.bottom = bottom;
	preRender->output->max_result_rect.left = left;
	preRender->output->max_result_rect.right = right;
}
/*******************************************************************************************************
Sets the output rectangle in smart pre render.
*******************************************************************************************************/
inline void setOuputRectangle(PF_PreRenderExtra* preRender, long left, long right, long top, long bottom) {
	preRender->output->result_rect.top = top;
	preRender->output->result_rect.bottom = bottom;
	preRender->output->result_rect.left = left;
	preRender->output->result_rect.right = right;
}


PixelFunction8 selectPixelRenderFunction8(long method) {
	switch(method) {
		case 1:
			return Render_KFRColouring::Render8;
		case 2:
			return Render_KFRDistance::Render8;
		case 4:
			return Render_DarkLightWave::Render8;
		default:
			throw(std::exception("Unknown rendering method"));
	}
}

PixelFunction16 selectPixelRenderFunction16(long method) {
	switch(method) {
		case 1:
			return Render_KFRColouring::Render16;
		case 2:
			return Render_KFRDistance::Render16;
		case 4:
			return Render_DarkLightWave::Render16;
		default:
			throw(std::exception("Unknown rendering method"));
	}
}

PixelFunction32 selectPixelRenderFunction32(long method) {
	switch(method) {
		case 1:
			return Render_KFRColouring::Render32;
		case 2:
			return Render_KFRDistance::Render32;
		case 4:
			return Render_DarkLightWave::Render32;
		default:
			throw(std::exception("Unknown rendering method"));
	}
}


/*******************************************************************************************************
Actually generate the image in the output buffer.
*******************************************************************************************************/
static PF_Err GenerateImage(PF_InData *in_data, PF_SmartRenderExtra* smartRender, PF_EffectWorld* output, LocalSequenceData * local) {
	PF_Err err {PF_Err_NONE};
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	const auto bitDepth = smartRender->input->bitdepth;
	

	//Call the appropriate pixel iterator
	switch(bitDepth) {
		case 8:
			{
				auto fn = selectPixelRenderFunction8(local->method);
				err = suites.Iterate8Suite1()->iterate(in_data, 0, output->height, nullptr, nullptr, (void*)local, fn, output);
				break;
			}
		case 16:
			{
				auto fn = selectPixelRenderFunction16(local->method);
				err = suites.Iterate16Suite1()->iterate(in_data, 0, output->height, nullptr, nullptr, (void*)local, fn, output);
				break;
			}
		case 32:
			{
				auto fn = selectPixelRenderFunction32(local->method);
				err = suites.IterateFloatSuite1()->iterate(in_data, 0, output->height, nullptr, nullptr, (void*)local, fn, output);
				break;
			}
		default:
			break;
	}

	return err;

}

static PF_Err makeKFBCachedImage(std::shared_ptr<KFBData> &  kfb, PF_InData *in_data, PF_SmartRenderExtra* smartRender, LocalSequenceData * local) {
	PF_Err err {PF_Err_NONE};
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	AEFX_CLR_STRUCT(kfb->cachedImage);
	int width = static_cast<int>(kfb->getWidth() / local->scaleFactorX);
	int height = static_cast<int>(kfb->getHeight() / local->scaleFactorY);
	
	switch(smartRender->input->bitdepth) {
		
		case 8:
			err = suites.WorldSuite3()->AEGP_New(NULL, AEGP_WorldType_8, width, height,&kfb->cachedImageAEGP);
			break;
		case 16:
			err = suites.WorldSuite3()->AEGP_New(NULL, AEGP_WorldType_16, width, height, &kfb->cachedImageAEGP);
			break;
		case 32:
			err = suites.WorldSuite3()->AEGP_New(NULL, AEGP_WorldType_32, width, height, &kfb->cachedImageAEGP);
			break;
		default:
			break;
	}
	if(err) return err;
	err = suites.WorldSuite3()->AEGP_FillOutPFEffectWorld(kfb->cachedImageAEGP, &kfb->cachedImage);
	
	kfb->isImageCached = true;
	local->saveCachedParameters();

	//Draw the image
	auto backup1 = local->keyFramePercent;
	auto backup2 = local->activeZoomScale;
	auto backup3 = local->nextZoomScale;
	local->keyFramePercent = 0;
	local->activeZoomScale = 1;
	local->nextZoomScale = 0;
	err = GenerateImage(in_data, smartRender, &kfb->cachedImage , local);
	local->keyFramePercent = backup1;
	local->activeZoomScale = backup2;
	local->nextZoomScale = backup3;
	if(err) local->activeKFB->DisposeOfCache();
	
	return err;
}

static PF_Err ScaleAroundCentre(PF_InData *in_data, PF_EffectWorld* input, PF_EffectWorld* output, const PF_Rect * rect, double scale, double postScaleX, double postScaleY, double opacity) {
	PF_Err err {PF_Err_NONE};
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	//suites.WorldTransformSuite1()->copy(in_data->effect_ref, &local->activeKFB->cachedImage,output, &smartRender->input->output_request.rect , &smartRender->input->output_request.rect);
	float s = static_cast<float>(scale);
	float sX = static_cast<float>(postScaleX);
	float sY = static_cast<float>(postScaleY);
	float centreX = static_cast<float>(input->width)/2;
	float centreY = static_cast<float>(input->height) / 2;

	//Note: AE uses matrix with row & col swapped (ie, the inner brackets is a coloumn).
	const PF_FloatMatrix activeTrans = {{{s/sX, 0, 0}, {0, s/sY, 0}, {-(s/sX)*(centreX) + (centreX/sX), -(s/sY)*centreY + (centreY/sY), 1}}};

	PF_CompositeMode comp;
	AEFX_CLR_STRUCT(comp);
	comp.opacity = 255;
	comp.opacitySu = white16;
	comp.xfer = PF_Xfer_IN_FRONT; //PF_Xfer_COPY;
	
	if(opacity < 1.0) {
		comp.opacity = static_cast<unsigned char>(opacity * white8);
		comp.opacitySu = static_cast<unsigned short>(opacity * white16);
	}

	err = suites.WorldTransformSuite1()->transform_world(in_data->effect_ref, in_data->quality, 0, in_data->field, input, &comp, nullptr, &activeTrans, 1, true, rect, output);
	return err;
}


static PF_Err DoCachedImages(PF_InData *in_data, PF_SmartRenderExtra* smartRender, PF_EffectWorld* output, LocalSequenceData * local) {
	
	
	PF_Err err {PF_Err_NONE};
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	if (local->isCacheInvalid()){
		if (local->activeKFB) local->activeKFB->DisposeOfCache();
		if (local->nextFrameKFB) local->nextFrameKFB->DisposeOfCache();
	}

	
	
	if (!local->activeKFB->isImageCached){
		err = makeKFBCachedImage(local->activeKFB, in_data,  smartRender,local);
		if(err) return err;
	}

	if(local->nextFrameKFB && !local->nextFrameKFB->isImageCached) {
		auto backup = local->activeKFB;
		local->activeKFB = local->nextFrameKFB;
		err = makeKFBCachedImage(local->nextFrameKFB, in_data, smartRender, local);
		local->activeKFB = backup;
		if(err) return err;
	}
	
	
	ScaleAroundCentre(in_data, &local->activeKFB->cachedImage, output, &smartRender->input->output_request.rect , local->activeZoomScale, 1,1, 1.0);
	if(local->nextFrameKFB && local->nextFrameKFB->isImageCached) {
		ScaleAroundCentre(in_data, &local->nextFrameKFB->cachedImage, output, &smartRender->input->output_request.rect, local->nextZoomScale, 1, 1, local->keyFramePercent);
	}


	return err;
}


/*******************************************************************************************************
Smart Render
*******************************************************************************************************/
PF_Err SmartRender(PF_InData *in_data, PF_OutData *out_data, PF_SmartRenderExtra* smartRender) {
	PF_Err err {PF_Err_NONE};
	

	//Check that sequence data is ready to render
	auto sd = SequenceData::GetSequenceData(in_data);
	if(!sd) return PF_Err_INTERNAL_STRUCT_DAMAGED;
	if(!sd->Validate()) return PF_Err_NONE;
	auto local = sd->getLocalSequenceData();

	//Read parameters.
	double keyFrame = readFloatSliderParam(in_data, ParameterID::keyFrameNumber);
	keyFrame = std::min(keyFrame, static_cast<double>(local->kfbFiles.size() - 1));
	local->colourDivision = readFloatSliderParam(in_data, ParameterID::colourDivision);
	if(local->colourDivision == 0) local->colourDivision = 0.000001;
	local->method = readListParam(in_data, ParameterID::colourMethod);
	local->modifier = readListParam(in_data, ParameterID::modifier);
	local->useSmooth = readCheckBoxParam(in_data, ParameterID::smooth);
	local->scalingMode = readListParam(in_data, ParameterID::scalingMode);
	local->insideColour = readColour(in_data, ParameterID::insideColour);
	local->colourOffset = readFloatSliderParam(in_data, ParameterID::colourOffset);
	local->distanceClamp = readFloatSliderParam(in_data, ParameterID::distanceClamp);

	//Setup data for active frame, and next frame.
	long activeFrame = static_cast<long>(std::floor(keyFrame));
	local->keyFramePercent = keyFrame - activeFrame;
	local->activeZoomScale = std::exp(std::log(2) * (keyFrame-activeFrame));
	local->nextZoomScale = std::exp(std::log(2) * (keyFrame - 1 - activeFrame));
	local->SetupActiveKFB(activeFrame, in_data);
	local->scaleFactorX = static_cast<float>(in_data->downsample_x.den) / static_cast<float>(in_data->downsample_x.num);
	local->scaleFactorY = static_cast<float>(in_data->downsample_y.den) / static_cast<float>(in_data->downsample_y.num);
	local->bitDepth = smartRender->input->bitdepth;

	//Checkout Output buffer
	PF_EffectWorld* output {nullptr};
	err = smartRender->cb->checkout_output(in_data->effect_ref, &output);
	if(err != PF_Err_NONE) return err;
	
	if(local->scalingMode == 1) {
		DoCachedImages(in_data, smartRender, output, local);
	}
	else {
		GenerateImage(in_data, smartRender, output, local);
	}

	return err;

	
}

/*******************************************************************************************************
Adjust the iteration count based on the the selected modifier
*******************************************************************************************************/
double doModifier(long modifier, double it) {
	switch(modifier) {
		case 1: //Linear
			return it;
		case 2: //Square Root
			return std::sqrt(it);
		case 3: //Cubic Root
			return std::pow(std::fmax(0,it), 1.0 / 3.0);
		case 4: //Logarithm
			return std::log(std::fmax(1,it));
		default:
			return it;
	}
}




void GetBlendedDistanceMatrix(float matrix[][3], const LocalSequenceData* local, A_long x, A_long y) {
	const double halfWidth = static_cast<double>(local->width) / 2;
	const double halfHeight = static_cast<double>(local->height) / 2;
	const double xCentre = (x * local->scaleFactorX) - halfWidth;
	const double yCentre = (y * local->scaleFactorY) - halfHeight;
	double xLocation = xCentre / local->activeZoomScale + halfWidth;
	double yLocation = yCentre / local->activeZoomScale + halfHeight;
	
	
	local->activeKFB->getDistanceMatrix(matrix, static_cast<float>(xLocation), static_cast<float>(yLocation), 1/static_cast<float>(local->activeZoomScale));
	
	if(local->nextFrameKFB && local->keyFramePercent > 0.01 && local->nextZoomScale > 0) {
		xLocation = xCentre / local->nextZoomScale + halfWidth;
		yLocation = yCentre / local->nextZoomScale + halfHeight;
		bool nextInBounds = (xLocation >= 1 && yLocation >= 1 && xLocation <= local->width - 2 && yLocation <= local->height - 2);

		if(nextInBounds) {
			float next[3][3];
			local->nextFrameKFB->getDistanceMatrix(next, static_cast<float>(xLocation), static_cast<float>(yLocation), 1/static_cast<float>(local->nextZoomScale));
			const float mixWeight = static_cast<float>(local->keyFramePercent);
			for(int i = 0; i < 3; i++) for(int j = 0; j < 3; j++) {
				matrix[i][j] = (1 - mixWeight) * matrix[i][j] + next[i][j]* mixWeight;
			}
		}
	}
}


/*******************************************************************************************************
Calculates the interation count for (x,y) by blending frames.
Note: Must be thread-safe, so "LocalSequenceData" should be read-only.
*******************************************************************************************************/
double GetBlendedPixelValue(const LocalSequenceData* local, A_long x, A_long y) {
	//Calculate pixel location, and get iteration count.
	const double halfWidth = static_cast<double>(local->width) / 2;
	const double halfHeight = static_cast<double>(local->height) / 2;
	const double xCentre = (x * local->scaleFactorX) - halfWidth;
	const double yCentre = (y * local->scaleFactorY) - halfHeight;
	double xLocation = xCentre / local->activeZoomScale + halfWidth;
	double yLocation = yCentre / local->activeZoomScale + halfHeight;

	double iCount = local->activeKFB->calculateIterationCountBiCubic(static_cast<float>(xLocation), static_cast<float>(yLocation), local->useSmooth);

	

	//Get iteration count from the same location in the next frame (we overlay this image)
	if(local->nextFrameKFB && local->keyFramePercent > 0.01 && local->nextZoomScale >0) {
		xLocation = xCentre / local->nextZoomScale + halfWidth;
		yLocation = yCentre / local->nextZoomScale + halfHeight;
		bool nextInBounds = (xLocation >= 0 && yLocation >= 0 && xLocation <= local->width-1 && yLocation <= local->height-1);
		double iCountNext {0.0};
		if(nextInBounds) {
			iCountNext = local->nextFrameKFB->calculateIterationCountBiCubic(static_cast<float>(xLocation), static_cast<float>(yLocation), local->useSmooth);
			double mixWeight = local->keyFramePercent;
			iCount = iCount*(1-mixWeight) + iCountNext *(mixWeight);
		}
	}
	return iCount;
}


/*******************************************************************************************************
Render (non smart)
We won't support older versions of AE.  Just set pixels to black.
*******************************************************************************************************/
PF_Err NonSmartRender(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef	*output) {
	PF_Err err = PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	auto lines = output->extent_hint.bottom - output->extent_hint.top;
	
	if (output->world_flags & PF_WorldFlag_DEEP) {
		err = suites.Iterate8Suite1()->iterate(in_data, 0, lines, nullptr, nullptr, nullptr, SetToBlack8, output);
	}
	else {
		err = suites.Iterate16Suite1()->iterate(in_data, 0, lines, nullptr, nullptr, nullptr, SetToBlack16, output);
	}
	
	return err;
}

/*******************************************************************************************************
Sets a pixel to black (8 bit pixel).
Called by a pixel iterator.
*******************************************************************************************************/
static PF_Err SetToBlack8(void* refcon, A_long x, A_long y, PF_Pixel8* in, PF_Pixel8* out) {
	out->red = 0;
	out->blue = 0;
	out->green = 0;
	out->alpha = 0xff;
	return PF_Err_NONE;
}

/*******************************************************************************************************
Sets a pixel to black (16 bit pixel)
Called by a pixel iterator.
*******************************************************************************************************/
static PF_Err SetToBlack16(void* refcon, A_long x, A_long y, PF_Pixel16* in, PF_Pixel16* out){
	out->red = 0;
	out->blue = 0;
	out->green = 0;
	out->alpha = 0xffff;
	return PF_Err_NONE;
}

/*******************************************************************************************************
Round and clamp to an 8 bit value
*******************************************************************************************************/
unsigned char roundTo8Bit(double f) {
	auto d = std::round(f);
	int i = static_cast<int>(d);
	if(i < black8) i = black8;
	if(i > white8) i = white8;
	return i;
}

/*******************************************************************************************************
Round and clamp to an 16 bit colour value. (note: white is 32768 in After Effects, not 0xffff)
*******************************************************************************************************/
unsigned short roundTo16Bit(double f) {
	auto d = std::round(f);
	int i = static_cast<int>(d);
	if(i < black16) i = black16;
	if(i > white16) i = white16;
	return i;
}


PF_Err SetInsideColour8(LocalSequenceData* local, PF_Pixel8 * out) {
	out->alpha = white8;
	out->red = local->insideColour.red;
	out->green = local->insideColour.green;
	out->blue = local->insideColour.blue;
	return PF_Err_NONE;
}

PF_Err SetInsideColour16(LocalSequenceData* local, PF_Pixel16 * out) {
	constexpr double colourScale = static_cast<double>(white16) / static_cast<double>(white8);
	out->alpha = white16;
	out->red = roundTo16Bit(local->insideColour.red * colourScale);
	out->green = roundTo16Bit(local->insideColour.green* colourScale);
	out->blue = roundTo16Bit(local->insideColour.blue* colourScale);
	return PF_Err_NONE;
}

PF_Err SetInsideColour32(LocalSequenceData* local, PF_Pixel32 * out) {
	constexpr double colourScale = static_cast<double>(white32) / static_cast<double>(white8);
	out->alpha = white32;
	out->red = static_cast<float>(local->insideColour.red * colourScale);
	out->green = static_cast<float>(local->insideColour.green* colourScale);
	out->blue = static_cast<float>(local->insideColour.blue* colourScale);
	return PF_Err_NONE;
}

/*******************************************************************************************************
Given an iteration count, gets the colour above and below, and calculates a mixing weight.
(The mixing can then later be done at the desired precision.)
*******************************************************************************************************/
void GetColours(const LocalSequenceData* local, double iCount, RGB & highColour, RGB & lowColour, double & mixWeight) {
	auto nColours = local->numKFRColours;
	if(nColours == 0) nColours = 1;
	double rem = std::fmod(iCount, static_cast<double>(nColours));
	unsigned long lowColourIndex = static_cast<unsigned long>(std::floor(rem));
	if(lowColourIndex > nColours - 1) lowColourIndex = nColours - 1;
	unsigned long highColourIndex = lowColourIndex + 1;
	if(highColourIndex > nColours - 1) highColourIndex = 0;
	lowColour = local->kfrColours[lowColourIndex];
	highColour = local->kfrColours[highColourIndex];
	mixWeight = rem - std::floor(rem);
}