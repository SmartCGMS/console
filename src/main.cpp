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



scgms::SFilter_Executor gFilter_Executor;
solver::TSolver_Progress progress = solver::Null_Solver_Progress; //so that we can cancel from sigint


void MainCalling sighandler(int signo) {
	// SIGINT should terminate filters; this will eventually terminate whole app
	if (signo == SIGINT) {
		std::wcout << std::endl << "Cancelling..." << std::endl;

		progress.cancelled = true;

		if (gFilter_Executor) {
			scgms::UDevice_Event shut_down_event{ scgms::NDevice_Event_Code::Shut_Down };
			gFilter_Executor.Execute(std::move(shut_down_event));
		}
	}
}

int Execute_Configuration(scgms::SPersistent_Filter_Chain_Configuration configuration, const bool save_config) {
	refcnt::Swstr_list errors;
	gFilter_Executor = scgms::SFilter_Executor{ configuration.get(),
#ifndef DDO_NOT_USE_QT
		Setup_Filter_DB_Access
#else
		nullptr
#endif
		,  nullptr, errors };
	errors.for_each([](auto str) { std::wcerr << str << std::endl;	});

	if (!gFilter_Executor) {
		std::wcerr << L"Could not execute the filters!" << std::endl;
		return __LINE__;
	}


	// wait for filters to finish, or user to close the app
	gFilter_Executor->Terminate(TRUE);


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



int Optimize_Configuration(scgms::SPersistent_Filter_Chain_Configuration configuration, std::wstring arg_3, std::wstring arg_4, const std::wstring &arg_5) {



	size_t optimize_filter_index = std::numeric_limits<size_t>::max();
	std::wstring parameters_name;

	const auto delim_pos = arg_3.find(L',');
	if (delim_pos != std::wstring::npos) {
		arg_3[delim_pos] = 0;
		optimize_filter_index = str_2_int(arg_3.data() + wcslen(dsOptimize_Switch));
		parameters_name = &arg_3[delim_pos + 1];


		std::vector<std::vector<double>> hints = Load_Hints(arg_5);
		std::vector<const double*> hints_ptr;
		for (size_t i = 0; i < hints.size(); i++) {
			hints_ptr.push_back(hints[i].data());
		}

		refcnt::Swstr_list errors;

		CPriority_Guard priority_guard;

		HRESULT rc = E_FAIL;
		std::atomic<bool> optimizing_flag{ true };
		std::thread optimitizing_thread([&] {
			//use thread, not async because that could live-lock on a uniprocessor

			auto [generation_count, population_size, solver_id] = Get_Solver_Parameters(arg_4);

			rc = scgms::Optimize_Parameters(configuration,
				optimize_filter_index, parameters_name.c_str(),
#ifndef DDO_NOT_USE_QT
				Setup_Filter_DB_Access
#else
				nullptr
#endif
				, nullptr,
				solver_id, population_size, generation_count,
				hints_ptr.data(), hints_ptr.size(),				
				progress, errors);

			optimizing_flag = false;
			});

		double recent_percentage = std::numeric_limits<double>::quiet_NaN();
		solver::TFitness recent_fitness = solver::Nan_Fitness;
		std::wcout << "Will report progress and best fitness. Optimizing...";
		std::wcout.flush();
		while (optimizing_flag) {
			if (progress.max_progress != 0) {
				double current_percentage = static_cast<double>(progress.current_progress) / static_cast<double>(progress.max_progress);
				current_percentage = std::trunc(current_percentage * 1000.0);
				current_percentage *= 0.1;
				current_percentage = std::min(current_percentage, 100.0);

				if (recent_percentage != current_percentage) {
					recent_percentage = current_percentage;
					std::wcout << " " << current_percentage << "%...";

					
					for (size_t i = 0; i < solver::Maximum_Objectives_Count; i++) {
						const double tmp_best = progress.best_metric[i];
						if ((recent_fitness[i] != tmp_best) && (!std::isnan(tmp_best))) {
							recent_fitness[i] = progress.best_metric[i];

							std::wcout << L' ' << i << L':' << recent_fitness[i];
						}
					}

					std::wcout.flush();
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

		if (optimitizing_thread.joinable())
			optimitizing_thread.join();

		errors.for_each([](auto str) { std::wcerr << str << std::endl;	});

		if (!Succeeded(rc)) {
			std::wcerr << L"Optimization failed!" << std::endl;
			return __LINE__;
		}

		if (rc == S_OK) {
			std::wcout << L"Parameters were succesfully optimized, saving...";
			errors = refcnt::Swstr_list{};
			rc = configuration->Save_To_File(nullptr, errors.get());
			errors.for_each([](auto str) { std::wcerr << str << std::endl; });
			if (!Succeeded(S_OK)) {
				std::wcerr << std::endl << L"Failed to save optimized parameters!" << std::endl;
				return __LINE__;
			}
			else
				std::wcout << L" saved." << std::endl;
		}

	}
	else {
		std::wcerr << L"Optimize specified, but wrong parameters given. Aborting..." << std::endl;
		return __LINE__;
	}

	return 0;
}


int MainCalling main(int argc, char** argv) {
#ifndef DDO_NOT_USE_QT
	QCoreApplication app{ argc, argv };	//needed as we expose qdb connector that uses Qt
#endif

	signal(SIGINT, sighandler);

	int OS_rc = POST_Check(argc, argv);
	if (OS_rc != 0) 
		return OS_rc;
	
	auto [rc, configuration] = Load_Experimental_Setup(argc, argv);
	if (!Succeeded(rc))
		return __LINE__;
	

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
	}
	
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
	const auto result = progress.cancelled == 0 ? Execute_Configuration(configuration, save_config) : __LINE__;
	configuration.reset();	//extraline so that we can take memory snapshot to ease our debugging
	return result;	//so that we can nicely set breakpoints to take memory snapshots
}
