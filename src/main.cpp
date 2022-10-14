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

#include "utils.h"
#include "options.h"
#include "optimize.h"

#include "../../common/rtl/scgmsLib.h"
#include "../../common/rtl/FilterLib.h"
#include "../../common/rtl/FilesystemLib.h"
#include "../../common/rtl/referencedImpl.h"
#include "../../common/rtl/SolverLib.h"
#include "../../common/rtl/UILib.h"
#include "../../common/utils/winapi_mapping.h"
#include "../../common/utils/string_utils.h"

#include <iostream>
#include <csignal>
#include <thread>
#include <climits>
#include <chrono>
#include <cmath>

#ifdef _WIN32
	#include <Windows.h>
#endif



scgms::SFilter_Executor Global_Filter_Executor;
solver::TSolver_Progress Global_Progress = solver::Null_Solver_Progress; //so that we can cancel from sigint


void MainCalling sighandler(int signo) {
	// SIGINT should terminate filters; this will eventually terminate whole app
	if (signo == SIGINT) {
		std::wcout << std::endl << "Cancelling..." << std::endl;

		Global_Progress.cancelled = TRUE;

		if (Global_Filter_Executor) {
			scgms::UDevice_Event shut_down_event{ scgms::NDevice_Event_Code::Shut_Down };
			Global_Filter_Executor.Execute(std::move(shut_down_event));
		}
	}
}

int Execute_Configuration(scgms::SPersistent_Filter_Chain_Configuration configuration, const bool save_config) {
	refcnt::Swstr_list errors;
	Global_Filter_Executor = scgms::SFilter_Executor{ configuration.get(),
#ifndef DDO_NOT_USE_QT
		Setup_Filter_DB_Access
#else
		nullptr
#endif
		,  nullptr, errors };
	errors.for_each([](auto str) { std::wcerr << str << std::endl;	});

	if (!Global_Filter_Executor) {
		std::wcerr << L"Could not execute the filters!" << std::endl;
		return __LINE__;
	}


	// wait for filters to finish, or user to close the app
	Global_Filter_Executor->Terminate(TRUE);


	if (save_config) {
		std::wcout << L"Saving configuration...";
		errors = refcnt::Swstr_list{};
		const HRESULT rc = configuration->Save_To_File(nullptr, errors.get());
		errors.for_each([](auto str) { std::wcerr << str << std::endl; });
		if (!Succeeded(S_OK)) {
			std::wcerr << std::endl << L"Failed to save the configruation!" << std::endl;
			return __LINE__;
		}
		else
			std::wcout << L" saved." << std::endl;
	}


	return 0;
}


int MainCalling main(int argc, char** argv) {

	int result = __LINE__;

#ifndef DDO_NOT_USE_QT
	QCoreApplication app{ argc, argv };	//needed as we expose qdb connector that uses Qt
#endif

	if (!scgms::is_scgms_loaded()) {
		std::wcerr << L"SmartCGMS library is not loaded!" << std::endl;
		return __LINE__;
	}

	signal(SIGINT, sighandler);

	TAction action_to_do = Parse_Options(argc, const_cast<const char**> (argv));
	if (action_to_do.action != NAction::failed_configuration) {
		auto [rc, configuration] = Load_Experimental_Setup(argc, argv, action_to_do.variables);
		if (!Succeeded(rc))
			return __LINE__;


		switch (action_to_do.action) {
			case NAction::execute:
				result = Global_Progress.cancelled == 0 ? Execute_Configuration(configuration, action_to_do.save_config) : __LINE__;
				break;
				

			case NAction::optimize:
				result = Global_Progress.cancelled == 0 ? Optimize_Configuration(configuration, action_to_do, Global_Progress) : __LINE__;
				break;

			default:
				std::wcout << L"Not implemented action requested! Action code: " << static_cast<size_t>(action_to_do.action) << std::endl;
				return __LINE__;
		}


		configuration.reset();	//extraline so that we can take memory snapshot to ease our debugging
	}

	
	return result;	//so that we can nicely set breakpoints to take memory snapshots				
}

	
	
/*
	//Check if we will optimize	
	if (argc > 2) {
		std::wstring arg_3{ Widen_Char(argv[2]) };
		if (arg_3.rfind(dsOptimize_Switch, 0) == 0) {

			//cannot safely optimize, if all filters were not loaded
			if (rc != S_OK) {
				std::wcerr << L"Optimization aborted." << std::endl;
				return __LINE__;
			}

			std::wstring arg_4;
			if (argc > 3) arg_4 = Widen_Char(argv[3]);

			std::wstring arg_5;
			if (argc > 4) arg_5 = Widen_Char(argv[4]);

			OS_rc = Optimize_Configuration(configuration, arg_3, arg_4, arg_5);
			if (OS_rc != 0)
				return OS_rc;
		}
		
		/*else { will have to rework the options later on
			std::wcerr << L"Stopping, encountered unknown option: " << arg_3 << std::endl;
			return __LINE__;
		}
		*/
/*	}
	
	//If we have optimized succesfully, then the best parameters are already saved.
	//Hence, we execute the configuration once more to make the final pass with the best parameters.
	//Or, we just execute it as if no optimization was asked for.

	bool save_config = false;

	for (size_t i = 1; i < argc; i++) {
		if (dsSave_Config_Switch.compare(argv[i]) == 0) {
			save_config = true;
			break;
		}
	}
	
}

*/