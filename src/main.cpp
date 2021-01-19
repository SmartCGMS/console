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
#include "../../common/rtl/SolverLib.h"
#include "../../common/rtl/UILib.h"
#include "../../common/utils/winapi_mapping.h"
#include "../../common/utils/string_utils.h"

 /*
  *	If you do not need database access, or do not want to use Qt, then
  *  #define DDO_NOT_USE_QT
  */

#ifndef DDO_NOT_USE_QT
	#include "../../common/rtl/qdb_connector.h"
	#include <QtCore/QCoreApplication>
#endif

#include <iostream>
#include <csignal>
#include <thread>
#include <climits>
#include <chrono>
#include <cmath>

#ifdef _WIN32
	#include <Windows.h>
#endif


class CPriority_Guard {
public:
	CPriority_Guard() {
#ifdef _WIN32
		if (SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS))
			std::wcout << L"Process priority lowered to BELOW_NORMAL." << std::endl;
#endif
	}
	
	~CPriority_Guard() {
#ifdef _WIN32		
		if (SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS))
			std::wcout << L"Process priority restored to NORMAL." << std::endl;
#endif
	}
};



const wchar_t* dsOptimize_Switch = L"/optimize=";
const wchar_t* dsSolver_Switch = L"/solver=";

scgms::SFilter_Executor gFilter_Executor;
solver::TSolver_Progress progress = solver::Null_Solver_Progress; //so that we can cancel from sigint


int Execute_Configuration(scgms::SPersistent_Filter_Chain_Configuration configuration) {
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

	return 0;
}

std::tuple<size_t, size_t, GUID> Get_Solver_Parameters(std::wstring arg_4) {
	
	constexpr GUID Halton_Meta_DE = { 0x1274b08, 0xf721, 0x42bc, { 0xa5, 0x62, 0x5, 0x56, 0x71, 0x4c, 0x56, 0x85 } };
	constexpr size_t Population_Size = 100;
	constexpr size_t Generation_Count = 1000;

	std::tuple<size_t, size_t, GUID> result;
	std::get<0>(result) = Generation_Count;
	std::get<1>(result) = Population_Size;
	std::get<2>(result) = Halton_Meta_DE;

	if (arg_4.rfind(dsSolver_Switch, 0) == 0) {
		arg_4.erase(0, wcslen(dsSolver_Switch));

		auto delim_pos = arg_4.find(L',');
		if (delim_pos != std::wstring::npos) 
			arg_4[delim_pos] = 0;
				
		bool ok;
		size_t num = static_cast<size_t>(str_2_int(arg_4.c_str(), ok));
		if (ok) {
			std::get<0>(result) = num;

			if ((delim_pos != std::wstring::npos) && (delim_pos+1<arg_4.size())) {		
				arg_4 = arg_4.data() + delim_pos+1;
				delim_pos = arg_4.find(L',');
				if (delim_pos != std::wstring::npos)
					arg_4[delim_pos] = 0;

				size_t num = static_cast<size_t>(str_2_int(arg_4.c_str(), ok));
				if (ok) {
					std::get<1>(result) = num;

						if ((delim_pos != std::wstring::npos) && (delim_pos + 1 < arg_4.size())) {
							arg_4 = arg_4.data() + delim_pos + 1;
								GUID id = WString_To_GUID(trim(arg_4), ok);
								if (ok)
									std::get<2>(result) = id;
								else
									std::wcerr << L"Failed to convert " << arg_4 << L". Using default value " << GUID_To_WString(std::get<2>(result)) << "." << std::endl;
						}
				} else
					std::wcerr << L"Failed to convert " << arg_4 << L". Using default value " << std::get<1>(result) << "." << std::endl;
			}
		}
		else
			std::wcerr << L"Failed to convert " << arg_4 << L". Using default value " << std::get<0>(result) << "." << std::endl;

		
	}

	return result;
}

int Optimize_Configuration(scgms::SPersistent_Filter_Chain_Configuration configuration, std::wstring arg_3, std::wstring arg_4) {

	

	size_t optimize_filter_index = std::numeric_limits<size_t>::max();
	std::wstring parameters_name;

	const auto delim_pos = arg_3.find(L',');
	if (delim_pos != std::wstring::npos) {
		arg_3[delim_pos] = 0;
		optimize_filter_index = str_2_int(arg_3.data() + wcslen(dsOptimize_Switch));
		parameters_name = &arg_3[delim_pos + 1];

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
				solver_id, population_size, generation_count, progress, errors);

			optimizing_flag = false;
		});

		double recent_percentage = std::numeric_limits<double>::quiet_NaN();
		double recent_fitness = std::numeric_limits<double>::max();
		std::wcout << "Will report progress and best fitness. Optimizing...";
		while (optimizing_flag) {
			if (progress.max_progress != 0) {
				double current_percentage = static_cast<double>(progress.current_progress) / static_cast<double>(progress.max_progress);
				current_percentage = std::trunc(current_percentage * 1000.0);
				current_percentage *= 0.1;
				current_percentage = std::min(current_percentage, 100.0);

				if (recent_percentage != current_percentage) {
					recent_percentage = current_percentage;
					std::wcout << " " << current_percentage << "%...";

					if (recent_fitness > progress.best_metric) {
						recent_fitness = progress.best_metric;
						std::wcout << " " << recent_fitness;
					}
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
			errors.for_each([](auto str) { std::wcerr << str << std::endl;});
			if (!Succeeded(S_OK)) {
				std::wcerr << std::endl << L"Failed to save optimized parameters!" << std::endl;
				return __LINE__;
			} else
				std::wcout << L" saved." << std::endl;
		}

	}
	else {
		std::wcerr << L"Optimize specified, but wrong parameters given. Aborting..." << std::endl;
		return __LINE__;
	}

	return 0;
}

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

int POST_Check(int argc, char** argv) {
	if (argc < 2) {
		std::wcout << L"Usage: ";
		if ((argc > 0) && (argv[0]) && (argv[0][0])) std::wcout << argv[0]; //the standard does permit zero argc and empty argv[0]
		else std::wcout << L"console";
		std::wcout << " filename [" << dsOptimize_Switch << "filter_index,parameters_name[ " <<dsSolver_Switch <<"generations[,population[,solver_GUID]]]]" << std::endl << std::endl;
		std::wcout << L"The filename designates the configuration of an experimental setup. Usually, it is .ini file." << std::endl << std::endl;
		std::wcout << L"The filter_index starts at zero. parameters_name is a string." << std::endl << std::endl;
		std::wcout << L"Generations is the maximum number of generations to evolve/computational steps." << std::endl;
		std::wcout << L"Population is the population size, when applicable, or 0 to allow entering the solver_id." << std::endl;
		std::wcout << L"Solver_GUID is solver's id, formatted like {01274B08-F721-42BC-A562-0556714C5685}." << std::endl;
		std::wcout << L"Make no spaces around the , delimiters." << std::endl;
		std::wcout << L"Default solver is Halton-driven Meta-Differential Evolution, 1000 generations, 100 population size." << std::endl;

		std::wcout << std::endl << L"Available solvers:" << std::endl;

		
		for (const auto& solver : scgms::get_solver_descriptors()) 
			if (!solver.specialized) 
				std::wcout << GUID_To_WString(solver.id) << " - " << solver.description << std::endl;

		return __LINE__;
	}

	if (!scgms::is_scgms_loaded()) {
		std::wcerr << L"SmartCGMS library is not loaded!" << std::endl;
		return __LINE__;
	}

	return 0;
}

std::tuple<HRESULT, scgms::SPersistent_Filter_Chain_Configuration> Load_Experimental_Setup(int argc, char** argv) {
	std::tuple<HRESULT, scgms::SPersistent_Filter_Chain_Configuration> result;

	//Let's try to load the configuration file
	const std::wstring config_filepath = argc > 1 ? std::wstring{ argv[1], argv[1] + strlen(argv[1]) } : std::wstring{};
	scgms::SPersistent_Filter_Chain_Configuration configuration;

	refcnt::Swstr_list errors;


	HRESULT rc = configuration ? S_OK : E_FAIL;
	if (rc == S_OK) configuration->Load_From_File(config_filepath.c_str(), errors.get());
	errors.for_each([](auto str) { std::wcerr << str << std::endl;	});

	if (!Succeeded(rc))
		std::wcerr << L"Cannot load the configuration file " << config_filepath << std::endl << L"Error code: " << rc << std::endl;	

	if (rc == S_FALSE)
		std::wcerr << L"Warning: some filters were not loaded!" << std::endl;

	std::get<0>(result) = rc;
	std::get<1>(result) = std::move(configuration);

	return result;
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
			OS_rc = Optimize_Configuration(configuration, arg_3, arg_4);
			if (OS_rc != 0)
				return OS_rc;
		}
	}
	
	//If we have optimized succesfully, then the best parameters are already saved.
	//Hence, we execute the configuration once more to make the final pass with the best parameters.
	//Or, we just execute it as if no optimization was asked for.

	return progress.cancelled == 0 ? Execute_Configuration(configuration) : __LINE__;
}
