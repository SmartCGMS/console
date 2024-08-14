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

#include "optimize.h"

#include "utils.h"
#include <scgms/utils/string_utils.h>
#include <scgms/utils/system_utils.h>

#include <iostream>

int Optimize_Configuration(scgms::SPersistent_Filter_Chain_Configuration configuration, const TAction& action, solver::TSolver_Progress& progress) {

	const size_t optimize_param_count = action.parameters_to_optimize.size();
	if (optimize_param_count < 1) {
		std::wcerr << L"Have no parameters to optimize!\n";
		return __LINE__;
	}

	std::vector<size_t> optimize_param_indices;
	std::vector<const wchar_t*> optimize_param_names;

	for (const auto& param : action.parameters_to_optimize) {
		optimize_param_indices.push_back(param.index);
		optimize_param_names.push_back(param.name.c_str());
	}

	const auto [hint_rc, expected_param_size] = Count_Parameters_Size(configuration, action.parameters_to_optimize);
	if (hint_rc != S_OK) {
		return __LINE__;
	}

	std::vector<std::vector<double>> hints;
	//load hints
	if (!Load_Hints(action.hints_to_load, expected_param_size, false, hints)) {
		return __LINE__;
	}

	//load parameters
	if (!Load_Hints(action.hinting_parameters_to_load, expected_param_size, true, hints)) {
		return __LINE__;
	}

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

		rc = scgms::Optimize_Parameters(configuration,
			optimize_param_indices.data(), optimize_param_names.data(), optimize_param_count,
#ifndef DDO_NOT_USE_QT
			Setup_Filter_DB_Access,
#else
			nullptr,
#endif
			nullptr,
			action.solver_id, action.population_size, action.generation_count,
			hints_ptr.data(), hints_ptr.size(),
			progress, errors);

		optimizing_flag = false;
	});

	double recent_percentage = std::numeric_limits<double>::quiet_NaN();
	solver::TFitness recent_fitness = solver::Max_Fitness;
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

				for (size_t i = 0; i < solver::Maximum_Objectives_Count; i++) {
					const double tmp_best = progress.best_metric[i];
					if ((recent_fitness[i] > tmp_best) && (!std::isnan(tmp_best))) {
						recent_fitness[i] = tmp_best;

						std::wcout << L' ' << i << L':' << tmp_best;
					}
				}

				std::wcout.flush();
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	if (optimitizing_thread.joinable()) {
		optimitizing_thread.join();
	}

	errors.for_each([](auto str) {
		std::wcerr << str << std::endl;
	});

	if (rc == S_OK) {
		std::wcout << L"\nResulting fitness:";
		for (size_t i = 0; i < solver::Maximum_Objectives_Count; i++) {
			std::wcout << L' ' << i << L':' << progress.best_metric[i];
		}

		std::wcout << L"\nParameters were succesfully optimized, saving...";
		errors = refcnt::Swstr_list{};
		rc = configuration->Save_To_File(nullptr, errors.get());
		errors.for_each([](auto str) {
			std::wcerr << str << std::endl;
		});

		if (!Succeeded(S_OK)) {
			std::wcerr << std::endl << L"Failed to save optimized parameters!" << std::endl;
			return __LINE__;
		}
		else {
			std::wcout << L" saved." << std::endl;
		}
	}
	else if (rc == S_FALSE) {
		std::wcerr << L"Solver did not improve the solution." << std::endl;
		return __LINE__;
	}
	else {
		std::wcerr << L"Optimization failed! Error: " << Describe_Error(rc) << std::endl;
		return __LINE__;
	}

	return 0;
}
