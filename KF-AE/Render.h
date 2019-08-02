#pragma once
/********************************************************************************************
Render.h

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
#include "KFMovieMaker.h"
#include "Parameters.h"
#include "LocalSequenceData.h"

constexpr unsigned char black8 = 0;
constexpr unsigned char white8 = 0xff;
constexpr unsigned short black16 = 0;
constexpr unsigned short white16 = 0x8000;
constexpr float black32 = 0.0;
constexpr float white32 = 1.0;
constexpr double pi = 3.14159265358979323846;
constexpr int colourRange = 1024;

PF_Err SmartPreRender(PF_InData * in_data, PF_OutData * out_data, PF_PreRenderExtra* preRender);
PF_Err SmartRender(PF_InData * in_data, PF_OutData * out_data,  PF_SmartRenderExtra* smartRender);
double doModifier(long modifier, double it);
void GetBlendedDistanceMatrix(float matrix[][3], const LocalSequenceData * local, A_long x, A_long y);
double GetBlendedPixelValue(const LocalSequenceData* local, A_long x, A_long y);
void doSlopes(float p[][3], const LocalSequenceData * local, double & r, double & g, double & b);
void getDistanceIntraFrame(float p[][3], A_long x, A_long y, const LocalSequenceData * local, bool minimal = false);
PF_Err NonSmartRender(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef	*output);
unsigned char roundTo8Bit(double f);
unsigned short roundTo16Bit(double f);
void GetColours(const LocalSequenceData* local, double iCount, RGB & highColour, RGB & lowColour, double & mixWeight);
PF_Err SetInsideColour8(const LocalSequenceData * local, PF_Pixel8 * out);
PF_Err SetInsideColour16(const LocalSequenceData * local, PF_Pixel16 * out);
PF_Err SetInsideColour32(const LocalSequenceData * local, PF_Pixel32 * out);