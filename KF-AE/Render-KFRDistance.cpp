/********************************************************************************************
Distance Estimation Rendering.

Authors:	Adam Sakareassen 
			Karl Runmo (via Kalles Fraktaler 2)
			Claude Heiland-Allen (via Kalles Fraktaler 2)

Credits:	This file contains code snippets copied from the Kalles Fraktaler Project 
			Project:	Kalles Fraktaler 2
			File:		fraktal_sft.cpp 
			Version:	2.14.6.1
			Licence:	GNU Affero General Public License
			Copyright (C) 2013-2017 Karl Runmo
			Copyright (C) 2017-2019 Claude Heiland-Allen

Licence:	GNU Affero General Public License

********************************************************************************************
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
********************************************************************************************/

#include "Render-KFRDistance.h"
#include "Render.h"





/*******************************************************************************************************
Perform distance calculation
*******************************************************************************************************/
inline static double doDistance(float p[][3], A_long x, A_long y, const LocalSequenceData* local, bool locationBasedScale=true) {
	
	//Traditional
	double gx = (p[0][1] - p[1][1]) ;
	double gy = (p[1][0] - p[1][1]) ;
	double gu = (p[0][0] - p[1][1]) ;
	double gv = (p[0][2] - p[1][1]) ;
	double g = fabs(gx) + fabs(gy) + fabs(gu) + fabs(gv);
	auto iter = g;

	return iter;
}



/*******************************************************************************************************
The initial render calculations, common to all bit depths.
*******************************************************************************************************/
inline ARGBdouble RenderCommon(const LocalSequenceData * local,  A_long x, A_long y) {
	float distance[3][3];

	//Get iteration value
	double iCount = GetBlendedPixelValue(local, x, y);
	if(iCount >= local->activeKFB->maxIterations)  return ARGBdouble(-1, -1, -1, -1);  //Inside pixel

	

	
	if(local->scalingMode == 1) {
		getDistanceIntraFrame(distance, x, y, local);		
	}
	else {
		GetBlendedDistanceMatrix(distance, local, x, y);
	}

	iCount = doDistance(distance, x, y, local);
	iCount = doModifier(local->modifier, iCount);
	if(local->modifier == 4) iCount++; //log colouring needs minimum value to be 1.
	if(iCount > 1024) iCount = 1024;  //clamped to match KF. 
	iCount /= local->colourDivision;
	if(local->distanceClamp > 0 && iCount > local->distanceClamp) iCount = local->distanceClamp;
	iCount += local->colourOffset;
	

	ARGBdouble result(1.0,0.5,0.5,0.5);
	if(local->sampling) {
		if(local->layer) {
			//Override, use sampled layer for colours.
			double index = (std::fmod(iCount, 1024) / 1024)*(local->layer->width*local->layer->height);
			double x = std::fmod(index, local->layer->width);
			double y = std::floor(index / local->layer->width);
			result = sampleLayerPixel(local, x, y);
		}

	}
	else {
		//Get Colours from .kfr
		RGB highColour, lowColour;
		double mixWeight {0.0};
		GetColours(local, std::floor(iCount), highColour, lowColour, mixWeight, false);

		//Mix Colours
		result.red = (lowColour.red * (1 - mixWeight) + highColour.red* mixWeight) / white8;
		result.green = (lowColour.green * (1 - mixWeight) + highColour.green* mixWeight) / white8;
		result.blue = (lowColour.blue * (1 - mixWeight) + highColour.blue *mixWeight) / white8;
	}


	if(local->slopesEnabled) {
		if(local->scalingMode == 1) {
			getDistanceIntraFrame(distance, x, y, local, true);
		}
		else {
			GetBlendedDistanceMatrix(distance, local, x, y);
		}

		doSlopes(distance, local, result.red, result.green, result.blue);
	}
	return result;
}

/*******************************************************************************************************
Distance Estimation 8bpc
*******************************************************************************************************/
PF_Err Render_KFRDistance::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);

	auto colour = RenderCommon(local, x, y);
	if(colour.red == -1) SetInsideColour8(local, out);

	out->red = roundTo8Bit(colour.red * white8);
	out->green = roundTo8Bit(colour.green * white8);
	out->blue = roundTo8Bit(colour.blue * white8);
	out->alpha = white8;
	return PF_Err_NONE;
}

/*******************************************************************************************************
Distance Estimation 16bpc
*******************************************************************************************************/
PF_Err Render_KFRDistance::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);

	auto colour = RenderCommon(local, x, y);
	if(colour.red == -1) SetInsideColour16(local, out);

	out->red = roundTo16Bit(colour.red * white16);
	out->green = roundTo16Bit(colour.green * white16);
	out->blue = roundTo16Bit(colour.blue * white16);
	out->alpha = white16;
	return PF_Err_NONE;
}

/*******************************************************************************************************
Distance Estimation 32bpc
*******************************************************************************************************/
PF_Err Render_KFRDistance::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * in, PF_Pixel32 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);

	auto colour = RenderCommon(local, x, y);
	if(colour.red == -1) SetInsideColour32(local, out);


	if(colour.red < 0.0) colour.red = 0.0; //Negative values causing rendering issues
	if(colour.green < 0.0) colour.green = 0.0;
	if(colour.blue < 0.0) colour.blue = 0.0;

	out->red = static_cast<float>(colour.red);
	out->green = static_cast<float>(colour.green);
	out->blue = static_cast<float>(colour.blue);
	out->alpha = white32;
	return PF_Err_NONE;
}
