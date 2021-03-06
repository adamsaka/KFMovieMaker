#pragma once
/********************************************************************************************
Local Sequence Data

Author: (c) 2019 Adam Sakareassen

Licence:		GNU Affero General Public License

Contains data that is relevent for the current sequence (ie the specific instance of the plug-in)
Contains data that is not saved within the AE Project file.

This class will only be created within a SequenceData object.  It will be created and destroyed as
After Effect flattens and unflattens data. (happens on save, etc). The reason for this is that we
don't want all this working data written back to the project file.  The .kfr and .kfb file data
should be read fresh each time the project is opened.

This class also hold much of the data required by the rendering functions.

********************************************************************************************
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
********************************************************************************************/
#include "KFBData.h"

#include <atomic>
#include <string>
#include <array>
#include <vector>
#include <filesystem>




class LocalSequenceData {
	public:
		bool readyToRender{ false };
		std::string kfrFileName;
		std::vector<std::string> kfbFiles;
		int width { 0 }; ///width of fractal
		int height { 0 };
		int layerWidth{ 0 }; //width of layer.
		int layerHeight{ 0 };

		unsigned int numKFRColours {0};
		std::array<RGB, maxKFRColours> kfrColours;
		double kfrIterationDivision {1};

		//Data specific to rendering this frame
		double scaleFactorX {1};
		double scaleFactorY {1};
		double colourDivision {1};
		long modifier {1};
		long method {1};
		bool useSmooth {true};
		double keyFramePercent {0};
		int scalingMode {1};
		short bitDepth {0};
		RGB insideColour {};
		double distanceClamp {0};
		double colourOffset {0};
		bool slopesEnabled {false};
		double slopeShadowDepth {0};
		double slopeStrength {0};
		double slopeAngle {0};
		double slopeAngleX {0};
		double slopeAngleY {0};
		long slopeMethod {1};
		bool overrideMinimalDistance {false};  ///Get the full matrix, regardless of request because its needed for slopes
		bool sampling {false};
		PF_EffectWorld * layer {nullptr};
		double special {0};
		bool mercator{ false };
		long mercatorMode{ 1 };
		double mercatorRadius{ 1 };

		
		//For sampling functions
		PF_Sampling8Suite1 * sample8 {nullptr};
		PF_Sampling16Suite1 * sample16 {nullptr};
		PF_SamplingFloatSuite1 * sample32 {nullptr};
		PF_InData * in_data {nullptr};

		std::shared_ptr<KFBData> activeKFB {nullptr};
		long activeFrameNumber {-1};
		double activeZoomScale {1};

		std::shared_ptr<KFBData> nextFrameKFB {nullptr};
		long nextFrameNumber {-1};
		double nextZoomScale {2};

		std::shared_ptr<KFBData> thirdFrameKFB{ nullptr };
		long thirdFrameNumber{ -1 };
		std::shared_ptr<KFBData> fourthFrameKFB{ nullptr };
		long fourthFrameNumber{ -1 };
		
		WorldHolder tempImageBuffer;
		WorldHolder tempImageBuffer2;
		
		PF_EffectWorld* mercatorOutput{ nullptr }; //Mercator output image

		LocalSequenceData();

		void SetupFileData(const std::string & fileName);
		void SetupActiveKFB(long keyFrame, PF_InData *in_data);
		void DeleteKFBData();

		///Save a copy of parameters that might invalidate the cache.
		void saveCachedParameters() {
			cache_colourDivision = colourDivision;
			cache_modifier = modifier;
			cache_method = method;
			cache_useSmooth = useSmooth;
			cache_scaleFactorX = scaleFactorX;
			cache_scaleFactorY = scaleFactorY;
			cache_bitDepth = bitDepth;
			cache_insideColour = insideColour;
			cache_distanceClamp = distanceClamp;
			cache_colourOffset = colourOffset;
			cache_slopesEnabled = slopesEnabled;
			cache_slopeShadowDepth = slopeShadowDepth;
			cache_slopeStrength = slopeStrength;
			cache_slopeAngle = slopeAngle;
			cache_slopeMethod = slopeMethod;
			cache_sampling = sampling;
			cache_special = special;
		};

		///Check if any parameters that would invalidate the cache have changed
		bool isCacheInvalid() const {
			if(sampling) return true;
			return !(cache_colourDivision == colourDivision &&
					 cache_modifier == modifier &&
					 cache_method == method &&
					 cache_useSmooth == useSmooth &&
					 cache_scaleFactorX == scaleFactorX &&
					 cache_scaleFactorY == scaleFactorY &&
					 cache_bitDepth == bitDepth &&
					 cache_insideColour == insideColour &&
					 cache_colourOffset == colourOffset &&
					 cache_distanceClamp == distanceClamp &&
					 cache_slopesEnabled == slopesEnabled &&
					 cache_slopeShadowDepth == slopeShadowDepth &&
					 cache_slopeStrength == slopeStrength &&
					 cache_slopeAngle == slopeAngle &&
					 cache_slopeMethod == slopeMethod &&
					 cache_sampling == sampling &&
					 cache_special == special
					
				);
		}
		
private:
		//Store the parameters that make the cahced image (so we can compare them).
		double cache_colourDivision {1};
		long  cache_modifier {1};
		long  cache_method {1};
		bool  cache_useSmooth {true};
		double cache_scaleFactorX {0};
		double cache_scaleFactorY {0};
		short cache_bitDepth {0};
		RGB cache_insideColour {};
		double cache_distanceClamp {0};
		double cache_colourOffset {0};
		bool cache_slopesEnabled {false};
		double cache_slopeShadowDepth {0};
		double cache_slopeStrength {0};
		double cache_slopeAngle {0};
		long cache_slopeMethod {1};
		bool cache_sampling {false};
		double cache_special {0};
		

		void clear();
		void getKFBlist();
		void getKFBStats();
		void LoadKFB(std::shared_ptr<KFBData> & data, long keyFrame);
		void readKFRfile();
		
		

};