/********************************************************************************************
KFBData.cpp

Author:			Adam Sakareassen

Credits:		KeyFrameMovieMaker from the Kalles Fraktaler project was consulted for the KFB
				file format.

Description:	Holds the contents of a .kfb file.
				.kfb file includes iteration data, smooth data, and colour data.
				Has handels to hold (and destroy) a chached pre-rendered image of this data.
				This class will clean-up chache handles and its own memory allocations.

Licence:		GNU Affero General Public License

********************************************************************************************
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
********************************************************************************************/

#include "KFBData.h"
#include "os.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>

inline long clampToLong(float d, long max);
inline float BiLinearIterpolation(float x, float y, float ul, float ur, float ll, float lr);
inline float biCubicIterpolation(const float values[][4], float x, float y);



/*******************************************************************************************************
Constuctor.
Gets AE managed memory (non-zerod).
*******************************************************************************************************/
KFBData::KFBData( int w, int h)
{
	AEGP_SuiteHandler suites(globalTL_in_data->pica_basicP);
	auto handleSuite = suites.HandleSuite1();
	if(!handleSuite) throw(std::exception("Unable to aquire HandleSuite1"));

	//Note we request data and smoothData as one block of memory.
	memWidth = w + paddingSize * 2;
	memHeight = h + paddingSize * 2;


	long dataSize = sizeof(int)* memWidth * memHeight;
	this->memSize = dataSize + sizeof(float)* memWidth * memHeight;
	this->width = w;
	this->height = h;
	
	this->handle = handleSuite->host_new_handle(memSize);
	if (!this->handle) throw(PF_Err_OUT_OF_MEMORY);
	this->data = reinterpret_cast<int*>(handleSuite->host_lock_handle(this->handle));
	if (!this->data)  throw(PF_Err_OUT_OF_MEMORY);
	
	//Ugly pointer math to get a pointer to the smoothData (which is the 2nd part of the mem block)
	char * c = reinterpret_cast<char*>(this->data);
	c += dataSize;
	this->smoothData = reinterpret_cast<float*>(c);
	
	
	
}
/*******************************************************************************************************
Deconstuctor.
Dispose of AE managed memory buffer.
*******************************************************************************************************/
KFBData::~KFBData()
{
	DebugMessage("~KFBData()\n");
	AEGP_SuiteHandler suites(globalTL_in_data->pica_basicP);
	auto handleSuite = suites.HandleSuite1();
	if(!handleSuite) return;

	handleSuite->host_dispose_handle(this->handle);
	smoothData = nullptr;
	data = nullptr;
	handle = nullptr;
	
	DisposeOfCache();
}

/*******************************************************************************************************
Releases the cached image for this kfb data.
*******************************************************************************************************/
void KFBData::DisposeOfCache() {
	if(!this) return;  
	AEGP_SuiteHandler suites(globalTL_in_data->pica_basicP);
	if(this->isImageCached && this->cachedImageAEGP) {
		suites.WorldSuite3()->AEGP_Dispose(this->cachedImageAEGP);
	}
	this->isImageCached = false;
}




void KFBData::ReadKFBFile(std::string fileName) {
	//Readfile
	std::ifstream file {fileName, std::ios::binary | std::ios::in};
	if(!file) throw (std::exception("Unable to open KFB file\n"));

	//Check ID
	char id[3];
	file.read(id, 3);
	if(!(id[0] == 'K' && id[1] == 'F' && id[2] == 'B')) throw (std::exception("KFB file has invalid ID\n"));


	//Read Size
	int h, w;
	file.read((char *)&w, sizeof(width));
	file.read((char *)&h, sizeof(height));
	if(w != this->width || h != this->height) throw (std::exception("KFB file has incorrect size\n"));
	if(w*h * sizeof(int) != dataSize()) throw (std::exception("Array size incorrect to read KFB file\n"));

	//Read Iteration Data (also rotate, because KFB data is sideways)
	auto data = this->getIterationData();
	for(long x = 0; x < width; x++) {
		for(long y = 0; y < height; y++) {
			auto index = makeIndex(x + paddingSize, y + paddingSize);
			file.read((char*) &data[index], sizeof(int));
		}
	}
	

	//Read Colour information
	file.read((char *)&this->colourDiv, sizeof(int));
	file.read((char *)&this->numColours, sizeof(int));
	if(this->numColours > 1024) throw(std::exception("Number of KFB colours invalid."));
	for(unsigned int i = 0; i < this->numColours; i++) {
		file.read((char *)&this->kfbColours[i].red, sizeof(char));
		file.read((char *)&this->kfbColours[i].green, sizeof(char));
		file.read((char *)&this->kfbColours[i].blue, sizeof(char));
	}

	//Read max iterations
	file.read((char *)&this->maxIterations, sizeof(int));
	

	//Read (raw) smooth data;
	auto smooth = this->getSmoothData();
	float temp;
	for(long x = 0; x < width; x++) {
		for(long y = 0; y < height; y++) {
			file.read((char*)&temp, sizeof(temp));
			const auto index = makeIndex(x + paddingSize, y + paddingSize);
			smooth[index] = static_cast<float>(data[index]) + 1 - temp;
		}
	}
	


	//Assign extapolated values to padded iteration values.
	for(int x = 0; x < memWidth; x++) {
		//Pad top
		auto edge = data[makeIndex(x, 2)];
		auto diff = edge  - data[makeIndex(x, 3)];
		data[makeIndex(x, 1)] = std::max(edge + diff, 0);
		data[makeIndex(x, 0)] = std::max(edge + (diff * 2), 0);
		
		//Pad Bottom
		edge = data[makeIndex(x, memHeight -3)];
		diff = edge - data[makeIndex(x, memHeight-4)];
		data[makeIndex(x, memHeight -2)] = std::max(edge + diff, 0);
		data[makeIndex(x, memHeight -1)] = std::max(edge + (diff * 2), 0);
	}
	for(int y = 0; y < memHeight; y++) {
		//Pad left
		auto edge = data[makeIndex(2,y)];
		auto diff = edge - data[makeIndex(3,y)];
		data[makeIndex(1, y)] = std::max(edge + diff, 0);
		data[makeIndex(0, y)] = std::max(edge + (diff * 2), 0);

		//Pad right
		edge = data[makeIndex(memWidth - 3,y)];
		diff = edge - data[makeIndex(memWidth - 4,y)];
		data[makeIndex(memWidth - 2,y)] = std::max(edge + diff, 0);
		data[makeIndex(memWidth - 1,y)] = std::max(edge + (diff * 2), 0);
	}

	//Assign extapolated values to padded smooth values.
	for(int x = 0; x < memWidth; x++) {
		//Pad top
		auto edge = smoothData[makeIndex(x, 2)];
		auto diff = edge - smoothData[makeIndex(x, 3)];
		smoothData[makeIndex(x, 1)] = std::fmax(edge + diff, 0.0f);
		smoothData[makeIndex(x, 0)] = std::fmax(edge + (diff * 2), 0.0f);

		//Pad Bottom
		edge = smoothData[makeIndex(x, memHeight - 3)];
		diff = edge - smoothData[makeIndex(x, memHeight - 4)];
		smoothData[makeIndex(x, memHeight - 2)] = std::fmax(edge + diff, 0.0f);
		smoothData[makeIndex(x, memHeight - 1)] = std::fmax(edge + (diff * 2), 0.0f);
	}
	for(int y = 0; y < memHeight; y++) {
		//Pad left
		auto edge = smoothData[makeIndex(2, y)];
		auto diff = edge - smoothData[makeIndex(3, y)];
		smoothData[makeIndex(1, y)] = std::fmax(edge + diff, 0.0f);
		smoothData[makeIndex(0, y)] = std::fmax(edge + (diff * 2), 0.0f);

		//Pad right
		edge = smoothData[makeIndex(memWidth - 3, y)];
		diff = edge - smoothData[makeIndex(memWidth - 4, y)];
		smoothData[makeIndex(memWidth - 2, y)] = std::fmax(edge + diff, 0.0f);
		smoothData[makeIndex(memWidth - 1, y)] = std::fmax(edge + (diff * 2), 0.0f);
	}

}

/*******************************************************************************************************
Calculates an iteration value, given decimal (x,y) values.
The value is the weighted value of the surrounding pixels.
Returns the nearest edge pixel if requeted pixel is out of bounds
*******************************************************************************************************/
float KFBData::calculateIterationCountBiCubic(float x, float y, bool smooth) {
	x += paddingSize;
	y += paddingSize;
	const float floorX = std::floor(x);
	const float floorY = std::floor(y);
	const long xl = clampToLong(floorX, width - 1);
	const long yl = clampToLong(floorY, height - 1);
	
	//If we are passed an integer pixel, no need to Interpolate. (always the case building cache)
	if(floorX == x && floorY == y) {
		const long index = calcIndexAndClamp(xl, yl);
		return (smooth) ? smoothData[index] : static_cast<float>(data[index]);
	}
	
	//Get 16 surrounding pixels.
	float values[4][4];
	if(smooth) {
		for(int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) values[i][j] = smoothValue(xl + i - 1, yl + j - 1);
	}
	else {
		for(int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) values[i][j] = static_cast<float>(smoothValue(xl + i - 1, yl + j - 1));
	}
	

	return biCubicIterpolation(values, x, y);
	
}

/*******************************************************************************************************
Calculates an iteration value, given decimal (x,y) values.
The value is the weighted value of the surrounding pixels.
Returns the nearest edge pixel if requeted pixel is out of bounds
//Note: Only Smooth
*******************************************************************************************************/
inline float KFBData::calculateIterationCountBiLinear(float x, float y) {
	return calculateIterationCountBiLinearNoPad(x + paddingSize, y + paddingSize);
}


/*******************************************************************************************************
BiLinear with values already padded.
*******************************************************************************************************/
float KFBData::calculateIterationCountBiLinearNoPad(float x, float y) {
	const float floorX = std::floor(x);
	const float floorY = std::floor(y);
	const long xl = clampToLong(floorX, memWidth - 1);
	const long yl = clampToLong(floorY, memHeight - 1);

	//If we are passed an integer pixel, no need to Interpolate. (always the case building cache)
	if(floorX == x && floorY == y) {
		return smoothValue(xl, yl);
	}
	float ul, ur, ll, lr;

	ul = smoothValue(xl, yl);
	ur = smoothValue(xl + 1, yl);
	ll = smoothValue(xl, yl + 1);
	lr = smoothValue(xl + 1, yl + 1);


	return BiLinearIterpolation(x, y, ul, ur, ll, lr);
}

/*******************************************************************************************************
Gets a matrix of 9 pixel values surrounding (x,y)
If "minimal", we just calculate a cross, not all 9 values.
*******************************************************************************************************/
void  KFBData::getDistanceMatrix(float p[][3], float x, float y, float step, bool minimal) {
	x += paddingSize;
	y += paddingSize;
	auto xMinusStep = std::max(0.0f, x - step);
	auto xPlusStep = std::min(static_cast<float>(memWidth - 1), x + step);
	auto yMinusStep = std::max(0.0f, y - step);
	auto yPlusStep = std::min(static_cast<float>(memHeight - 1), y + step);

	
	p[1][0] = calculateIterationCountBiLinearNoPad(x, yMinusStep);
	p[0][1] = calculateIterationCountBiLinearNoPad(xMinusStep, y);
	p[1][1] = calculateIterationCountBiLinearNoPad(x, y);
	p[2][1] = calculateIterationCountBiLinearNoPad(xPlusStep, y);
	p[1][2] = calculateIterationCountBiLinearNoPad(x, yPlusStep);
	
	if(!minimal) {
		p[0][0] = calculateIterationCountBiLinearNoPad(xMinusStep, yMinusStep);
		p[2][0] = calculateIterationCountBiLinearNoPad(xPlusStep, yMinusStep);
		p[0][2] = calculateIterationCountBiLinearNoPad(xMinusStep, yPlusStep);
		p[2][2] = calculateIterationCountBiLinearNoPad(xPlusStep, yPlusStep);
	}


	//Mirror to correct edge pixel
	if(step > 1.0f) {
		if(x - step < 0) {
			p[0][1] = p[2][1];
			if(!minimal) {
				p[0][0] = p[2][0];
				p[0][2] = p[2][2];
			}
		}
		if(x + step > memWidth - 1) {
			p[2][1] = p[0][1];
			if(!minimal) {
				p[2][0] = p[0][0];
				p[2][2] = p[0][2];
			}

		}
		if(y - step < 0) {
			p[1][0] = p[1][2];
			if(!minimal) {
				p[0][0] = p[0][2];
				p[2][0] = p[2][2];
			}
		}
		if(y + step > memHeight - 1) {
			p[1][2] = p[1][0];
			if(!minimal) {
				p[0][2] = p[0][0];
				p[2][2] = p[2][0];
			}
		}
	}

}


/*******************************************************************************************************
Get's the iteration count at co-ordinates (x,y)
Returns boundry pixel if out of bounds
*******************************************************************************************************/
int KFBData::getIterationCount(long x, long y) {
	x += paddingSize;
	y += paddingSize;
	auto index = calcIndexAndClamp(x, y);
	return data[index];
}

/*******************************************************************************************************
Get's the iteration count at co-ordinates (x,y)
Returns boundry pixel if out of bounds
*******************************************************************************************************/
double KFBData::getIterationCountSmooth(long x, long y) {
	x += paddingSize;
	y += paddingSize;
	auto index = calcIndexAndClamp(x, y);
	return smoothData[index];
}



/*******************************************************************************************************
Computes the by linear interpolation.
Given a floating point co-ordinate (x,y), computes the weighted averge of the 4 neighbouring values.
(ul = upper-left vale, ur = upper right value, etc.)
*******************************************************************************************************/
inline float BiLinearIterpolation(float x, float y, float ul, float ur, float ll, float lr) {
	float xfloor = std::floor(x);
	float x1, x2;
	if(x == xfloor) {
		x1 = ul;
		x2 = ll;
	}else {
		auto xCeil = std::ceil(x);
		x1 = (xCeil - x) *ul + (x - xfloor)*ur;
		x2 = (xCeil - x) *ll + (x - xfloor)*lr;
	}
	
	auto yfloor = std::floor(y);
	float result;
	if(y == yfloor) {
		result = x1;
	}
	else {
		result = (std::ceil(y) - y) *x1 + (y - yfloor)*x2;
	}
	
	return result;
}


/*******************************************************************************************************
Clamps a value between 0 and max.
*******************************************************************************************************/
inline long clamp(long v, long max) {
	return (v < 0) ? 0 : ((v > max) ? max : v);
	
}
/******************************************************************************************************
Clamp value from 0 to max (val will be converted to a long)
*******************************************************************************************************/
inline long clampToLong(float d, long max) {
	long v = static_cast<long>(d);
	return (v < 0) ? 0 : ((v > max) ? max : v);
}



/******************************************************************************************************
Calculate the index to the KFB buffer.
The value will be clamped within range.

*******************************************************************************************************/



/******************************************************************************************************
Calculate one cubic line.  
Given 4 consecutative pixel values (v0..v3)
Returns a weighted value between v1 and v2 based on the offset from v1.
v0 and v3 are used to build a cubic spline to caclulate the results.
*******************************************************************************************************/
inline float biCubicStep(float v0, float v1, float v2, float v3, float offset) {
	float a = (-v0 / 2) + (3 * v1) / 2 - (3 * v2) / 2 + (v3 / 2);
	float b = v0 - (5 * v1) / 2 + (2 * v2) - (v3 / 2);
	float c = -v0 / 2 + v2 / 2;
	float d = v1;
	return a* std::pow(offset, 3) + b * std::pow(offset, 2) + c* offset + d;
}

/*inline float biCubicStep(float v0, float v1, float v2, float v3, float offset) {
float a = (v3 - v2) - (v0 - v1);
float b = (v0 - v1) - a;
float c = v2-v0;
float d = v1;
return a*std::pow(offset, 3) + b * std::pow(offset, 2) + c*offset + d;
}*/

/******************************************************************************************************
Use 16 points to calculate the Bicubic Interpolarion of the pixel (x,y)
*******************************************************************************************************/
inline float biCubicIterpolation(const float values[][4], float x, float y) {
	float xOffset = x - std::floor(x);
	float yOffset = y - std::floor(y);
	float y1 = biCubicStep(values[0][0], values[0][1], values[0][2], values[0][3], yOffset);
	float y2 = biCubicStep(values[1][0], values[1][1], values[1][2], values[1][3], yOffset);
	float y3 = biCubicStep(values[2][0], values[2][1], values[2][2], values[2][3], yOffset);
	float y4 = biCubicStep(values[3][0], values[3][1], values[3][2], values[3][3], yOffset);
	float result = biCubicStep(y1, y2, y3, y4, xOffset);
	return result;
}












