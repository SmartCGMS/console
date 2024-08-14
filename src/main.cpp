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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "utils.h"
#include "options.h"
#include "optimize.h"

#include <scgms/rtl/scgmsLib.h>
#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/FilesystemLib.h>
#include <scgms/rtl/referencedImpl.h>
#include <scgms/rtl/SolverLib.h>
#include <scgms/rtl/UILib.h>
#include <scgms/utils/winapi_mapping.h>
#include <scgms/utils/string_utils.h>

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

//so that we can cancel from sigint
solver::TSolver_Progress Global_Progress = solver::Null_Solver_Progress;

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
		Setup_Filter_DB_Access,
#else
		nullptr,
#endif
		nullptr, errors
	};

	errors.for_each([](auto str) {
		std::wcerr << str << std::endl;
	});

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
		errors.for_each([](auto str) {
			std::wcerr << str << std::endl;
		});

		if (!Succeeded(rc)) {
			std::wcerr << std::endl << L"Failed to save the configuration!" << std::endl;
			std::wcerr << std::endl << L"Error 0x" << std::hex << rc << std::dec << ": "  << Describe_Error(rc) << std::endl;
			return __LINE__;
		}
		else {
			std::wcout << L" saved." << std::endl;
		}
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
		if (!Succeeded(rc)) {
			return __LINE__;
		}

		switch (action_to_do.action) {
			case NAction::execute:
				result = Global_Progress.cancelled == 0 ? Execute_Configuration(configuration, action_to_do.save_config) : __LINE__;
				break;
			case NAction::optimize:
				result = Global_Progress.cancelled == 0 ? Optimize_Configuration(configuration, action_to_do, Global_Progress) : __LINE__;
				break;
			default:
				std::wcout << L"Not-implemented action requested! Action code: " << static_cast<size_t>(action_to_do.action) << std::endl;
				return __LINE__;
		}

		//extra line so that we can take memory snapshot to ease our debugging
		configuration.reset();
	}

	//so that we can nicely set breakpoints to take memory snapshots
	return result;
}
