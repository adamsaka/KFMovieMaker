/********************************************************************************************
KF Movie Maker

Author:		Adam Sakareassen

Credits:	skeleton.cpp from the Adobe After Effects SDK was used as a reference for this file

Description: Contains the entry points and basic utilities for controlling the plug-in

Licence:	GNU Affero General Public License

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
#include "SequenceData.h"
#include "Render.h"

static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data);
static PF_Err About(PF_InData *in_data, PF_OutData	*out_data);

//Store a copy of the memory access suite so it can be accessed globably to allocate/deallocate memory.
//It changes with each call, much easier to hold globably and only update one copy.

thread_local PF_InData	* globalTL_in_data {nullptr};


/*******************************************************************************************************
Main Entry Point for the plug-in.
All exceptions must be caught to avoid host crashing.
*******************************************************************************************************/
PF_Err EffectMain(PF_Cmd cmd, PF_InData	*in_data, PF_OutData *out_data, PF_ParamDef	*params[], PF_LayerDef *output, void *extra) {
	globalTL_in_data = in_data;
	PF_Err	err = PF_Err_NONE;

	try {
		switch (cmd) {
		case PF_Cmd_ABOUT:
			err = About(in_data, out_data);
			break;

		case PF_Cmd_GLOBAL_SETUP:
			err = GlobalSetup(in_data, out_data);
			break;

		case PF_Cmd_GLOBAL_SETDOWN:
			break;

		case PF_Cmd_PARAMS_SETUP:
			err = ParameterSetup(in_data, out_data, params, output);
			break;
		case PF_Cmd_USER_CHANGED_PARAM:
			err = ParameterChanged(in_data, out_data, params, reinterpret_cast<const PF_UserChangedParamExtra *>(extra));
			break;
		
		case PF_Cmd_SEQUENCE_SETUP:
			err = SequenceData::SequenceSetup(in_data, out_data);
			break;

		case PF_Cmd_SEQUENCE_RESETUP:
			err = SequenceData::SequenceResetup(in_data, out_data);
			break;

		case PF_Cmd_SEQUENCE_FLATTEN:
			err = SequenceData::SequenceFlatten(in_data, out_data);
			SequenceData::SequenceSetdown(in_data, out_data);
			break;

		case PF_Cmd_SEQUENCE_SETDOWN:
			err = SequenceData::SequenceSetdown(in_data, out_data);
			break;

		case PF_Cmd_RENDER:
			err = NonSmartRender(in_data, out_data, params, output);
			break;
		
		case PF_Cmd_SMART_PRE_RENDER:
			SmartPreRender(in_data, out_data,  reinterpret_cast<PF_PreRenderExtra*>(extra));
			break;
		
		case PF_Cmd_SMART_RENDER:
			SmartRender(in_data, out_data, reinterpret_cast<PF_SmartRenderExtra*>(extra));
			break;
		
		}
	}
	catch (PF_Err &thrown_err) {
		err = thrown_err;
	}
	catch (std::exception ex) {
		strncpy_s(out_data->return_msg, ex.what(), 256);
		out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
	}
	/*catch (...) {
		//Catch unknown exceptions here, hopefully this won't happen, but there is no reason to also crash After Effects if we generate an exception.
			err = PF_Err_INTERNAL_STRUCT_DAMAGED;
	}*/

	
	return err;
}

/*******************************************************************************************************
Plug-in data
This an en entry point function that might be called by After Effects
Note: Any changes should have a matching change in (.r) resource file.
*******************************************************************************************************/
extern "C" DllExport PF_Err PluginDataEntryFunction(PF_PluginDataPtr inPtr, PF_PluginDataCB inPluginDataCallBackPtr, SPBasicSuite* inSPBasicSuitePtr, const char* inHostName, const char* inHostVersion) {
	PF_Err result = PF_Err_INVALID_CALLBACK;
	result = PF_REGISTER_EFFECT(inPtr, inPluginDataCallBackPtr, "KF Movie Maker", "Maths Town KF Movie Maker", "Maths Town", AE_RESERVED_INFO);
	return result;
}


/*******************************************************************************************************
GlobalSetup
Note: Changes to flags should have a matching change in (.r) resource file.
*******************************************************************************************************/
static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data) {
	out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION,	BUG_VERSION, STAGE_VERSION, BUILD_VERSION);
	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_SEQUENCE_DATA_NEEDS_FLATTENING | PF_OutFlag_PIX_INDEPENDENT;
	out_data->out_flags2 = PF_OutFlag2_SUPPORTS_SMART_RENDER | PF_OutFlag2_FLOAT_COLOR_AWARE;
	return PF_Err_NONE;
}



/*******************************************************************************************************
About dialog to be displayed.
Text to be displayed in out_data->return_msg
*******************************************************************************************************/
static PF_Err About(PF_InData *in_data, PF_OutData	*out_data) {
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg, "%s v%d.%d\r%s", MAJOR_VERSION, "KF Movie Maker");
	return PF_Err_NONE;
}

