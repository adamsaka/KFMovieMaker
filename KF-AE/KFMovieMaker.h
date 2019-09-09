#pragma once
/********************************************************************************************
KF Movie Maker

Author:		Adam Sakareassen

Credits:	The Adobe After Effects SDK was consulted while creating this file.

Licence:	GNU Affero General Public License

********************************************************************************************
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
********************************************************************************************/

#ifndef KFMOVIEMAKER_H
#define KFMOVIEMAKER_H

//Adobe SDK Setup.  All the AdobeSDK is included by including this header
typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;
#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096
#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels; 
#include "AEConfig.h"
#include "../AfterEffectsSDK/Examples/Util/entry.h"
#include "../AfterEffectsSDK/Examples/Headers/AE_Effect.h"
#include "../AfterEffectsSDK/Examples/Headers/AE_EffectCB.h"
#include "../AfterEffectsSDK/Examples/Headers/AE_Macros.h"
#include "../AfterEffectsSDK/Examples/Util/Param_Utils.h"
#include "../AfterEffectsSDK/Examples/Headers/AE_EffectCBSuites.h"
#include "../AfterEffectsSDK/Examples/Util/String_Utils.h"
#include "../AfterEffectsSDK/Examples/Headers/AE_GeneralPlug.h"
#include "../AfterEffectsSDK/Examples/Util/AEFX_ChannelDepthTpl.h"
#include "../AfterEffectsSDK/Examples/Util/AEGP_SuiteHandler.h"

/* Versioning information */
#define	MAJOR_VERSION	1
#define	MINOR_VERSION	0
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1

//Main Entry Point
extern "C" {
	DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output, void *extra);
}

extern thread_local PF_InData * globalTL_in_data;

constexpr int maxKFRColours = 1024;
struct RGB {
	unsigned char red {0};
	unsigned char green {0};
	unsigned char blue {0};

	RGB() {};
	RGB(unsigned char r, unsigned char g, unsigned char b) {
		red = r;
		green = g;
		blue = b;
	}
	bool operator==(const RGB& rhs) {
		return (red == rhs.red) && (green == rhs.green) && (blue == rhs.blue);
	}
};

struct RGBdouble {
	double red {0};
	double green {0};
	double blue {0};

	RGBdouble() {};
	RGBdouble(double r, double g, double b) {
		red = r;
		green = g;
		blue = b;
	}
	bool operator==(const RGBdouble& rhs) {
		return (red == rhs.red) && (green == rhs.green) && (blue == rhs.blue);
	}
};


//Wrapper struct around an effectWorld and handle to ensure it is released.
struct WorldHolder {
	AEGP_WorldH handle {nullptr};
	PF_EffectWorld effectWorld {};
	unsigned short bitDepth {0};
	~WorldHolder() {
		Destroy();
	}

	void Destroy() {
		if(handle) {
			AEGP_SuiteHandler suites(globalTL_in_data->pica_basicP);
			suites.WorldSuite3()->AEGP_Dispose(handle);
			handle = nullptr;
			AEFX_CLR_STRUCT(effectWorld);
			bitDepth = 0;
		}
	}
};


#endif