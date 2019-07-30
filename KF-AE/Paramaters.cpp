#include "KFMovieMaker.h"
#include "Parameters.h"
#include "SequenceData.h"
#include "OS.h"

#include <string>
#include <experimental/filesystem>  //Experimental as of VC2017 c++17

std::array<int, static_cast<int>(ParameterID::__last)> paramTranslate; ///To translate param ID to location
int paramsAdded {1};  ///Counter used to track parameters as they are added.

static void AddButton(ParameterID id, const char * name, const char * buttonText, PF_ParamFlags flags = 0, PF_ParamUIFlags ui_flags = 0);
static void AddSlider(ParameterID id, const char * name, float min, float max, float sliderMin, float sliderMax, float value, short decimals = 2, PF_ParamFlags flags = 0);
static void AddDropDown(ParameterID id, const char * name, const char * choices, short value = 1, PF_ParamFlags flags = 0);
static void AddCheckBox(ParameterID id, const char * name, const char * comment, bool value, PF_ParamFlags flags = 0);
static void AddGroupStart(ParameterID id, const char * name, PF_ParamFlags flags = 0);
static void AddGroupEnd(ParameterID id);
static void AddColourPicker(ParameterID id, const char * name, unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0, PF_ParamFlags flags = 0);
inline long useParam(ParameterID p);



/*******************************************************************************************************
Setup Parameters
*******************************************************************************************************/
PF_Err ParameterSetup(PF_InData	*in_data, PF_OutData *out_data, PF_ParamDef	*params[], PF_LayerDef *output) {
	PF_Err err {PF_Err_NONE};

	PF_ParamDef	def;
	AEFX_CLR_STRUCT(def);
	paramTranslate.fill(0);
	paramsAdded = 1;	//Effect world is parameter number 0.

	
	AddButton(ParameterID::fileSelectButton, "File Location", "Browse", PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY);
	AddSlider(ParameterID::keyFrameNumber, "Key Frame", 0, 9999999 ,0, 1, 0, PF_Precision_TEN_THOUSANDTHS);
	AddDropDown(ParameterID::scalingMode, "Render Method", "Use Cached Frames|Frame by Frame", 1, PF_ParamFlag_CANNOT_TIME_VARY);
	AddGroupStart(ParameterID::topic_start_colour, "Colours (Outside)");
	AddDropDown(ParameterID::colourMethod, "Colour Method", "Standard (.kfr Colours)|Distance Estimation (.kfr Colours)|(-|Black and White Wave",1, PF_ParamFlag_CANNOT_TIME_VARY );
	AddDropDown(ParameterID::modifier, "Modifier", "Linear|Square Root|Cubic Root|Logarithm", 1);
	AddSlider(ParameterID::colourDivision, "Iteration Division", 0, 1000,0,1000, 1, PF_Precision_TEN_THOUSANDTHS);
	AddCheckBox(ParameterID::smooth, "Smooth Colouring", "", true);
	AddSlider(ParameterID::colourOffset, "Colour Offset", 0, 1024, 0, 1024, 0, PF_Precision_TENTHS);
	AddSlider(ParameterID::distanceClamp, "Distance Clamp", 0, 1024, 0, 1024, 0, PF_Precision_TENTHS);
	AddGroupEnd(ParameterID::topic_start_colour);
	AddGroupStart(ParameterID::topic_start_insideColour, "Colours (Inside)");
	AddColourPicker(ParameterID::insideColour, "Inside Colour");
	AddGroupEnd(ParameterID::topic_end_insideColour);

	out_data->num_params = paramsAdded;
	return err;
}

/*******************************************************************************************************
A helper function to build location to ID table (paramTranslate) and count the parameters as they are added
*******************************************************************************************************/
inline long useParam(ParameterID p) {
	long val = static_cast<long>(p);
	paramTranslate[val] = paramsAdded;
	paramsAdded++;
	return val;
}





/*******************************************************************************************************
File Select Button Clicked
*******************************************************************************************************/
PF_Err FileSelectButtonClicked(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[]) {
	auto fileName = ShowFileOpenDialogKFR();
	if (fileName == "") return PF_Err_NONE;
	
	auto sd = SequenceData::GetSequenceData(in_data);
	if (!sd) return PF_Err_INTERNAL_STRUCT_DAMAGED;
	
	sd->SetFileName(fileName);

	if(sd->Validate()) {
		auto numFiles = sd->getLocalSequenceData()->kfbFiles.size() - 1;
		params[paramTranslate[static_cast<int>(ParameterID::keyFrameNumber)]]->u.fs_d.slider_max = (float) numFiles;
		params[paramTranslate[static_cast<int>(ParameterID::keyFrameNumber)]]->uu.change_flags = true;
		params[paramTranslate[static_cast<int>(ParameterID::colourDivision)]]->u.fs_d.value = sd->getLocalSequenceData()->kfrIterationDivision;
		params[paramTranslate[static_cast<int>(ParameterID::colourDivision)]]->uu.change_flags = true;
	}else {
		auto numFiles = sd->getLocalSequenceData()->kfbFiles.size() - 1;
		params[paramTranslate[static_cast<int>(ParameterID::keyFrameNumber)]]->u.fs_d.slider_max = 1;
		params[paramTranslate[static_cast<int>(ParameterID::keyFrameNumber)]]->uu.change_flags = true;
	}

	return PF_Err_NONE;
}

/*******************************************************************************************************
Parameters Changed
For responding to button clicks
*******************************************************************************************************/
PF_Err ParameterChanged(PF_InData	*in_data, PF_OutData *out_data, PF_ParamDef	*params[], const PF_UserChangedParamExtra *paramExtra) {
	//Perform actions for specific parameters.
	auto paramChanged = static_cast<ParameterID>(paramExtra->param_index);
	switch (paramChanged){
		case ParameterID::fileSelectButton:
			out_data->out_flags |= PF_OutFlag_FORCE_RERENDER;
			return FileSelectButtonClicked(in_data, out_data, params);
			
		default:
			break;
	}
	
	return PF_Err_NONE;
}

/*******************************************************************************************************
Create a Button parameter  (only to be called during ParamaterSetup event)
Note: Safer than the AE macro. 
*******************************************************************************************************/
static void AddButton(ParameterID id, const char * name, const char * buttonText, PF_ParamFlags flags , PF_ParamUIFlags ui_flags) {
	PF_ParamDef	def {};	//Must be zero initialised.
	
	def.param_type = PF_Param_BUTTON;
	strncpy_s(def.name, name, sizeof(def.name));
	def.uu.id = static_cast<long>(id);
	def.flags = flags;
	def.ui_flags = ui_flags;
	def.u.button_d.u.namesptr = buttonText;

	auto err = globalTL_in_data->inter.add_param(globalTL_in_data->effect_ref, -1, &def);
	if(err) throw(err);
	useParam(id);
	return;
}


/*******************************************************************************************************
Create a Button parameter  (only to be called during ParamaterSetup event)
Note: Safer than the AE macro.
*******************************************************************************************************/
static void AddSlider(ParameterID id, const char * name, float min, float max, float sliderMin, float sliderMax, float value, short decimals,  PF_ParamFlags flags) {
	PF_ParamDef	def {};	//Must be zero initialised.
	
	def.param_type = PF_Param_FLOAT_SLIDER;
	strncpy_s(def.name, name, sizeof(def.name));
	def.uu.id = static_cast<long>(id);
	def.flags = flags;
	def.u.fs_d.valid_min = min;   //minimum value of input
	def.u.fs_d.valid_max = max;   //maximum value of input
	def.u.fs_d.slider_min = sliderMin;  //minimum value of slider control (value can be less)
	def.u.fs_d.slider_max = sliderMax;  //maximum value of slider control (value can be higher)
	def.u.fs_d.value = value;
	def.u.fs_d.dephault = value;
	def.u.fs_d.precision = decimals;

	auto err = globalTL_in_data->inter.add_param(globalTL_in_data->effect_ref, -1, &def);
	if(err) throw(err);
	useParam(id);
	return;
}

/*******************************************************************************************************
Create a DropDown (Popup) parameter  (only to be called during ParamaterSetup event)
First value is 1 (not zero).
Note: Safer than the AE macro.
*******************************************************************************************************/
static void AddDropDown(ParameterID id, const char * name, const char * choices, short value, PF_ParamFlags flags) {
	PF_ParamDef	def {};	//Must be zero initialised.
	short choiceCount = 1;
	for(int i = 0; choices[i] != 0; i++) if(choices[i] == '|') choiceCount++;

	def.param_type = PF_Param_POPUP;
	strncpy_s(def.name, name, sizeof(def.name));
	def.flags = flags;
	def.uu.id = static_cast<long>(id);
	def.u.pd.dephault = value;  
	def.u.pd.value = value; //1 based index
	def.u.pd.num_choices = choiceCount;
	def.u.pd.u.namesptr = choices;
	
	auto err = globalTL_in_data->inter.add_param(globalTL_in_data->effect_ref, -1, &def);
	if(err) throw(err);
	useParam(id);
	return;
}

/*******************************************************************************************************
Create a Checkbox parameter  (only to be called during ParamaterSetup event)
Note: Safer than the AE macro.
*******************************************************************************************************/
static void AddCheckBox(ParameterID id, const char * name, const char * comment, bool value, PF_ParamFlags flags) {
	PF_ParamDef	def {};	//Must be zero initialised.
	def.param_type = PF_Param_CHECKBOX;
	strncpy_s(def.name, name, sizeof(def.name));
	def.uu.id = static_cast<long>(id);
	def.flags = flags;
	def.u.bd.u.nameptr = comment;
	def.u.bd.dephault = value;
	def.u.bd.value = value;

	auto err = globalTL_in_data->inter.add_param(globalTL_in_data->effect_ref, -1, &def);
	if(err) throw(err);
	useParam(id);
	return;
}

/*******************************************************************************************************
Create a Group Start parameter  (only to be called during ParamaterSetup event)
Note: Safer than the AE macro.
*******************************************************************************************************/
static void AddGroupStart(ParameterID id, const char * name, PF_ParamFlags flags ) {
	PF_ParamDef	def {};	//Must be zero initialised.

	def.param_type = PF_Param_GROUP_START;
	strncpy_s(def.name, name, sizeof(def.name));
	def.uu.id = static_cast<long>(id);
	def.flags = flags;
	

	auto err = globalTL_in_data->inter.add_param(globalTL_in_data->effect_ref, -1, &def);
	if(err) throw(err);
	useParam(id);
	return;
}

/*******************************************************************************************************
Create a Group End parameter  (only to be called during ParamaterSetup event)
*******************************************************************************************************/
static void AddGroupEnd(ParameterID id) {
	PF_ParamDef	def {};	//Must be zero initialised.

	def.param_type = PF_Param_GROUP_END;
	def.uu.id = static_cast<long>(id);

	auto err = globalTL_in_data->inter.add_param(globalTL_in_data->effect_ref, -1, &def);
	if(err) throw(err);
	useParam(id);
	return;
}
/*******************************************************************************************************
Create aColour parameter  (only to be called during ParamaterSetup event)
*******************************************************************************************************/
static void AddColourPicker(ParameterID id, const char * name, unsigned char red, unsigned char green, unsigned char blue,  PF_ParamFlags flags) {
	PF_ParamDef	def {};	//Must be zero initialised.
	def.param_type = PF_Param_COLOR;
	strncpy_s(def.name, name, sizeof(def.name));
	def.uu.id = static_cast<long>(id);
	def.flags = flags;
	def.u.cd.value.red = red;
	def.u.cd.value.green = green;
	def.u.cd.value.blue = blue;
	def.u.cd.value.alpha = 255; //not selectable.
	def.u.cd.dephault = def.u.cd.value;

	auto err = globalTL_in_data->inter.add_param(globalTL_in_data->effect_ref, -1, &def);
	if(err) throw(err);
	useParam(id);
	return;
}

/*******************************************************************************************************
Reads the value of a float slider parameter.  
Uses check-in/check-out, so this is ok to use during a smart-render call.
*******************************************************************************************************/
double readFloatSliderParam(PF_InData *in_data, ParameterID paramID) {
	auto parameterLocation = paramTranslate[static_cast<int> (paramID)];
	if(parameterLocation == 0) throw ("Invalid request in readFloatSliderParam");
	PF_ParamDef param;
	AEFX_CLR_STRUCT(param);
	in_data->inter.checkout_param(in_data->effect_ref, parameterLocation, in_data->current_time, in_data->time_step, in_data->time_scale, &param);
	double value = param.u.fs_d.value;
	in_data->inter.checkin_param(in_data->effect_ref, &param);
	return value;
}

/*******************************************************************************************************
Reads the value of a list (popup) parameter.
Uses check-in/check-out, so this is ok to use during a smart-render call.
*******************************************************************************************************/
long readListParam(PF_InData *in_data, ParameterID paramID) {
	auto parameterLocation = paramTranslate[static_cast<int> (paramID)];
	if(parameterLocation == 0) throw ("Invalid request in readListParam");
	PF_ParamDef param;
	AEFX_CLR_STRUCT(param);
	in_data->inter.checkout_param(in_data->effect_ref, parameterLocation, in_data->current_time, in_data->time_step, in_data->time_scale, &param);
	long value = param.u.pd.value;
	in_data->inter.checkin_param(in_data->effect_ref, &param);
	return value;
}

/*******************************************************************************************************
Reads the value of a check box parameter.
Uses check-in/check-out, so this is ok to use during a smart-render call.
*******************************************************************************************************/
bool readCheckBoxParam(PF_InData *in_data, ParameterID paramID) {
	auto parameterLocation = paramTranslate[static_cast<int> (paramID)];
	if(parameterLocation == 0) throw ("Invalid request in readCheckBoxParam");
	PF_ParamDef param;
	AEFX_CLR_STRUCT(param);
	in_data->inter.checkout_param(in_data->effect_ref, parameterLocation, in_data->current_time, in_data->time_step, in_data->time_scale, &param);
	auto value = param.u.pd.value;
	in_data->inter.checkin_param(in_data->effect_ref, &param);
	return value !=0;
}

/*******************************************************************************************************

*******************************************************************************************************/
RGB readColour(PF_InData *in_data, ParameterID paramID) {
	auto parameterLocation = paramTranslate[static_cast<int> (paramID)];
	if(parameterLocation == 0) throw ("Invalid request in readColour");
	PF_ParamDef param;
	AEFX_CLR_STRUCT(param);
	in_data->inter.checkout_param(in_data->effect_ref, parameterLocation, in_data->current_time, in_data->time_step, in_data->time_scale, &param);
	RGB value;
	value.red = param.u.cd.value.red;
	value.green = param.u.cd.value.green;
	value.blue = param.u.cd.value.blue;
	in_data->inter.checkin_param(in_data->effect_ref, &param);
	return value;
}