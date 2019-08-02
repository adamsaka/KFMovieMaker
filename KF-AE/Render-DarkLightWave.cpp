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
#include "Render-DarkLightWave.h"
#include "LocalSequenceData.h"
#include "Render.h"
constexpr float sinScaleFactor = 1; 

/*******************************************************************************************************
Render a pixel at 8-bit colour depth.
*******************************************************************************************************/
PF_Err Render_DarkLightWave::Render8(void * refcon, A_long x, A_long y, PF_Pixel8 * in, PF_Pixel8 * out) {
	constexpr float sinScaleFactor = 10; //Prevent from been too dense;
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);
	double iCount = GetBlendedPixelValue(local, x, y);
	//Check for inside pixel
	if(iCount >= local->activeKFB->maxIterations)  return SetInsideColour8(local, out);
	
	iCount = doModifier(local->modifier, iCount);
	

	iCount = std::sin((iCount / sinScaleFactor) / local->colourDivision);
	iCount = (iCount + 1) / 2; //scaled from 0.0 to 1.0;
	iCount += local->colourOffset;
		
	auto val = roundTo8Bit(iCount * white8);
	
	out->red = val;
	out->blue = val;
	out->green = val;
	out->alpha = white8;
	return PF_Err_NONE;
	
}

/*******************************************************************************************************
Render a pixel at 16-bit colour depth.
*******************************************************************************************************/
PF_Err Render_DarkLightWave::Render16(void * refcon, A_long x, A_long y, PF_Pixel16 * in, PF_Pixel16 * out) {

	auto local = reinterpret_cast<LocalSequenceData*>(refcon);
	double iCount = GetBlendedPixelValue(local, x, y);
	//Check for inside pixel
	if(iCount >= local->activeKFB->maxIterations)  return SetInsideColour16(local, out);

	iCount = std::sin((iCount / sinScaleFactor) / local->colourDivision) ;
	iCount = (iCount +1)/ 2;  //scaled from 0.0 to 1.0;
	iCount += local->colourOffset;
	
	auto val = roundTo16Bit(iCount * white16);

	out->red = val;
	out->blue = val;
	out->green = val ;
	out->alpha = white16;
	return PF_Err_NONE;
	
}

/*******************************************************************************************************
Render a pixel at 32-bit colour depth.
*******************************************************************************************************/
PF_Err Render_DarkLightWave::Render32(void * refcon, A_long x, A_long y, PF_Pixel32 * iP, PF_Pixel32 * out) {
	auto local = reinterpret_cast<LocalSequenceData*>(refcon);
	double iCount = GetBlendedPixelValue(local, x, y);
	//Check for inside pixel

	if(iCount >= local->activeKFB->maxIterations)  return SetInsideColour32(local, out);
	iCount = std::sin((iCount / sinScaleFactor) / local->colourDivision);
	iCount = (iCount + 1) / 2;  //scaled from 0.0 to 1.0;
	iCount += local->colourOffset;
	
	auto val = static_cast<float>(iCount);
	out->red = val;
	out->blue = val;
	out->green = val;
	out->alpha = white32;
	return PF_Err_NONE;
	
}
