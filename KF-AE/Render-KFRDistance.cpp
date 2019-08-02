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


inline void fillDistanceScale(float p[][3], A_long x, A_long y, const LocalSequenceData* local) {
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
	//Assumes zoom size 2
	
	float percentX = (distanceToEdgeX / static_cast<float>(local->width / 4));
	float percentY = (distanceToEdgeY / static_cast<float>(local->height / 4));
	float percent = std::min(percentX, percentY);
	step = std::exp(-std::log(2.0f)*percent);
	
	local->activeKFB->getDistanceMatrix(p, static_cast<float>(xLocation), static_cast<float>(yLocation), step);

}


double doDistance(float p[][3], A_long x, A_long y, const LocalSequenceData* local, bool locationBasedScale=true) {
	
	
	//Traditional
	double gx = (p[0][1] - p[1][1]) ;
	double gy = (p[1][0] - p[1][1]) ;
	double gu = (p[0][0] - p[1][1]) ;
	double gv = (p[0][2] - p[1][1]) ;
	double g = fabs(gx) + fabs(gy) + fabs(gu) + fabs(gv);
	auto iter = g;

	return iter;
}

void doSlopes(float p[][3], LocalSequenceData* local, double& r, double& g, double& b) {
	float diffx = (p[0][1] - p[1][1]);
	float diffy = (p[1][0] - p[1][1]);
	double diff = diffx*local->slopeAngleX + diffy*local->slopeAngleY;
	
	double p1 = fmax(1, p[1][1]);
	diff = (p1 + diff) / p1;
	diff = pow(diff, local->slopeShadowDepth * std::log(p[1][1]/10000+1) * (local->width)); //Arbitrary choice, need to inspect KF code 
	
	if(diff>1) {
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

/*******************************************************************************************************
The initial render calculations, common to all bit depths.
*******************************************************************************************************/
inline double RenderCommon(const LocalSequenceData * local, float distance[][3], A_long x, A_long y) {
	//Get iteration value
	double iCount = GetBlendedPixelValue(local, x, y);

	//Check for inside pixel
	if(iCount >= local->activeKFB->maxIterations)  return -1;

	
	if(local->nextZoomScale > 0) {
		GetBlendedDistanceMatrix(distance, local, x, y);
	}
	else {
		fillDistanceScale(distance, x, y, local);
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
		
	out->red = static_cast<float>(r);
	out->green = static_cast<float>(g);
	out->blue = static_cast<float>(b);

	return PF_Err_NONE;
}
