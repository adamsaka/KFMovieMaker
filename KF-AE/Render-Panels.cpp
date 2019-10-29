/********************************************************************************************
Author:			(c) 2019 Adam Sakareassen

Licence:		GNU Affero General Public License



********************************************************************************************
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
********************************************************************************************/
#include "Render-Panels.h"
#include "LocalSequenceData.h"
#include "Render.h"
#include <cmath>

constexpr double greyColour = 0.8; //Peak grey colour
constexpr double curveSize = 0.1;  //size of smoothed curve
constexpr double overshoot = 0.002; //Overshoot the top of the sin wave, gives a subtle highlight

static const double internalColour = greyColour * std::sin(((curveSize + overshoot) / curveSize) * pi / 2);

/*******************************************************************************************************
Rendering Code common to all bit depths
*******************************************************************************************************/
inline static double RenderCommonLogSteps(const LocalSequenceData * local, A_long x, A_long y) {
	double iCount = GetBlendedPixelValue(local, x, y);
	if(iCount >= local->activeKFB->maxIterations)  return -1;  //Inside pixel

	iCount = doModifier(local->modifier, iCount);
	iCount /= local->colourDivision;
	iCount += local->colourOffset;

	double colour = greyColour;
	double offSet = std::fmod(iCount, 1);
	double blackSize = (local->special/200)*0.8;
	

	if(offSet < blackSize || offSet > 1-blackSize) {
		//black gutter
		colour = 0;
	}else if(offSet < curveSize + blackSize + overshoot) {
		//curve in
		colour *= std::sin(((offSet-blackSize) / curveSize) * pi / 2);
	}
	else if(offSet > (1 - curveSize -blackSize-overshoot)) {
		//curve out
		colour *=  std::sin(((1-offSet-blackSize) / curveSize) * pi / 2);
	}
	else {
		//panel
		colour = internalColour;
	}
	if(colour < 0.1) colour = 0;

	if(local->slopesEnabled) {
		double distance[3][3];
		if(local->scalingMode == 1 && local->slopeMethod==1) {
			getDistanceIntraFrame(distance, x, y, local, true);
		}
		else {
			GetBlendedDistanceMatrix(distance, local, x, y);
		}
		double tempA, tempB;
		doSlopes(distance, local, colour, tempA, tempB);
	}
	return colour;
}


/*******************************************************************************************************
Render a pixel at 8-bit colour depth.
*******************************************************************************************************/
PF_Err Render_Panels::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
	const auto* local = static_cast<LocalSequenceData*>(refcon);

	double colour = RenderCommonLogSteps(local, x, y);
	if(colour == -1) SetInsideColour8(local, out);

	auto val = roundTo8Bit(colour * white8);
	out->red = val;
	out->blue = val;
	out->green = val;
	out->alpha = white8;

	return PF_Err_NONE;

}

/*******************************************************************************************************
Render a pixel at 16-bit colour depth.
*******************************************************************************************************/
PF_Err Render_Panels::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {
	const auto* local = static_cast<LocalSequenceData*>(refcon);

	double colour = RenderCommonLogSteps(local, x, y);
	if(colour == -1) SetInsideColour16(local, out);

	auto val = roundTo16Bit(colour * white16);

	out->red = val;
	out->blue = val;
	out->green = val;
	out->alpha = white16;
	return PF_Err_NONE;

}

/*******************************************************************************************************
Render a pixel at 32-bit colour depth.
*******************************************************************************************************/
PF_Err Render_Panels::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * iP, PF_Pixel32 * out) {
	const auto* local = static_cast<LocalSequenceData*>(refcon);

	double colour = RenderCommonLogSteps(local, x, y);
	if(colour == -1) SetInsideColour32(local, out);

	auto val = static_cast<float>(colour);
	if(val < 0.0) val = 0.0; //Negative values causing rendering issues

	out->red = val;
	out->blue = val;
	out->green = val;
	out->alpha = white32;
	return PF_Err_NONE;

}
