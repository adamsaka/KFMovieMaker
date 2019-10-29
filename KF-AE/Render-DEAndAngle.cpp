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
#include "Render-DEAndAngle.h"
#include "LocalSequenceData.h"
#include "Render.h"

constexpr double logScale = 10;
constexpr double greyColour = 1.0; //Peak grey colour
constexpr double curveSize = 0.1;  //size of smoothed curve
constexpr double blackSize = 0.01; //size of black gutter at each end.
constexpr double overshoot = 0.002; //Overshoot the top of the sin wave, gives a subtle highlight

static const double internalColour = greyColour * std::sin(((curveSize + overshoot) / curveSize) * pi / 2);


/*******************************************************************************************************
Perform distance calculation
*******************************************************************************************************/
inline static double doDistance(double p[][3], A_long x, A_long y, const LocalSequenceData* local, bool locationBasedScale = true) {

	//Traditional
	double gx = (p[0][1] - p[1][1]);
	double gy = (p[1][0] - p[1][1]);
	double gu = (p[0][0] - p[1][1]);
	double gv = (p[0][2] - p[1][1]);
	double g = fabs(gx) + fabs(gy) + fabs(gu) + fabs(gv);
	auto iter = g;

	return iter;
}

/*******************************************************************************************************
Rendering Code common to all bit depths
*******************************************************************************************************/
inline static ARGBdouble RenderCommon(const LocalSequenceData * local, A_long x, A_long y) {
	double iCount = GetBlendedPixelValue(local, x, y);
	if(iCount >= local->activeKFB->maxIterations)  return ARGBdouble(-1, -1, -1, -1);  //Inside pixel
	double distance[3][3];

	ARGBdouble result(1.0, 0.5, 0.5, 0.5);
	if(local->sampling) {
		if(local->layer) {
			if(local->scalingMode == 1) {
				getDistanceIntraFrame(distance, x, y, local);
			}
			else {
				GetBlendedDistanceMatrix(distance, local, x, y);
			}

			double dx = (distance[0][1] - distance[2][1]);
			double dy = (distance[1][0] - distance[1][2]);

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
			double angle = std::atan2(dy, dx) + pi;
			double dist = doDistance(distance, x, y, local, true);
	
			dist =  doModifier(local->modifier, dist)*1000;
			dist = std::log(std::log(dist + 1)+1) * 20;
			if(local->colourDivision > 0) dist /= local->colourDivision;

			dist += local->colourOffset;

			angle = angle / (2 * pi) * 1024;  //scale angle from 0 to 1024
			if (local->special !=0) angle *= local->special;  

	
			//Override, use sampled layer for colours.
			double x = (std::fmod(angle, 1024) / 1024)*(local->layer->width);
			double y = (std::fmod(dist, 1))*(local->layer->height);
			result = sampleLayerPixel(local, x, y);
		}
	}
	

	if(local->slopesEnabled) {
		doSlopes(distance, local, result.red, result.green, result.blue);
	}
	return  result;
}


/*******************************************************************************************************
Render a pixel at 8-bit colour depth.
*******************************************************************************************************/
PF_Err Render_DEAndAngle::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
	const auto* local = static_cast<LocalSequenceData*>(refcon);

	auto colour = RenderCommon(local, x, y);
	if(colour.red == -1) SetInsideColour8(local, out);


	out->red = roundTo8Bit(colour.red * white8);
	out->green = roundTo8Bit(colour.green * white8);
	out->blue = roundTo8Bit(colour.blue * white8);
	out->alpha = white8;

	return PF_Err_NONE;

}

/*******************************************************************************************************
Render a pixel at 16-bit colour depth.
*******************************************************************************************************/
PF_Err Render_DEAndAngle::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {
	const auto* local = static_cast<LocalSequenceData*>(refcon);

	auto colour = RenderCommon(local, x, y);
	if(colour.red == -1) SetInsideColour16(local, out);

	out->red = roundTo16Bit(colour.red * white16);
	out->green = roundTo16Bit(colour.green * white16);
	out->blue = roundTo16Bit(colour.blue * white16);
	out->alpha = white16;
	return PF_Err_NONE;

}

/*******************************************************************************************************
Render a pixel at 32-bit colour depth.
*******************************************************************************************************/
PF_Err Render_DEAndAngle::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * iP, PF_Pixel32 * out) {
	const auto* local = static_cast<LocalSequenceData*>(refcon);

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
