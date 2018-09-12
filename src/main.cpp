/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#include "../../common/desktop-console/filter_chain.h"
#include "../../common/desktop-console/filter_chain_manager.h"
#include "../../common/desktop-console/config.h"

#include "../../common/rtl/FilesystemLib.h"
#include "../../common/utils/winapi_mapping.h"

#include <iostream>
#include <csignal>
#include <mutex>
#include <condition_variable>

#include <QtCore/QCoreApplication>

std::unique_ptr<CFilter_Chain_Manager> gFilter_Chain_Manager;

void sighandler(int signo)
{
	// SIGINT should terminate filters; this will eventually terminate whole app
	if (signo == SIGINT) {
		glucose::UDevice_Event shut_down_event{ glucose::NDevice_Event_Code::Shut_Down };
		gFilter_Chain_Manager->Send(shut_down_event);
	}		
}

int MainCalling main(int argc, char** argv) {
	QCoreApplication app{ argc, argv };	//needed as we expose qdb connector that uses Qt

	signal(SIGINT, sighandler);

	// create chain holder - it holds CFilter_Chain instance
	gFilter_Chain_Manager = std::make_unique<CFilter_Chain_Manager>();

	// load config and retrieve loaded filter chain
	Configuration.Resolve_And_Load_Config_File(argc > 1 ? std::wstring{ argv[1], argv[1] + strlen(argv[1]) } : std::wstring{});
	Configuration.Load(gFilter_Chain_Manager->Get_Filter_Chain());

	// attempt to initialize and start filters
	HRESULT rc = gFilter_Chain_Manager->Init_And_Start_Filters();
	if (rc != S_OK)
	{
		std::cerr << "Could not initialize filter chain, return code: " << rc << std::endl;
		return 1;
	}

	// wait for filters to finish, or user to close app
	gFilter_Chain_Manager->Join_Filters();

	return 0;
}
