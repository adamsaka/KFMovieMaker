#pragma once
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
#include "render.h"

class Render_DEAndAngle {
	public:
	static PF_Err Render8(void *refcon, A_long x, A_long y, PF_Pixel8 *in, PF_Pixel8 *out);
	static PF_Err Render16(void *refcon, A_long x, A_long y, PF_Pixel16 *in, PF_Pixel16 *out);
	static PF_Err Render32(void *refcon, A_long x, A_long y, PF_Pixel32 *in, PF_Pixel32 *out);
};
#pragma once
