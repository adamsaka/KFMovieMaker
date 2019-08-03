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
inline double doDistance(float p[][3], A_long x, A_long y, const LocalSequenceData* local, bool locationBasedScale=true) {
	
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
inline double RenderCommon(const LocalSequenceData * local, float distance[][3], A_long x, A_long y) {
	//Get iteration value
	double iCount = GetBlendedPixelValue(local, x, y);

	//Check for inside pixel
	if(iCount >= local->activeKFB->maxIterations)  return -1;

	
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
	return iCount;
}

/*******************************************************************************************************
Distance Estimation 8bpc
*******************************************************************************************************/
PF_Err Render_KFRDistance::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);
	float distance[3][3];

	double iCount = RenderCommon(local, distance, x, y);
	if(iCount == -1) return SetInsideColour8(local, out);

	//Get Colours
	RGB highColour, lowColour;
	double mixWeight {0.0};
	GetColours(local, iCount, highColour, lowColour, mixWeight);

	//Mix Colours
	double r = lowColour.red * (1 - mixWeight) + highColour.red* mixWeight;
	double g = lowColour.green * (1 - mixWeight) + highColour.green* mixWeight;
	double b = lowColour.blue * (1 - mixWeight) + highColour.blue *mixWeight;

	//Set colours and add slopes.
	out->alpha = white8;
	if(local->slopesEnabled) {
		r /=  255.0;
		g /=  255.0;
		b /= 255.0;
		doSlopes(distance,local,r,g,b);
		r *= white8;
		g *= white8;
		b *= white8;
	}

	out->red = roundTo8Bit(r);
	out->green = roundTo8Bit(g);
	out->blue = roundTo8Bit(b);
	return PF_Err_NONE;
}

/*******************************************************************************************************
Distance Estimation 16bpc
*******************************************************************************************************/
PF_Err Render_KFRDistance::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);
	float distance[3][3];

	double iCount = RenderCommon(local, distance, x, y);
	if(iCount == -1) return SetInsideColour16(local, out);

	//Get Colours
	RGB highColour, lowColour;
	double mixWeight {0.0};
	GetColours(local, iCount, highColour, lowColour, mixWeight);

	//Mix Colours
	double r = lowColour.red * (1 - mixWeight) + highColour.red* mixWeight;
	double g = lowColour.green * (1 - mixWeight) + highColour.green* mixWeight;
	double b = lowColour.blue * (1 - mixWeight) + highColour.blue *mixWeight;

	//Set colours and add slopes.
	out->alpha = white16;
	if(local->slopesEnabled) {
		r /= 255.0;
		g /= 255.0;
		b /= 255.0;
		doSlopes(distance, local, r, g, b);
		r *= white16;
		g *= white16;
		b *= white16;
	}
	else {
		double colourScale = white16 / white8;
		r *= colourScale;
		g *= colourScale;
		b *= colourScale;
	}

	out->red = roundTo16Bit(r);
	out->green = roundTo16Bit(g);
	out->blue = roundTo16Bit(b);
	return PF_Err_NONE;
}

/*******************************************************************************************************
Distance Estimation 32bpc
*******************************************************************************************************/
PF_Err Render_KFRDistance::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * in, PF_Pixel32 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);
	float distance[3][3];

	double iCount = RenderCommon(local, distance, x, y);
	if(iCount == -1) return SetInsideColour32(local, out);

	//Get Colours
	RGB highColour, lowColour;
	double mixWeight {0.0};
	GetColours(local, iCount, highColour, lowColour, mixWeight);

	//Mix Colours
	double r = lowColour.red * (1 - mixWeight) + highColour.red* mixWeight;
	double g = lowColour.green * (1 - mixWeight) + highColour.green* mixWeight;
	double b = lowColour.blue * (1 - mixWeight) + highColour.blue *mixWeight;

	//Set colours and add slopes.
	out->alpha = white32;
	r /= 255.0;
	g /= 255.0;
	b /= 255.0;

	if(local->slopesEnabled) doSlopes(distance, local, r, g, b);
	
	if(r < 0) r = 0;
	if(g < 0) g = 0;
	if(b < 0) b = 0;
	out->red = static_cast<float>(r);
	out->green = static_cast<float>(g);
	out->blue = static_cast<float>(b);

	return PF_Err_NONE;
}
