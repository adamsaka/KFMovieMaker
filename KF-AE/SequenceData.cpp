/********************************************************************************************
Sequence Data

Author: Adam Sakareassen

Licence:		GNU Affero General Public License

Manages the sequence data (ie. per instance data)

The SequenceData struct contains the sequence data that will be sent to the plug-in with each call.
This data will be saved within the After Effects Project

********************************************************************************************
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
********************************************************************************************/
#include "SequenceData.h"
#include "KFMovieMaker.h"
#include "OS.h"

#include <cassert>

/*******************************************************************************************************
After Effects Event Management
AE is telling us to create the sequence data for the first time. 
The plugin has been added to some footage.
We leave the data flat because no file is selected at this stage.
Note: Resetup is called when a project is opened (not this function).
*******************************************************************************************************/
PF_Err SequenceData::SequenceSetup(PF_InData *in_data, PF_OutData *out_data) {
	DebugMessage("Sequence Setup\n");
	AEGP_SuiteHandler suites(globalTL_in_data->pica_basicP);
	auto handleSuite = suites.HandleSuite1();
	if(!handleSuite) throw(std::exception("Unable to aquite HandleSuite1"));

	//Allocate memory for the sequence data structure.  Must be in memory managed by AE.
	PF_Handle handle = handleSuite->host_new_handle(sizeof(SequenceData));
	if (!handle) return PF_Err_OUT_OF_MEMORY;
	auto sequenceData = reinterpret_cast<SequenceData*>(handleSuite->host_lock_handle(handle));
	if (!sequenceData) return PF_Err_OUT_OF_MEMORY;
	new(sequenceData) SequenceData{}; //Intialise in place.
	auto result = sequenceData->Unflatten(in_data);
	
	handleSuite->host_unlock_handle(handle);  //Note: AE locks/unlocks sequence data handles for us.
	if (result != PF_Err_NONE) return result;
	
	out_data->sequence_data = handle;
	return PF_Err_NONE;
}

/*******************************************************************************************************
After Effects Event Management
AE tells us to flatten the sequence data.  Usually for saving the project file.
*******************************************************************************************************/
PF_Err SequenceData::SequenceFlatten(PF_InData *in_data, PF_OutData *out_data) {
	DebugMessage("Sequence Flatten\n");
	AEGP_SuiteHandler suites(globalTL_in_data->pica_basicP);
	auto handleSuite = suites.HandleSuite1();
	if(!handleSuite) throw(std::exception("Unable to aquite HandleSuite1"));

	auto sd = SequenceData::GetSequenceData(in_data);
	if (!sd) return PF_Err_INTERNAL_STRUCT_DAMAGED;
	if (!sd->isFlat) {
		assert(sd->localHandle);
		assert(sd->local);
	
		//Release AE owned local memory (Must call deconstructor manually)
		sd->local->~LocalSequenceData();
		handleSuite->host_dispose_handle(sd->localHandle);
		
		sd->localHandle = nullptr;
		sd->local = nullptr;
		sd->isFlat = true;
	}
	return PF_Err_NONE;
}

/*******************************************************************************************************
After Effects Event Management
AE tells us to setup the sequence again.  Usually after file open, flatten or on duplication.
*******************************************************************************************************/
PF_Err SequenceData::SequenceResetup(PF_InData *in_data, PF_OutData *out_data) {
	auto sd = SequenceData::GetSequenceData(in_data);
	if (!sd) return PF_Err_INTERNAL_STRUCT_DAMAGED;
	
	auto result = sd->SequenceFlatten(in_data, out_data);
	if(result != PF_Err_NONE) return result;

	result = sd->Unflatten(in_data);
	if (result != PF_Err_NONE) return result;

	auto valid = sd->Validate();

	DebugMessage("Sequence Resetup: "); 
	if (valid) DebugMessage(sd->kfrFileName); 
	DebugMessage("\n");

	return PF_Err_NONE;
}

/*******************************************************************************************************
After Effects Event Management
AE tells us to destroy the sequence data
*******************************************************************************************************/
PF_Err SequenceData::SequenceSetdown(PF_InData *in_data, PF_OutData *out_data) {
	DebugMessage("Sequence Shutdown\n");
	return PF_Err_NONE;
}

/*****************************************************************************
(Static) Get the sequence data from the in_data structure
*****************************************************************************/
SequenceData * SequenceData::GetSequenceData(PF_InData	*in_data) {
	if (!in_data) return nullptr;
	auto sd = *(SequenceData **)in_data->sequence_data; //Cast and dereference
	if (sd->confirm[0] != 'M' || sd->confirm[1] != 'T') return nullptr;  //Sanity Check
	return sd;
}

/*****************************************************************************
Checks if all the data is ok to render.
Returns true if struct is in a suitable state for rendering.
Will return false if no file selected or can't be found.
*****************************************************************************/
bool SequenceData::Validate() {
	if (confirm[0] != 'M' || confirm[1] != 'T') return false;  //Just a sanity check
	if (kfrFileName[0] == 0) return false;
	if (!fs::exists(kfrFileName)) return false;
	if (isFlat) return false;
	if (!local) return false;
	if (!localHandle) return false;
	if (!local->readyToRender) return false;
	return true;
}

/*******************************************************************************************************
Set the KFR filename.
Public member function that is called when the user shooses a .kfr file.
*******************************************************************************************************/
void SequenceData::SetFileName(std::string & str)
{
	if (str.length() > maximumFilePathLength-1) {
		ShowMessageBox("The file path name is too long.");
		return;
	}

	//Convert to c-style string because we need flat data for AE.
	#pragma warning(disable: 4996) //VC++ unchecked iterators warning on str.copy (used safely in this case)
	auto length = str.copy(kfrFileName, sizeof(kfrFileName)-1, 0);
	kfrFileName[length] = '\0';

	//Pass the string onto the LocalSequenceData
	if (!this->isFlat && this->local) this->local->SetupFileData(this->kfrFileName);
	
}

/*******************************************************************************************************
Unflatten the local data
Private member function
*******************************************************************************************************/
PF_Err SequenceData::Unflatten(PF_InData *in_data) {
	DebugMessage("Sequence Unflatten\n");
	AEGP_SuiteHandler suites(globalTL_in_data->pica_basicP);
	auto handleSuite = suites.HandleSuite1();
	
	if (!this->isFlat) return PF_Err_NONE;
	assert(this->local == nullptr);

	//Get some AE memory for the local data structure.
	//AEGP_SuiteHandler suites(in_data->pica_basicP);
	this->localHandle = handleSuite->host_new_handle(sizeof(LocalSequenceData));
	if (!this->localHandle) return PF_Err_OUT_OF_MEMORY;
	this->local = reinterpret_cast<LocalSequenceData*>(handleSuite->host_lock_handle(this->localHandle));
	if (!this->local) return PF_Err_OUT_OF_MEMORY;
	new(this->local) LocalSequenceData{}; //Intialise in place.
	//global_handleSuite->host_unlock_handle(this->localHandle);

	this->isFlat = false;
	this->local->SetupFileData(this->kfrFileName);
	return PF_Err_NONE;
}

/*******************************************************************************************************
Get the width of the image (from kfb file)
*******************************************************************************************************/
int SequenceData::getWidth()
{
	if (!this->local || !this->local->readyToRender) return 0;
	return this->local->width;
}
/*******************************************************************************************************
Get the width of the image (from kfb file)
*******************************************************************************************************/
int SequenceData::getHeight()
{
	if (!this->local || !this->local->readyToRender) return 0;
	return this->local->height;
}