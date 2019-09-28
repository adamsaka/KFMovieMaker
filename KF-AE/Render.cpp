/********************************************************************************************
Render.cpp

Author:			(c) 2019 Adam Sakareassen

Licence:		GNU Affero General Public License

Contains smart rendering and various common functions for rendering.
Usually dispatches rendering to pixel iterator functions (defined elsewhere).
Don't edit LocalSequenceData in pixel specific functions, it must remain read only to be 
thread-safe once rendering begins.

********************************************************************************************
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
********************************************************************************************/
#include "Render.h"
#include "SequenceData.h"
#include "LocalSequenceData.h"
#include "OS.h"
#include "Parameters.h"
#include "Render-DarkLightWave.h"
#include "Render-KFRColouring.h"
#include "Render-KFRDistance.h"
#include "Render-LogSteps.h"
#include "Render-WaveOnPalette.h"
#include "Render-LogStepPalette.h"
#include "Render-Panels.h"
#include "Render-PanelsColour.h"
#include "Render-Angle.h"
#include "Render-AngleColour.h"
#include "Render-DEAndAngle.h"

#include <cmath>

//Function prototypes for pixel iterators.
typedef PF_Err(*PixelFunction8)(void *refcon, A_long x, A_long y, PF_Pixel8 *in, PF_Pixel8 *out);
typedef PF_Err(*PixelFunction16)(void* refcon, A_long x, A_long y, PF_Pixel16 *in, PF_Pixel16 *out);
typedef PF_Err(*PixelFunction32)(void* refcon, A_long x, A_long y, PF_Pixel32 *in, PF_Pixel32 *out);

static void setMaxOuputRectangle(PF_PreRenderExtra* preRender, long left, long right, long top, long bottom);
static void setOuputRectangle(PF_PreRenderExtra* preRender, long left, long right, long top, long bottom);
static PF_Err SetToBlack8(void *refcon, A_long xL, A_long yL, PF_Pixel8 *inP, PF_Pixel8 *outP);
static PF_Err SetToBlack16(void *refcon, A_long xL, A_long yL, PF_Pixel16 *inP, PF_Pixel16 *outP);
inline PixelFunction8 selectPixelRenderFunction8(long method);
inline PixelFunction16 selectPixelRenderFunction16(long method);
inline PixelFunction32 selectPixelRenderFunction32(long method);
static void GenerateImage(PF_InData *in_data, PF_SmartRenderExtra* smartRender, PF_EffectWorld* output, LocalSequenceData * local);
static void DoCachedImages(PF_InData *in_data, PF_SmartRenderExtra* smartRender, PF_EffectWorld* output, LocalSequenceData * local);
static void ScaleAroundCentre(PF_InData *in_data, PF_EffectWorld* input, PF_EffectWorld* output, const PF_Rect * rect, double scale, double postScaleX, double postScaleY, double opacity);
static void makeKFBCachedImage(std::shared_ptr<KFBData> &  kfb, PF_InData *in_data, PF_SmartRenderExtra* smartRender, LocalSequenceData * local);

enum class checkoutID {
	sampleLayer = 1
} ;





/*******************************************************************************************************
Pre Render
Gives AE the maximum output rectangle (always the size of the kfb).
Gives AE an output rectangle, based on the reqest.
Note: It is quite possible that AE will reqest negative locations, which we won't render.
*******************************************************************************************************/
PF_Err SmartPreRender(PF_InData *in_data, PF_OutData *out_data,  PF_PreRenderExtra* preRender) {
	auto sd = SequenceData::GetSequenceData(in_data);
	if(!sd) throw ("Sequence Data invalid");

	auto request = preRender->input->output_request;

	if (!sd->Validate()) {
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
	}
	else {
		setOuputRectangle(preRender, std::max(0, r.left), std::min(w, r.right), std::max(0, r.top), std::min(h, r.bottom));
	}

	//Manage layer sampling
	auto sampling = readCheckBoxParam(in_data, ParameterID::samplingOn);
	if(sampling) {
		preRender->output->solid = false;  //layer might contain alpha
		
		//Checkout sampling layer
		auto layerIndex = readLayerParamIndex(in_data, ParameterID::layerSample);
		PF_CheckoutResult checkout_result;
		auto err = preRender->cb->checkout_layer(in_data->effect_ref, layerIndex, static_cast<long>(checkoutID::sampleLayer), &request, in_data->current_time, in_data->time_step, in_data->time_scale, &checkout_result);
		if(err) throw(err);
	}else{
		preRender->output->solid = true;  
	}

	
	return PF_Err_NONE;
}

/*******************************************************************************************************
Smart Render
*******************************************************************************************************/
PF_Err SmartRender(PF_InData *in_data, PF_OutData *out_data, PF_SmartRenderExtra* smartRender) {
	PF_Err err {PF_Err_NONE};

	//Check that sequence data is ready to render, and extract localdata
	auto sd = SequenceData::GetSequenceData(in_data);
	if(!sd) return PF_Err_INTERNAL_STRUCT_DAMAGED;
	if(!sd->Validate()) return PF_Err_NONE;
	auto local = sd->getLocalSequenceData();
	if(!local) PF_Err_INTERNAL_STRUCT_DAMAGED;

	try {
		//Read parameters.
		local->overrideMinimalDistance = false;
		double keyFrame = readFloatSliderParam(in_data, ParameterID::keyFrameNumber);
		keyFrame = std::min(keyFrame, static_cast<double>(local->kfbFiles.size() - 1));
		local->colourDivision = readFloatSliderParam(in_data, ParameterID::colourDivision);
		if(local->colourDivision == 0) local->colourDivision = 0.000001;
		local->method = readListParam(in_data, ParameterID::colourMethod);
		local->modifier = readListParam(in_data, ParameterID::modifier);
		local->useSmooth = readCheckBoxParam(in_data, ParameterID::smooth);
		local->scalingMode = readListParam(in_data, ParameterID::scalingMode);
		local->insideColour = readColourParam(in_data, ParameterID::insideColour);
		double cycle = readAngleParam(in_data, ParameterID::colourCycle)*1024.0 / 360.0;
		local->colourOffset = cycle + readFloatSliderParam(in_data, ParameterID::colourOffset);
		local->distanceClamp = readFloatSliderParam(in_data, ParameterID::distanceClamp);
		local->slopesEnabled = readCheckBoxParam(in_data, ParameterID::slopesEnabled);
		if(local->slopesEnabled) {
			local->slopeShadowDepth = readFloatSliderParam(in_data, ParameterID::slopeShadowDepth);
			local->slopeStrength = readFloatSliderParam(in_data, ParameterID::slopeStrength);
			local->slopeAngle = readAngleParam(in_data, ParameterID::slopeAngle);
			const double angleRadians = local->slopeAngle * pi / 180;
			local->slopeAngleX = cos(angleRadians);
			local->slopeAngleY = sin(angleRadians);
			local->slopeMethod = readListParam(in_data, ParameterID::slopeMethod);
			if(local->slopeMethod == 2) local->overrideMinimalDistance = true;

		}
		local->sampling = readCheckBoxParam(in_data, ParameterID::samplingOn);
		local->special = readFloatSliderParam(in_data, ParameterID::special);
		

		//Setup data for active frame, and next frame.
		long activeFrame = static_cast<long>(std::floor(keyFrame));
		local->keyFramePercent = keyFrame - activeFrame;
		local->activeZoomScale = std::exp(std::log(2) * (keyFrame - activeFrame));
		local->nextZoomScale = std::exp(std::log(2) * (keyFrame - 1 - activeFrame));
		local->SetupActiveKFB(activeFrame, in_data);
		local->scaleFactorX = static_cast<float>(in_data->downsample_x.den) / static_cast<float>(in_data->downsample_x.num);
		local->scaleFactorY = static_cast<float>(in_data->downsample_y.den) / static_cast<float>(in_data->downsample_y.num);
		local->bitDepth = smartRender->input->bitdepth;
		local->in_data = in_data;

		

		//Setup sampling if we are doing that
		if(local->sampling) {
			//Checkout layer. Note: check-in/memory management for layers done by AE.
			err = smartRender->cb->checkout_layer_pixels(in_data->effect_ref, static_cast<long>(checkoutID::sampleLayer), &local->layer);
			if(err) throw (err);


			AEGP_SuiteHandler suites(in_data->pica_basicP);
			switch(local->bitDepth) {
				case 8:
					local->sample8 = suites.Sampling8Suite1();
					break;
				case 16:
					local->sample16 = suites.Sampling16Suite1();
					break;
				case 32:
					local->sample32 = suites.SamplingFloatSuite1();
					break;
				default:
					break;
			}
		}

		//Checkout Output buffer
		PF_EffectWorld* output {nullptr};
		err = smartRender->cb->checkout_output(in_data->effect_ref, &output);
		if(err != PF_Err_NONE) throw(err);

		//Actually begin rendering
		if(local->scalingMode == 1) {
			DoCachedImages(in_data, smartRender, output, local);
		}
		else {
			GenerateImage(in_data, smartRender, output, local);
		}

		local->layer = nullptr;
		local->sample8 = nullptr;
		local->sample16 = nullptr;
		local->sample32 = nullptr;
		local->in_data = nullptr;
		return err;
	}
	catch(PF_Err &thrown_err) {
		local->layer = nullptr;
		local->sample8 = nullptr;
		local->sample16 = nullptr;
		local->sample32 = nullptr;
		local->in_data = nullptr;
		local->DeleteKFBData();
		return thrown_err;
	}
	catch(std::exception ex) {
		local->layer = nullptr;
		local->sample8 = nullptr;
		local->sample16 = nullptr;
		local->sample32 = nullptr;
		local->in_data = nullptr;
		local->DeleteKFBData();
		throw(ex);
	}
	
}

/*******************************************************************************************************
A helper to set the max output rectangle in smart pre render.
*******************************************************************************************************/
inline void setMaxOuputRectangle(PF_PreRenderExtra* preRender, long left, long right, long top, long bottom) {
	preRender->output->max_result_rect.top = top;
	preRender->output->max_result_rect.bottom = bottom;
	preRender->output->max_result_rect.left = left;
	preRender->output->max_result_rect.right = right;
}

/*******************************************************************************************************
A helper to set the output rectangle in smart pre render.
*******************************************************************************************************/
inline void setOuputRectangle(PF_PreRenderExtra* preRender, long left, long right, long top, long bottom) {
	preRender->output->result_rect.top = top;
	preRender->output->result_rect.bottom = bottom;
	preRender->output->result_rect.left = left;
	preRender->output->result_rect.right = right;
}

/*******************************************************************************************************
Actually generate the image in the output buffer.
Calls a pixel iterator based on bit depth
Note: Iterators will often return errors, usually because the render is canceled.
*******************************************************************************************************/
static void GenerateImage(PF_InData *in_data, PF_SmartRenderExtra* smartRender, PF_EffectWorld* output, LocalSequenceData * local) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	switch(smartRender->input->bitdepth) {
		case 8:
			{
				auto fn = selectPixelRenderFunction8(local->method);
				auto err = suites.Iterate8Suite1()->iterate(in_data, 0, output->height, nullptr, nullptr, (void*)local, fn, output);
				if(err) throw (err);
				break;
			}
		case 16:
			{
				auto fn = selectPixelRenderFunction16(local->method);
				auto err = suites.Iterate16Suite1()->iterate(in_data, 0, output->height, nullptr, nullptr, (void*)local, fn, output);
				if(err) throw (err);
				break;
			}
		case 32:
			{
				auto fn = selectPixelRenderFunction32(local->method);
				auto err = suites.IterateFloatSuite1()->iterate(in_data, 0, output->height, nullptr, nullptr, (void*)local, fn, output);
				if(err) throw (err);
				break;
			}
		default:
			break;
	}
}

/*******************************************************************************************************
Render using the chached image method.

Note: I now render to an temporary buffer first which is twice the requested resolution.
The cached images are blended on this buffer, then it is downsampled.
AE was producing very ugly results scaling images to between 95% and 99%, creating very messy
artifacts.  Oversampling has dramatically improved the result.
In theory this probably limits the output resolution to 16k x 16k, which should be fine for now.
*******************************************************************************************************/
static void DoCachedImages(PF_InData *in_data, PF_SmartRenderExtra* smartRender, PF_EffectWorld* output, LocalSequenceData * local) {
	
	PF_Err err {PF_Err_NONE};
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	if(local->isCacheInvalid()) {
		if(local->activeKFB) local->activeKFB->DisposeOfCache();
		if(local->nextFrameKFB) local->nextFrameKFB->DisposeOfCache();
	}

	if(!local->activeKFB->isImageCached) {
		makeKFBCachedImage(local->activeKFB, in_data, smartRender, local);
			
	}

	if(local->nextFrameKFB && !local->nextFrameKFB->isImageCached) {
		auto backup = local->activeKFB;
		local->activeKFB = local->nextFrameKFB;
		makeKFBCachedImage(local->nextFrameKFB, in_data, smartRender, local);
		local->activeKFB = backup;
	}

	//Intermediate Render Stage
	double nextOpacity = local->keyFramePercent;
	
	constexpr double tempScale = 2.0;
	int width = static_cast<int>(tempScale * local->width / local->scaleFactorX);
	int height = static_cast<int>(tempScale * local->height / local->scaleFactorY);
	
	if(local->tempImageBuffer.handle && (local->tempImageBuffer.bitDepth != smartRender->input->bitdepth || local->tempImageBuffer.effectWorld.width != width || local->tempImageBuffer.effectWorld.height != height)) {
		local->tempImageBuffer.Destroy();
	}

	if(!local->tempImageBuffer.handle) {
		//Create a new "world" (aka, an image buffer).
		switch(smartRender->input->bitdepth) {
			case 8:
				err = suites.WorldSuite3()->AEGP_New(NULL, AEGP_WorldType_8, width, height, &local->tempImageBuffer.handle);
				if(err) throw (err);
				break;
			case 16:
				err = suites.WorldSuite3()->AEGP_New(NULL, AEGP_WorldType_16, width, height, &local->tempImageBuffer.handle);
				if(err) throw (err);
				break;
			case 32:
				err = suites.WorldSuite3()->AEGP_New(NULL, AEGP_WorldType_32, width, height, &local->tempImageBuffer.handle);
				if(err) throw (err);
				break;
			default:
				break;
		}
		local->tempImageBuffer.bitDepth = smartRender->input->bitdepth;
		err = suites.WorldSuite3()->AEGP_FillOutPFEffectWorld(local->tempImageBuffer.handle, &local->tempImageBuffer.effectWorld);
		if(err) throw (err);
	}
		

	PF_LRect rectOut {0, 0, width, height};
	ScaleAroundCentre(in_data, &local->activeKFB->cachedImage, &local->tempImageBuffer.effectWorld, &rectOut, local->activeZoomScale, tempScale, tempScale, 1.0);

	if(local->nextFrameKFB && local->nextFrameKFB->isImageCached) {
		ScaleAroundCentre(in_data, &local->nextFrameKFB->cachedImage, &local->tempImageBuffer.effectWorld, &rectOut, local->nextZoomScale, tempScale, tempScale, nextOpacity);

	}
	double scaleAdjust = 1 + (1 / local->width) * 2;
	ScaleAroundCentre(in_data, &local->tempImageBuffer.effectWorld, output, &smartRender->input->output_request.rect, scaleAdjust, 1 / (tempScale), 1 / tempScale, 1.0);

	return;
	
}

/*******************************************************************************************************
Make a chached image of the .kfb
*******************************************************************************************************/
static void makeKFBCachedImage(std::shared_ptr<KFBData> &  kfb, PF_InData *in_data, PF_SmartRenderExtra* smartRender, LocalSequenceData * local) {
	PF_Err err {PF_Err_NONE};
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	AEFX_CLR_STRUCT(kfb->cachedImage);
	int width = static_cast<int>(kfb->getWidth() / local->scaleFactorX);
	int height = static_cast<int>(kfb->getHeight() / local->scaleFactorY);
	
	//Create a new "world" (aka, an image buffer).
	switch(smartRender->input->bitdepth) {
		case 8:
			err = suites.WorldSuite3()->AEGP_New(NULL, AEGP_WorldType_8, width, height,&kfb->cachedImageAEGP);
			if(err) throw(err);
			break;
		case 16:
			err = suites.WorldSuite3()->AEGP_New(NULL, AEGP_WorldType_16, width, height, &kfb->cachedImageAEGP);
			if(err) throw(err);
			break;
		case 32:
			err = suites.WorldSuite3()->AEGP_New(NULL, AEGP_WorldType_32, width, height, &kfb->cachedImageAEGP);
			if(err) throw(err);
			break;
		default:
			break;
	}
	
	err = suites.WorldSuite3()->AEGP_FillOutPFEffectWorld(kfb->cachedImageAEGP, &kfb->cachedImage);
	if(err) throw(err);

	//Adjust zoom scales, because we don't want a zoomed image, then call GenerateImage
	auto backup1 = local->keyFramePercent;
	auto backup2 = local->activeZoomScale;
	auto backup3 = local->nextZoomScale;
	local->keyFramePercent = 0;
	local->activeZoomScale = 1;
	local->nextZoomScale = 0;
	GenerateImage(in_data, smartRender, &kfb->cachedImage , local);
	local->keyFramePercent = backup1;
	local->activeZoomScale = backup2;
	local->nextZoomScale = backup3;
	
	kfb->isImageCached = true;
	local->saveCachedParameters();
}




/*******************************************************************************************************
Scales the input image about its centre, and writes it to output.
*******************************************************************************************************/
static void ScaleAroundCentre(PF_InData *in_data, PF_EffectWorld* input, PF_EffectWorld* output, const PF_Rect * rect, double scale, double postScaleX, double postScaleY, double opacity) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	const float s = static_cast<float>(scale);
	const float sX = static_cast<float>(postScaleX);
	const float sY = static_cast<float>(postScaleY);
	const float centreX = static_cast<float>(input->width) / 2;
	const float centreY = static_cast<float>(input->height) / 2;
	
	//Note: For specifying an AE matrix, the inner brackets is a column.
	const PF_FloatMatrix activeTrans = {{{s / sX, 0, 0}, {0, s / sY, 0}, {-(s / sX)*(centreX)+(centreX / sX), -(s / sY)*centreY + (centreY / sY), 1}}};

	PF_CompositeMode comp;
	AEFX_CLR_STRUCT(comp);
	comp.xfer = PF_Xfer_IN_FRONT;
	comp.opacity = roundTo8Bit(opacity * white8);
	comp.opacitySu = roundTo16Bit(std::round(opacity * white16));

	auto err = suites.WorldTransformSuite1()->transform_world(in_data->effect_ref, PF_Quality_HI, 0, in_data->field, input, &comp, nullptr, &activeTrans, 1, true, rect, output);
	if(err) throw (err);
}


/*******************************************************************************************************
Selects a pixel iterator based on method.
Note: method is the index of the drop-down box paramater.
*******************************************************************************************************/
inline PixelFunction8 selectPixelRenderFunction8(long method) {
	switch(method) {
		case 1:
			return Render_KFRColouring::Render8;
		case 2:
			return Render_KFRDistance::Render8;
		case 4:
			return Render_DarkLightWave::Render8;
		case 5:
			return Render_WaveOnPalette::Render8;
		case 6:
			return Render_LogSteps::Render8;
		case 7:
			return Render_LogStepPalette::Render8;
		case 8:
			return Render_Panels::Render8;
		case 9:
			return Render_PanelsColour::Render8;
		case 10:
			return Render_Angle::Render8;
		case 11:
			return Render_AngleColour::Render8;
		case 12:
			return Render_DEAndAngle::Render8;
		default:
			throw(std::exception("Unknown rendering method"));
	}
}

/*******************************************************************************************************
Selects a pixel iterator based on method.
Note: method is the index of the drop-down box paramater.
*******************************************************************************************************/
inline PixelFunction16 selectPixelRenderFunction16(long method) {
	switch(method) {
		case 1:
			return Render_KFRColouring::Render16;
		case 2:
			return Render_KFRDistance::Render16;
		case 4:
			return Render_DarkLightWave::Render16;
		case 5:
			return Render_WaveOnPalette::Render16;
		case 6:
			return Render_LogSteps::Render16;
		case 7:
			return Render_LogStepPalette::Render16;
		case 8:
			return Render_Panels::Render16;
		case 9:
			return Render_PanelsColour::Render16;
		case 10:
			return Render_Angle::Render16;
		case 11:
			return Render_AngleColour::Render16;
		case 12:
			return Render_DEAndAngle::Render16;
		default:
			throw(std::exception("Unknown rendering method"));
	}
}

/*******************************************************************************************************
Selects a pixel iterator based on method.
Note: method is the index of the drop-down box paramater.
*******************************************************************************************************/
inline PixelFunction32 selectPixelRenderFunction32(long method) {
	switch(method) {
		case 1:
			return Render_KFRColouring::Render32;
		case 2:
			return Render_KFRDistance::Render32;
		case 4:
			return Render_DarkLightWave::Render32;
		case 5:
			return Render_WaveOnPalette::Render32;
		case 6:
			return Render_LogSteps::Render32;
		case 7:
			return Render_LogStepPalette::Render32;
		case 8:
			return Render_Panels::Render32;
		case 9:
			return Render_PanelsColour::Render32;
		case 10:
			return Render_Angle::Render32;
		case 11:
			return Render_AngleColour::Render32;
		case 12:
			return Render_DEAndAngle::Render32;
		default:
			throw(std::exception("Unknown rendering method"));
	}
}


/*******************************************************************************************************
Adjust the iteration count based on the the selected modifier
Called by pixel functions.
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


/*******************************************************************************************************
Calculates the interation count for (x,y) by blending frames.
Note: Must be thread-safe, so "LocalSequenceData" should be read-only.
Called by pixel functions.
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

/*******************************************************************************************************
Set the colour of the pixel to the "inside colour" selected by the user.
*******************************************************************************************************/
PF_Err SetInsideColour8(const LocalSequenceData* local, PF_Pixel8 * out) {
	out->alpha = white8;
	out->red = local->insideColour.red;
	out->green = local->insideColour.green;
	out->blue = local->insideColour.blue;
	return PF_Err_NONE;
}

/*******************************************************************************************************
Set the colour of the pixel to the "inside colour" selected by the user.
*******************************************************************************************************/
PF_Err SetInsideColour16(const LocalSequenceData* local, PF_Pixel16 * out) {
	constexpr double colourScale = static_cast<double>(white16) / static_cast<double>(white8);
	out->alpha = white16;
	out->red = roundTo16Bit(local->insideColour.red * colourScale);
	out->green = roundTo16Bit(local->insideColour.green* colourScale);
	out->blue = roundTo16Bit(local->insideColour.blue* colourScale);
	return PF_Err_NONE;
}

/*******************************************************************************************************
Set the colour of the pixel to the "inside colour" selected by the user.
*******************************************************************************************************/
PF_Err SetInsideColour32(const LocalSequenceData* local, PF_Pixel32 * out) {
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
Output parameters: highColour, lowColour, mixWeight 
*******************************************************************************************************/
void GetColours(const LocalSequenceData* local, double iCount, RGB & highColour, RGB & lowColour, double & mixWeight, bool scaleLikeKF) {
	auto nColours = local->numKFRColours;
	if(nColours == 0) nColours = 1;
	if (scaleLikeKF) iCount *= static_cast<double>(nColours) / static_cast<double>(colourRange);  //Scale pallette like KF

	double rem = std::fmod(iCount, static_cast<double>(nColours));
	unsigned long lowColourIndex = static_cast<unsigned long>(std::floor(rem));
	if(lowColourIndex > nColours - 1) lowColourIndex = nColours - 1;
	unsigned long highColourIndex = lowColourIndex + 1;
	if(highColourIndex > nColours - 1) highColourIndex = 0;
	lowColour = local->kfrColours[lowColourIndex];
	highColour = local->kfrColours[highColourIndex];
	mixWeight = rem - std::floor(rem);
}



/*******************************************************************************************************
Adds slopes colour calculations to r,g,b
r,g,b are colour values from 0.0 to 1.0
p[x][y] is a maxtrix of itaration values around point p[1][1] (may be a minimal cross)
*******************************************************************************************************/
void doSlopes(float p[][3], const LocalSequenceData* local, double& r, double& g, double& b) {
	if(local->slopeMethod == 1) {
		//Standard (like KF)
		float diffx = (p[0][1] - p[2][1]) / 2.0f;
		float diffy = (p[1][0] - p[1][2]) / 2.0f;
		double diff = diffx*local->slopeAngleX + diffy*local->slopeAngleY;

		double p1 = fmax(1, p[1][1]);
		diff = (p1 + diff) / p1;

		//Different to KF code, as I want it frame independant, might need improving
		diff = pow(diff, local->slopeShadowDepth * std::log(p[1][1] / 5000 + 1) * (local->width));

		if(diff > 1) {
			diff = (atan(diff) - pi / 4) / (pi / 4);
			diff = diff*local->slopeStrength / 100;
			r = (1 - diff)*r;
			g = (1 - diff)*g;
			b = (1 - diff)*b;
		}
		else {
			diff = 1 / diff;
			diff = (atan(diff) - pi / 4) / (pi / 4);
			diff = diff*local->slopeStrength / 100;;
			r = (1 - diff)*r + diff;
			g = (1 - diff)*g + diff;
			b = (1 - diff)*b + diff;
		}
	}
	else if(local->slopeMethod == 2) {
		//Angle Only
		float dx = (p[0][1] - p[2][1]);
		float dy = (p[1][0] - p[1][2]);

		//For clean colouring we need to take colour from nearby pixel at stationaty points.
		if(dx == 0 && dy == 0) {
			dx = (p[0][0] - p[2][0]);
			if(dx == 0) {
				dx = (p[0][2] - p[2][2]);
				if(dx == 0) {
					dy = (p[0][0] - p[0][2]);
					if(dy == 0) dy = (p[2][0] - p[2][2]);
				}
			}
		}

		double angle = std::atan2(dy, dx) + pi;
		angle += (local->slopeAngle / 360.0) *2*pi;
		double colour = (std::sin(angle) + 1) / 2;
		
		auto depth = local->slopeShadowDepth / 100;
		colour = (1 - depth) + colour*depth;

		colour *= 1+(local->slopeStrength / 100);
		r *= colour;
		g *= colour;
		b *= colour;
	}
}

/*******************************************************************************************************
Gets a 3x3 matrix of values surrouning a given location (x,y)
Called by pixel functions.
(x,y) is the AE requested pixel, this function will translate to .kfb coordinates.
The value is calculated by blending .kfbs
For use in frame-by-frame (not suitable for cached images). 
No intra-frame complensation, so will create the pulsating look.
*******************************************************************************************************/
void GetBlendedDistanceMatrix(float matrix[][3], const LocalSequenceData* local, A_long x, A_long y) {
	const double halfWidth = static_cast<double>(local->width) / 2.0;
	const double halfHeight = static_cast<double>(local->height) / 2.0;
	const double xCentre = (x * local->scaleFactorX) - halfWidth;
	const double yCentre = (y * local->scaleFactorY) - halfHeight;
	double xLocation = xCentre / local->activeZoomScale + halfWidth;
	double yLocation = yCentre / local->activeZoomScale + halfHeight;

	local->activeKFB->getDistanceMatrix(matrix, static_cast<float>(xLocation), static_cast<float>(yLocation), 1 / static_cast<float>(local->activeZoomScale));

	if(local->nextFrameKFB && local->keyFramePercent > 0.01 && local->nextZoomScale > 0) {
		xLocation = xCentre / local->nextZoomScale + halfWidth;
		yLocation = yCentre / local->nextZoomScale + halfHeight;
		bool nextInBounds = (xLocation >= 1 && yLocation >= 1 && xLocation <= local->width - 2 && yLocation <= local->height - 2);

		if(nextInBounds) {
			float next[3][3];
			local->nextFrameKFB->getDistanceMatrix(next, static_cast<float>(xLocation), static_cast<float>(yLocation), 1 / static_cast<float>(local->nextZoomScale));
			const float mixWeight = static_cast<float>(local->keyFramePercent);
			for(int i = 0; i < 3; i++) for(int j = 0; j < 3; j++) {
				matrix[i][j] = (1 - mixWeight) * matrix[i][j] + next[i][j] * mixWeight;
			}
		}
	}
}

/*******************************************************************************************************
Build distance matrix for static cached image in a way that will work with cached image scaling.
This is required because DE is faked using pixel values.
It interpolate over smaller distances near the centre of the image

minmal (default=false) will only fill a cross (unless overidden in local)
*******************************************************************************************************/
void getDistanceIntraFrame(float p[][3], A_long x, A_long y, const LocalSequenceData* local, bool minimal) {
	float step = 0.5;

	//Calculate pixel location.
	double halfWidth = static_cast<double>(local->width) / 2.0f;
	double halfHeight = static_cast<double>(local->height) / 2.0f;
	double xCentre = (x * local->scaleFactorX) - halfWidth;
	double yCentre = (y * local->scaleFactorY) - halfHeight;
	double xLocation = xCentre / local->activeZoomScale + halfWidth;
	double yLocation = yCentre / local->activeZoomScale + halfHeight;
	float xf = static_cast<float>(xLocation);
	float yf = static_cast<float>(yLocation);

	float adjustX = xf / static_cast<float>(local->scaleFactorX);
	float adjustY = yf / static_cast<float>(local->scaleFactorY);

	float distanceToEdgeX = std::min(xf, local->width - xf);
	float distanceToEdgeY = std::min(yf, local->height - yf);
	float distanceToEdge = std::min(distanceToEdgeX, distanceToEdgeY);

	float percentX = (distanceToEdgeX / static_cast<float>(local->width / 4));
	float percentY = (distanceToEdgeY / static_cast<float>(local->height / 4));
	float percent = std::min(percentX, percentY);
	step = std::exp(-std::log(2.0f)*percent); //Assumes zoom size 2

	bool min = minimal && !local->overrideMinimalDistance;
	local->activeKFB->getDistanceMatrix(p, static_cast<float>(xLocation), static_cast<float>(yLocation), step, min);
}

/*******************************************************************************************************


*******************************************************************************************************/
ARGBdouble sampleLayerPixel(const LocalSequenceData * local, double x, double y) {
	ARGBdouble result(1.0,0.5,0.5,0.5); //Default to grey
	auto layer = local->layer;
	if(!layer) return result;
	if(layer->width == 0 || layer->height == 0) return result;

	if(x < 0) x = 0;
	if(y < 0)y = 0;
	if(x > layer->width) x = layer->width;
	if(y > layer->height) y = layer->height;
	PF_Fixed xF = static_cast<PF_Fixed>(x * 65536);
	PF_Fixed yF = static_cast<PF_Fixed>(y * 65536);

	
	
	PF_SampPB sampPB {};
	sampPB.src = layer;
	switch(local->bitDepth) {
		case 8:
			{
				PF_Pixel pixel {};
				local->sample8->subpixel_sample(local->in_data->effect_ref, xF, yF, &sampPB, &pixel);
				result.alpha = static_cast<double>(pixel.alpha) / white8;
				result.red = static_cast<double>(pixel.red) / white8;
				result.green = static_cast<double>(pixel.green) / white8;
				result.blue = static_cast<double>(pixel.blue) / white8;
				break;
			}

		case 16:
			{
				PF_Pixel16 pixel {};
				local->sample16->subpixel_sample16(local->in_data->effect_ref, xF, yF, &sampPB, &pixel);
				result.alpha = static_cast<double>(pixel.alpha) / white16;
				result.red = static_cast<double>(pixel.red) / white16;
				result.green = static_cast<double>(pixel.green) / white16;
				result.blue = static_cast<double>(pixel.blue) / white16;
				break;
			}
			break;
		case 32:
			{
				PF_Pixel32 pixel {};
				local->sample32->subpixel_sample_float(local->in_data->effect_ref, xF, yF, &sampPB, &pixel);
				result.alpha = static_cast<double>(pixel.alpha) ;
				result.red = static_cast<double>(pixel.red);
				result.green = static_cast<double>(pixel.green);
				result.blue = static_cast<double>(pixel.blue) ;
				break;
			}
			break;
		default:
			break;
	}
	

	return result;
}



/*******************************************************************************************************
Render (non smart)
We won't support older versions of AE.  Just set pixels to black.
*******************************************************************************************************/
PF_Err NonSmartRender(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef	*output) {
	PF_Err err = PF_Err_NONE;
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	auto lines = output->extent_hint.bottom - output->extent_hint.top;

	if(output->world_flags & PF_WorldFlag_DEEP) {
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
	out->alpha = white8;
	return PF_Err_NONE;
}

/*******************************************************************************************************
Sets a pixel to black (16 bit pixel)
Called by a pixel iterator.
*******************************************************************************************************/
static PF_Err SetToBlack16(void* refcon, A_long x, A_long y, PF_Pixel16* in, PF_Pixel16* out) {
	out->red = 0;
	out->blue = 0;
	out->green = 0;
	out->alpha = white16;
	return PF_Err_NONE;
}