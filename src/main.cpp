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
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
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

#include "../../common/rtl/scgmsLib.h"
#include "../../common/rtl/FilterLib.h"
#include "../../common/rtl/FilesystemLib.h"
#include "../../common/rtl/referencedImpl.h"
#include "../../common/utils/winapi_mapping.h"

#ifndef DDO_NOT_USE_QT
	#include "../../common/rtl/qdb_connector.h"
	#include <QtCore/QCoreApplication>
#endif

#include <iostream>
#include <csignal>


scgms::SFilter_Executor gFilter_Executor;

void MainCalling sighandler(int signo) {	
	// SIGINT should terminate filters; this will eventually terminate whole app
	if (signo == SIGINT) {
		if (gFilter_Executor) {
			scgms::UDevice_Event shut_down_event{ scgms::NDevice_Event_Code::Shut_Down };
			gFilter_Executor.Execute(std::move(shut_down_event));
		}
	}
}

int MainCalling main(int argc, char** argv) {	
	if (argc < 2) {
		std::wcerr << L"Usage: ";
		if ((argc>0) && (argv[0]) && (argv[0][0])) std::wcerr << argv[0]; //the standard does permit zero argc and empty argv[0]
			else std::wcerr << L"console";
		std::wcerr << " filename" << std::endl << std::endl;
		std::wcerr << L"The filename designates the configuration of an experimental setup. Usually, it is .ini file." << std::endl;
						
		return 4;
	}

	if (!scgms::is_scgms_loaded()) {
		std::wcerr << L"SmartCGMS library is not loaded!" << std::endl;
		return 3;
	}

#ifndef DDO_NOT_USE_QT
	QCoreApplication app{ argc, argv };	//needed as we expose qdb connector that uses Qt
#endif
	
	signal(SIGINT, sighandler);

	//Let's try to load the configuration file
	const std::wstring config_filepath = argc > 1 ? std::wstring{ argv[1], argv[1] + strlen(argv[1]) } : std::wstring{};
	scgms::SPersistent_Filter_Chain_Configuration configuration;
	
    refcnt::Swstr_list errors;


	HRESULT rc = configuration ? S_OK : E_FAIL;
	if (rc == S_OK) configuration->Load_From_File(config_filepath.c_str(), errors.get());
	errors.for_each([](auto str) { std::wcerr << str << std::endl;	});	

	if (!Succeeded(rc)) {
		std::wcerr << L"Cannot load the configuration file " << config_filepath << std::endl << L"Error code: " << rc << std::endl;        
		return 2;
	}

    if (rc == S_FALSE) {
        std::wcerr << L"Warning: some filters were not loaded!" << std::endl;        
    }
	
	errors = refcnt::Swstr_list{};	//erase any previous error that we already printed
	gFilter_Executor = scgms::SFilter_Executor{ configuration.get(), 
#ifndef DDO_NOT_USE_QT
		Setup_Filter_DB_Access
#else
		nullptr
#endif
		,  nullptr, errors };
	errors.for_each([](auto str) { std::wcerr << str << std::endl;	});

	if (!gFilter_Executor)
	{
		std::wcerr << L"Could not execute the filters!" << std::endl;    
		return 1;
	}


	// wait for filters to finish, or user to close the app
	gFilter_Executor->Terminate(TRUE);

    

	return 0;
}
