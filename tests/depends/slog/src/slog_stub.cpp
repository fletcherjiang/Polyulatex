/**
* you may not use this file except in compliance with the license
* you may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
**/

#include "toolchain/slog.h"
#include "toolchain/plog.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>


void dlog_init(){}

int dlog_getlevel(int module_id, int *enable_event){ 
	return 0;
}

void DlogErrorInner(int module_id, const char *fmt, ...){}

void DlogWarnInner(int module_id, const char *fmt, ...){}

void DlogInfoInner(int module_id, const char *fmt, ...){}

void DlogDebugInner(int module_id, const char *fmt, ...){}

void DlogEventInner(int module_id, const char *fmt, ...){}

int CheckLogLevel(int moduleId, int logLevel) { 
	return 1; 
}

int DlogReportFinalize() { 
	return 0;
}

int DlogReportInitialize() { 
	return 0; 
}
