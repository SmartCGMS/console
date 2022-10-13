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

#include <fstream>

#include "../../common/utils/string_utils.h"

CPriority_Guard::CPriority_Guard() {
#ifdef _WIN32
	if (SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS))
		std::wcout << L"Process priority lowered to BELOW_NORMAL." << std::endl;
#endif
}
	
CPriority_Guard::~CPriority_Guard() {
#ifdef _WIN32		
	if (SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS))
		std::wcout << L"Process priority restored to NORMAL." << std::endl;
#endif
}


std::vector<std::vector<double>> Load_Hints(const std::wstring &hint_path) {
	std::vector<std::vector<double>> result;

	
	std::wifstream hints_file{ hint_path };
	if (hints_file) {
		std::wstring line;

		while (std::getline(hints_file, line)) {
			bool ok = false;
			if (!line.empty())
				result.push_back(std::move(str_2_dbls(line.c_str(), ok)));

			if (!ok)
				std::wcerr << L"Skipped a possibly corrupted parameters line!" << std::endl;
		}


		std::wcout << L"Loaded " << result.size() << " additional hints from " << hint_path << std::endl;

	} else
		std::wcerr << L"Cannot open the hints file!" << std::endl;
	

	return result;
}



std::tuple<HRESULT, scgms::SPersistent_Filter_Chain_Configuration> Load_Experimental_Setup(int argc, char** argv) {
	std::tuple<HRESULT, scgms::SPersistent_Filter_Chain_Configuration> result;

	//Let's try to load the configuration file
	const std::wstring config_filepath = argc > 1 ? std::wstring{ argv[1], argv[1] + strlen(argv[1]) } : std::wstring{};
	scgms::SPersistent_Filter_Chain_Configuration configuration;

	refcnt::Swstr_list errors;


	HRESULT rc = E_FAIL;		//asssume the worst
	if (configuration) {		//and check whether we have constructed the main config container
		rc = configuration->Load_From_File(config_filepath.c_str(), errors.get());	//and load it if we did
	}
	
	errors.for_each([](auto str) { std::wcerr << str << std::endl;	});

	std::get<0>(result) = rc;

	if (Succeeded(rc)) {
		std::get<1>(result) = std::move(configuration);

		if (rc == S_FALSE)
			std::wcerr << L"Warning: some filters were not loaded, or some variables were not set!" << std::endl;
	} else
		std::wcerr << L"Cannot load the configuration file " << config_filepath << std::endl << L"Error code: " << rc << std::endl;	

	return result;
}

