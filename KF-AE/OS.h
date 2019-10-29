#pragma once
/********************************************************************************************
OS Specific functions

Author:			(c) 2019 Adam Sakareassen

Description:	Contains functions that rely on Windows.
				To port to another OS implement functions in OS.h in a seperate CPP file.
				Note: No checks for Endian type are included in any files.

Licence:		GNU Affero General Public License

********************************************************************************************
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
********************************************************************************************/
#include <string>

void DebugMessage(const std::string & str) noexcept;
void ShowMessageBox(const std::string & str);
std::string ShowFileOpenDialogKFR();