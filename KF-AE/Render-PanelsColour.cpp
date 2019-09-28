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
#include "Render-PanelsColour.h"
#include "LocalSequenceData.h"
#include "Render.h"

constexpr double logScale = 10;
constexpr double greyColour = 1.0; //Peak grey colour
constexpr double curveSize = 0.1;  //size of smoothed curve

constexpr double overshoot = 0.002; //Overshoot the top of the sin wave, gives a subtle highlight

static const double internalColour = greyColour * std::sin(((curveSize + overshoot) / curveSize) * pi / 2);

/*******************************************************************************************************
Rendering Code common to all bit depths
*******************************************************************************************************/
inline static ARGBdouble RenderCommon(const LocalSequenceData * local, A_long x, A_long y) {
	double iCount = GetBlendedPixelValue(local, x, y);
	if(iCount >= local->activeKFB->maxIterations)  return ARGBdouble(-1, -1, -1, -1);  //Inside pixel

	iCount = doModifier(local->modifier, iCount);
	iCount /= local->colourDivision;
	iCount += local->colourOffset;

	
	ARGBdouble result(1.0,0.5,0.5,0.5);
	if(local->sampling) {
		if(local->layer) {
			//Override, use sampled layer for colours.
			double index = (std::fmod(floor(iCount), 1024) / 1024)*(local->layer->width*local->layer->height);
			double x = std::fmod(index, local->layer->width);
			double y = std::floor(index / local->layer->width);
			result = sampleLayerPixel(local, x, y);
		}
	}
	else {
		//Get Colours
		RGB highColour, lowColour;
		double mixWeight {0.0};
		GetColours(local, std::floor(iCount), highColour, lowColour, mixWeight, false);
		//Mix Colours
		result.red = (lowColour.red * (1 - mixWeight) + highColour.red* mixWeight) / white8;
		result.green = (lowColour.green * (1 - mixWeight) + highColour.green* mixWeight) / white8;
		result.blue = (lowColour.blue * (1 - mixWeight) + highColour.blue *mixWeight) / white8;
	}

	//Panels
	double colour = greyColour;
	double offSet = std::fmod(iCount, 1);
	double blackSize = (local->special / 200)*0.8;
	if(offSet < blackSize || offSet > 1 - blackSize) {
		//black gutter
		colour = 0;
	}
	else if(offSet < curveSize + blackSize + overshoot) {
		//curve in
		colour *= std::sin(((offSet - blackSize) / curveSize) * pi / 2);
	}
	else if(offSet >(1 - curveSize - blackSize - overshoot)) {
		//curve out
		colour *= std::sin(((1 - offSet - blackSize) / curveSize) * pi / 2);
	}
	else {
		//panel
		colour = internalColour;
	}
	if(colour < 0.1) colour = 0;

	//Mix panels and colour
	result.red *= colour;
	result.green *= colour;
	result.blue *= colour;


	if(local->slopesEnabled) {
		float distance[3][3];
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
Render a pixel at 8-bit colour depth.
*******************************************************************************************************/
PF_Err Render_PanelsColour::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
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
Render a pixel at 16-bit colour depth.
*******************************************************************************************************/
PF_Err Render_PanelsColour::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {
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
Render a pixel at 32-bit colour depth.
*******************************************************************************************************/
PF_Err Render_PanelsColour::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * iP, PF_Pixel32 * out) {
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
