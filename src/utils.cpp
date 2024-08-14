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

#include <fstream>

#include <scgms/utils/string_utils.h>

void Load_Hints(const std::wstring &hint_path, const size_t expected_parameters_size, const bool parameters_file_type, std::vector<std::vector<double>> &hints_container) {

	std::wifstream hints_file{ filesystem::path{ hint_path } };
	if (hints_file) {
		std::wstring line;

		const size_t initial_hint_count = hints_container.size();

		size_t line_counter = 0;
		while (std::getline(hints_file, line)) {
			line_counter++;

			bool ok = false;
			if (!line.empty()) {
				auto loaded_hint = str_2_dbls(line.c_str(), ok);

				if (parameters_file_type) {
					//loaded parameters also contain lower and upper bounds, which we need to strip off
					ok = loaded_hint.size() == 3 * expected_parameters_size;
					if (ok) {
						loaded_hint.erase(loaded_hint.begin(), loaded_hint.begin() + expected_parameters_size);	//remove lower bounds
						loaded_hint.resize(expected_parameters_size);	//trim off upper bounds
					}
				} 
				else {
					ok = (loaded_hint.size() == expected_parameters_size);
				}

				if (ok) {
					hints_container.push_back(loaded_hint);
				}
				else {
					std::wcerr << L"Line no. " << line_counter << " contains a hint with a different than expected size.\n";
				}
			}

			if (!ok) {
				std::wcerr << L"Skipped a possibly corrupted parameters line!" << std::endl;
			}
		}

		std::wcout << L"Loaded " << hints_container.size() - initial_hint_count << " additional hints from " << hint_path << std::endl;

	}
	else {
		std::wcerr << L"Cannot open the hints file!" << std::endl;
	}
}


bool Load_Hints(const std::vector<std::wstring>& hint_paths, const size_t expected_parameters_size, const bool parameters_file_type, std::vector<std::vector<double>>& hints_container) {

	const auto current_dir = filesystem::current_path();

	for (const auto& path_mask : hint_paths) {

		const filesystem::path full_path{ Make_Absolute_Path(path_mask, current_dir) };

		//First, we need to ensure that we are not dealing with a uniquely identified file name
		if (Is_Regular_File_Or_Symlink(path_mask)) {
			Load_Hints(path_mask, expected_parameters_size, parameters_file_type, hints_container);
		}
		else {
			//if not, then we are asked to enumerate entire directory, may be with a mask

			const auto effective_path = full_path.parent_path();	//note that path_mask may have already contained a different, than current directory!

			if (Is_Directory(effective_path)) {

				const std::wstring wildcard = path_mask;
				constexpr bool case_sensitive =
#ifdef  _WIN32
					false
#else
					true
#endif   
					;

				std::error_code ec;
				if (effective_path.empty() || (!filesystem::exists(effective_path, ec) || ec)) {
					return false;
				}

				for (auto& enumerated_path : filesystem::directory_iterator(effective_path)) {
					const bool matches_wildcard = Match_Wildcard(enumerated_path.path().filename().wstring(), path_mask, case_sensitive);

					if (matches_wildcard) {
						if (Is_Regular_File_Or_Symlink(enumerated_path)) {
							Load_Hints(enumerated_path.path().wstring(), expected_parameters_size, parameters_file_type, hints_container);
						}
					}
				}
			}
		}
	}

	return true;
}

std::tuple<HRESULT, scgms::SPersistent_Filter_Chain_Configuration> Load_Experimental_Setup(int argc, char** argv, const std::vector<TVariable> &variables) {
	std::tuple<HRESULT, scgms::SPersistent_Filter_Chain_Configuration> result;

	//Let's try to load the configuration file
	const std::wstring config_filepath = argc > 1 ? std::wstring{ argv[1], argv[1] + strlen(argv[1]) } : std::wstring{};
	scgms::SPersistent_Filter_Chain_Configuration configuration;

	refcnt::Swstr_list errors;

	HRESULT rc = E_FAIL;		//asssume the worst
	if (configuration) {		//and check whether we have constructed the main config container
		rc = configuration->Load_From_File(config_filepath.c_str(), errors.get());	//and load it if we did
	}

	errors.for_each([](auto str) {
		std::wcerr << str << std::endl;
	});

	std::get<0>(result) = rc;

	if (Succeeded(rc)) {

		//let us set the variables
		if (!variables.empty()) {
			for (const auto& var : variables) {
				rc = configuration->Set_Variable(var.name.c_str(), var.value.c_str());
				if (!Succeeded(rc)) {
					std::wcerr << L"Failed to set variable named " << var.name << ", to a value of " << var.value << std::endl;

					std::get<0>(result) = rc;
				}
			}
		}

		std::get<1>(result) = std::move(configuration);

		if (rc == S_FALSE) {
			std::wcerr << L"Warning: some filters were not loaded, or some variables were not set!" << std::endl;
		}
	}
	else {
		std::wcerr << L"Cannot load the configuration file " << config_filepath << std::endl << L"Error code: " << rc << std::endl;
	}

	return result;
}

std::tuple<HRESULT, size_t> Count_Parameters_Size(scgms::SPersistent_Filter_Chain_Configuration& configuration, const std::vector<TOptimize_Parameter>& parameters) {

	size_t count = 0;

	for (size_t i = 0; i < parameters.size(); i++) {

		scgms::SFilter_Configuration_Link configuration_link_parameters = configuration[parameters[i].index];

		if (!configuration_link_parameters) {
			std::wcout << L"Cannot get configuration link (no. " << i <<") with parameters to optimize.\n";
			return { E_INVALIDARG, 0 };
		}

		std::vector<double> lbound, params, ubound;
		if (!configuration_link_parameters.Read_Parameters(parameters[i].name.c_str(), lbound, params, ubound)) {
			std::wcout << L"Cannot read parameters configuration link no. " << i << ", with parameters " << parameters[i].name << ".\n";
			return { E_FAIL, 0 };
		}

		count += params.size();
	}

	return { S_OK, count };
}
