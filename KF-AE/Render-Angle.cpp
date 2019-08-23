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
#include "Render-Angle.h"
#include "LocalSequenceData.h"
#include "Render.h"
#include <cmath>

constexpr double logScale = 10;

/*******************************************************************************************************
Rendering Code common to all bit depths
*******************************************************************************************************/
inline static double RenderCommon(const LocalSequenceData * local, A_long x, A_long y) {
	double iCount = GetBlendedPixelValue(local, x, y);
	if(iCount >= local->activeKFB->maxIterations)  return -1;  //Inside pixel

	float distance[3][3];
	if(local->scalingMode == 1) {
		getDistanceIntraFrame(distance, x, y, local);
	}
	else {
		GetBlendedDistanceMatrix(distance, local, x, y);
	}
	
	float dx = (distance[0][1] - distance[2][1]) ;
	float dy = (distance[1][0] - distance[1][2]);
	
	//For clean colouring we need to take colour from nearby pixel at stationaty points.
	if(dx == 0 && dy == 0) {
		dx = (distance[0][0] - distance[2][0]);
		if(dx == 0) {
			dx = (distance[0][2] - distance[2][2]);
			if(dx == 0) {
				dy = (distance[0][0] - distance[0][2]);
				if(dy == 0) dy = (distance[2][0] - distance[2][2]);
			}
		}
	}
	double angle = std::atan2(dy, dx) +pi;
	
	angle *= local->colourDivision;
	angle += (local->colourOffset/1024)*2*pi;
	angle = doModifier(local->modifier, angle);

	double colour = (std::sin(angle)+1)/2;
	

	if(local->slopesEnabled) {
		double tempA, tempB;
		doSlopes(distance, local, colour, tempA, tempB);
	}
	return colour;
}


/*******************************************************************************************************
Render a pixel at 8-bit colour depth.
*******************************************************************************************************/
PF_Err Render_Angle::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);

	double colour = RenderCommon(local, x, y);
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
PF_Err Render_Angle::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);

	double colour = RenderCommon(local, x, y);
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
PF_Err Render_Angle::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * iP, PF_Pixel32 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);

	double colour = RenderCommon(local, x, y);
	if(colour == -1) SetInsideColour32(local, out);

	auto val = static_cast<float>(colour);
	if(val < 0.0) val = 0.0; //Negative values causing rendering issues

	out->red = val;
	out->blue = val;
	out->green = val;
	out->alpha = white32;
	return PF_Err_NONE;

}
