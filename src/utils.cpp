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




const wchar_t* dsOptimize_Switch = L"/optimize=";
const wchar_t* dsSolver_Switch = L"/solver=";
const wchar_t* dsHints_Switch = L"/hints=";
const std::string dsSave_Config_Switch = "/save_configuration";

std::vector<std::vector<double>> Load_Hints(const std::wstring& arg_5) {
	std::vector<std::vector<double>> result;

	if (arg_5.find_first_of(dsHints_Switch) == 0) {
		const auto fname = arg_5.substr(wcslen(dsHints_Switch));
		std::wifstream hints_file{ Narrow_WString(fname) };
		if (hints_file) {
			std::wstring line;

			while (std::getline(hints_file, line)) {
				bool ok = false;
				if (!line.empty())
					result.push_back(std::move(str_2_dbls(line.c_str(), ok)));

				if (!ok)
					std::wcerr << L"Skipped a possibly corrupted parameters line!" << std::endl;
			}


			std::wcout << L"Loaded " << result.size() << " additional hints from " << fname << std::endl;

		} else
			std::wcerr << L"Cannot open the hints file!" << std::endl;
	}

	return result;
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



int POST_Check(int argc, char** argv) {
	if (argc < 2) {
		std::wcout << L"Usage: ";
		if ((argc > 0) && (argv[0]) && (argv[0][0])) std::wcout << argv[0]; //the standard does permit zero argc and empty argv[0]
		else std::wcout << L"console";
		std::wcout << " filename [" << dsOptimize_Switch << "filter_index,parameters_name[ " <<dsSolver_Switch <<"generations[,population[,solver_GUID]]]] [" << Widen_String(dsSave_Config_Switch) << "]" << std::endl << std::endl;
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

