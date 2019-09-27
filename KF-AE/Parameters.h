#pragma once
/********************************************************************************************
Parameters.h

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

//ParameterID contains the IDs assigned internally by After Effects when saving projects.
//Note: The AE SDK usually gives back the location rather than the ID, so it needs translating.
enum class ParameterID : long {
	input = 0,	//Reserve ID zero for AE.
	fileSelectButton,
	keyFrameNumber,
	colourDivision,
	colourMethod,
	modifier,
	smooth,
	topic_start_colour,
	topic_end_colour,
	topic_start_slopes,
	topic_end_slopes,
	scalingMode,
	topic_start_insideColour,
	topic_end_insideColour,
	insideColour,
	distanceClamp,
	colourOffset,
	slopesEnabled,
	slopeShadowDepth,
	slopeStrength,
	slopeAngle,
	colourCycle,
	slopeMethod,
	layerSample,
	samplingOn,
	__last,  //Must be last (used for array memory allocation)
};


PF_Err ParameterSetup(PF_InData	*in_data, PF_OutData *out_data, PF_ParamDef	*params[], PF_LayerDef *output);
PF_Err ParameterChanged(PF_InData	*in_data, PF_OutData *out_data, PF_ParamDef	*params[], const PF_UserChangedParamExtra	*paramExtra);
double readFloatSliderParam(PF_InData * in_data, ParameterID parameterNumber);
long readListParam(PF_InData * in_data, ParameterID parameterNumber);
bool readCheckBoxParam(PF_InData * in_data, ParameterID parameterNumber);
RGB readColourParam(PF_InData * in_data, ParameterID paramID);
long readLayerParamIndex(PF_InData * in_data, ParameterID paramID);
double readAngleParam(PF_InData * in_data, ParameterID paramID);
