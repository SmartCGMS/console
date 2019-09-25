/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8
 * 301 00, Pilsen
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "../../common/desktop-console/qdb_connector.h"

#include "../../common/rtl/FilterLib.h"
#include "../../common/rtl/FilesystemLib.h"
#include "../../common/utils/winapi_mapping.h"

#include <iostream>
#include <csignal>


#include <QtCore/QCoreApplication>

glucose::SFilter_Executor gFilter_Executor;

void MainCalling sighandler(int signo) {
	if (!gFilter_Executor) return;

	// SIGINT should terminate filters; this will eventually terminate whole app
	if (signo == SIGINT) {
		glucose::UDevice_Event shut_down_event{ glucose::NDevice_Event_Code::Shut_Down };
		glucose::IDevice_Event* shut_down_event_raw = shut_down_event.get();
		shut_down_event.release();
		gFilter_Executor->Execute(shut_down_event_raw);
	}
}


int MainCalling main(int argc, char** argv) {
	QCoreApplication app{ argc, argv };	//needed as we expose qdb connector that uses Qt

	signal(SIGINT, sighandler);

	//Let's try to load the configuration file
	const std::wstring config_filepath = argc > 1 ? std::wstring{ argv[1], argv[1] + strlen(argv[1]) } : std::wstring{};
	glucose::SPersistent_Filter_Chain_Configuration configuration;
	
	HRESULT rc = configuration->Load_From_File(config_filepath.c_str());
	if (rc != S_OK) {
		std::wcerr << L"Cannot load the configuration file " << config_filepath << std::endl << L"Error code: " << rc << std::endl;
		return 2;
	}
	
	gFilter_Executor = glucose::SFilter_Executor{ configuration, Setup_Filter_DB_Access, nullptr };
	
	if (!gFilter_Executor)
	{
		std::wcerr << L"Could not execute the filters!" << std::endl;
		return 1;
	}

	// wait for filters to finish, or user to close app
	gFilter_Executor->Wait_For_Shutdown_and_Terminate();

	return 0;
}
