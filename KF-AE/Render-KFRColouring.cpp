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
#include "Render-KFRColouring.h"
#include "LocalSequenceData.h"
#include "Render.h"


/*******************************************************************************************************
Render a pixel at 8-bit colour depth.
*******************************************************************************************************/
PF_Err Render_KFRColouring::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
	const auto local = reinterpret_cast<LocalSequenceData*>(refcon);

	//Get iteration value
	double iCount = GetBlendedPixelValue(local, x, y);

	//Check for inside pixel
	if(iCount >= local->activeKFB->maxIterations)  return SetInsideColour8(local,out);
	
	
	iCount = doModifier(local->modifier, iCount);
	
	iCount /= local->colourDivision;
	iCount += local->colourOffset;
	
	//Get Colours
	RGB highColour, lowColour;
	double mixWeight {0.0};
	GetColours(local, iCount, highColour, lowColour, mixWeight);

	//Mix Colours
	double r = lowColour.red * (1-mixWeight) + highColour.red* mixWeight;
	double g = lowColour.green * (1 - mixWeight) + highColour.green* mixWeight;
	double b = lowColour.blue * (1 - mixWeight) + highColour.blue *mixWeight;
	
	out->alpha = white8;
	out->red = roundTo8Bit(r);
	out->green = roundTo8Bit(g);
	out->blue = roundTo8Bit(b);
	return PF_Err_NONE;
	
}

/*******************************************************************************************************
Render a pixel at 16-bit colour depth
*******************************************************************************************************/
PF_Err Render_KFRColouring::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {
	constexpr double colourScale = static_cast<double>(white16) / static_cast<double>(white8);
	const auto local = reinterpret_cast<LocalSequenceData*>(refcon);

	//Get iteration value
	double iCount = GetBlendedPixelValue(local, x, y);

	//Check for inside pixel
	if(iCount >= local->activeKFB->maxIterations) return SetInsideColour16(local, out);

	iCount = doModifier(local->modifier, iCount);
	iCount /= local->colourDivision;
	iCount += local->colourOffset;

	//Get Colours
	RGB highColour, lowColour;
	double mixWeight {0.0};
	GetColours(local, iCount, highColour, lowColour, mixWeight);

	//Mix Colours
	
	double r = lowColour.red * colourScale * (1 - mixWeight) + highColour.red * colourScale* mixWeight;
	double g = lowColour.green * colourScale * (1 - mixWeight) + highColour.green * colourScale * mixWeight;
	double b = lowColour.blue * colourScale * (1 - mixWeight) + highColour.blue * colourScale *mixWeight;

	out->alpha = white16;
	out->red = roundTo16Bit(r);
	out->green = roundTo16Bit(g);
	out->blue = roundTo16Bit(b);
	return PF_Err_NONE;
}

/*******************************************************************************************************
Render a pixel at 32-bit colour depth
*******************************************************************************************************/
PF_Err Render_KFRColouring::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * in, PF_Pixel32 * out) {
	constexpr double colourScale = static_cast<double>(white32) / static_cast<double>(white8);
	const auto local = reinterpret_cast<LocalSequenceData*>(refcon);

	//Get iteration value
	double iCount = GetBlendedPixelValue(local, x, y);

	//Check for inside pixel
	if(iCount >= local->activeKFB->maxIterations) return SetInsideColour32(local, out);

	iCount = doModifier(local->modifier, iCount);
	iCount /= local->colourDivision;
	iCount += local->colourOffset;

	//Get Colours
	RGB highColour, lowColour;
	double mixWeight {0.0};
	GetColours(local, iCount, highColour, lowColour, mixWeight);

	//Mix Colours
	double r = lowColour.red * colourScale * (1 - mixWeight) + highColour.red * colourScale* mixWeight;
	double g = lowColour.green * colourScale * (1 - mixWeight) + highColour.green * colourScale * mixWeight;
	double b = lowColour.blue * colourScale * (1 - mixWeight) + highColour.blue * colourScale *mixWeight;

	out->alpha = white32;
	out->red = static_cast<float>(r);
	out->green = static_cast<float>(g);
	out->blue = static_cast<float>(b);
	return PF_Err_NONE;
}
