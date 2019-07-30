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


PF_Err Render_KFRDistance::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);

	//Get iteration value
	double iCount = GetBlendedPixelValue(local, x, y);

	//Check for inside pixel
	if(iCount >= local->activeKFB->maxIterations)  return SetInsideColour8(local, out);
	
	float distance[3][3];
	if(local->nextZoomScale > 0) {
		GetBlendedDistanceMatrix(distance, local, x, y);
	}
	else {
		fillDistanceScale(distance, x, y, local);
	}

	iCount = doDistance(distance,x, y, local);

	
	iCount = doModifier(local->modifier, iCount);
	if(local->modifier == 4) iCount++;

	if(iCount > 1024)iCount = 1024;
	iCount /= local->colourDivision;
	if(local->distanceClamp > 0 && iCount > local->distanceClamp) iCount = local->distanceClamp;
	iCount += local->colourOffset;

	//Get Colours
	RGB highColour, lowColour;
	double mixWeight {0.0};
	GetColours(local, iCount, highColour, lowColour, mixWeight);

	//Mix Colours
	double r = lowColour.red * (1 - mixWeight) + highColour.red* mixWeight;
	double g = lowColour.green * (1 - mixWeight) + highColour.green* mixWeight;
	double b = lowColour.blue * (1 - mixWeight) + highColour.blue *mixWeight;

	out->alpha = white8;
	out->red = roundTo8Bit(r);
	out->green = roundTo8Bit(g);
	out->blue = roundTo8Bit(b);
	return PF_Err_NONE;
}

PF_Err Render_KFRDistance::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {
	return PF_Err_NONE;
}

PF_Err Render_KFRDistance::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * in, PF_Pixel32 * out) {
	return PF_Err_NONE;
}
