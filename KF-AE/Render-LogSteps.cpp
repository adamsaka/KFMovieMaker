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
#include "Render-LogSteps.h"
#include "LocalSequenceData.h"
#include "Render.h"
#include <cmath>

constexpr double logScale = 10;  

/*******************************************************************************************************
Rendering Code common to all bit depths
*******************************************************************************************************/
inline static double RenderCommonLogSteps(const LocalSequenceData * local, A_long x, A_long y) {
	double iCount = GetBlendedPixelValue(local, x, y);
	if(iCount >= local->activeKFB->maxIterations)  return -1;  //Inside pixel

	iCount = doModifier(local->modifier, iCount);
	iCount /= local->colourDivision;
	iCount += local->colourOffset;

	iCount = 1- std::fmod(iCount,1) ;
	iCount = std::log((iCount * logScale)+1);
	iCount = (iCount)  / std::log(logScale+1); //scaled from 0.0 to 1.0;

	if(local->slopesEnabled) {
		double distance[3][3];
		if(local->scalingMode == 1) {
			getDistanceIntraFrame(distance, x, y, local, true);
		}
		else {
			GetBlendedDistanceMatrix(distance, local, x, y);
		}
		double tempA, tempB;
		doSlopes(distance, local, iCount, tempA, tempB);
	}
	return iCount;
}


/*******************************************************************************************************
Render a pixel at 8-bit colour depth.
*******************************************************************************************************/
PF_Err Render_LogSteps::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
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
PF_Err Render_LogSteps::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {
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
PF_Err Render_LogSteps::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * iP, PF_Pixel32 * out) {
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
