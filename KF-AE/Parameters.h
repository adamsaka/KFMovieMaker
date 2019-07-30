#pragma once
enum class ParameterID : long {
	input = 0,
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
	__last,
};


PF_Err ParameterSetup(PF_InData	*in_data, PF_OutData *out_data, PF_ParamDef	*params[], PF_LayerDef *output);
PF_Err ParameterChanged(PF_InData	*in_data, PF_OutData *out_data, PF_ParamDef	*params[], const PF_UserChangedParamExtra	*paramExtra);


double readFloatSliderParam(PF_InData * in_data, ParameterID parameterNumber);

long readListParam(PF_InData * in_data, ParameterID parameterNumber);

bool readCheckBoxParam(PF_InData * in_data, ParameterID parameterNumber);

RGB readColour(PF_InData * in_data, ParameterID paramID);
