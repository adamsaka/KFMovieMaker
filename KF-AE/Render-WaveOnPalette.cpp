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
#include "Render-WaveOnPalette.h"
#include "LocalSequenceData.h"
#include "Render.h"



/*******************************************************************************************************
Rendering Code common to all bit depths
*******************************************************************************************************/
inline static RGBdouble RenderCommon(const LocalSequenceData * local, A_long x, A_long y) {
	double iCount = GetBlendedPixelValue(local, x, y);
	if(iCount >= local->activeKFB->maxIterations)  return RGBdouble(-1,-1,-1);  //Inside pixel
	
	iCount = doModifier(local->modifier, iCount);
	iCount /= local->colourDivision;
	iCount += local->colourOffset;

	//Get Colours
	RGB highColour, lowColour;
	double mixWeight {0.0};
	GetColours(local, std::floor(iCount), highColour, lowColour, mixWeight, false);

	//Mix Colours
	RGBdouble result;
	result.red = (lowColour.red * (1 - mixWeight) + highColour.red* mixWeight) / white8;
	result.green = (lowColour.green * (1 - mixWeight) + highColour.green* mixWeight) / white8;
	result.blue = (lowColour.blue * (1 - mixWeight) + highColour.blue *mixWeight) / white8;

	iCount = std::fmod(iCount, 1);
	double sinMix = std::sin(pi * (iCount));
	//iCount = (iCount + 1) / 2; //scaled from 0.0 to 1.0;

	result.red *= sinMix;
	result.green *= sinMix;
	result.blue *= sinMix;
	

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
PF_Err Render_WaveOnPalette::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
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
PF_Err Render_WaveOnPalette::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {
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
PF_Err Render_WaveOnPalette::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * iP, PF_Pixel32 * out) {
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
