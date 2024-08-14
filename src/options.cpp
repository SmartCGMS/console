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

#include "options.h"

#include <scgms/rtl/UILib.h>
#include <scgms/utils/string_utils.h>
#include <scgms/utils/optionparser.h>

#include <iostream>
#include <typeinfo>

using TOption_Index = std::remove_cv<decltype(option::Descriptor::index)>::type;
enum class NOption_Index : TOption_Index {
	invalid = 0, //for zero-terminated padding, must be zero
	unknown, 
	action,
	parameter_to_optimize,
	variable,
	solver_id,
	generation_count,
	population_size,
	save_config,
	hint,
	parameters_hint
};

using TOption_Type = std::remove_cv<decltype(option::Descriptor::type)>::type;
enum class NAction_Type : TOption_Type {
	unused = 0,
	execute_config,
	optimize_config,
};

constexpr option::Descriptor Unknown_Option = {
	static_cast<TOption_Index>(NOption_Index::unknown),
	static_cast<TOption_Type>(NAction_Type::unused),
	"",
	"",
	option::Arg::None,
	"Usage: console3.exe configuration_path [options]\n\nOptions:"
};

constexpr option::Descriptor actExecute = {
	static_cast<TOption_Index>(NOption_Index::action),
	static_cast<TOption_Type>(NAction_Type::execute_config),
	"e",
	"execute",
	option::Arg::None,
	"--execute, -e \t\texecutes the configuratin, exclusive to optimize; default action"
};

constexpr option::Descriptor actOptimize = {
	static_cast<TOption_Index>(NOption_Index::action),
	static_cast<TOption_Type>(NAction_Type::optimize_config),
	"o",
	"optimize",
	option::Arg::None,
	"--optimize, -o \t\tperforms optimization instead of execution"
};

constexpr option::Descriptor actSave = {
	static_cast<TOption_Index>(NOption_Index::save_config),
	static_cast<TOption_Type>(NAction_Type::unused),
	"s",
	"save_configuration",
	option::Arg::None,
	"--save_configuration, -s \t\tsaves the config after execution/optimization"
};

constexpr option::Descriptor actSolver_Id = {
	static_cast<TOption_Index>(NOption_Index::solver_id),
	static_cast<TOption_Type>(NAction_Type::unused),
	"r",
	"solver_id",
	option::Arg::Optional,
	"--solver_id, -r={solver-guid} \t\tselects the desired solver"
};

constexpr option::Descriptor actGeneration_Count = {
	static_cast<TOption_Index>(NOption_Index::generation_count),
	static_cast<TOption_Type>(NAction_Type::unused),
	"g",
	"generation_count",
	option::Arg::Optional,
	"--generation_count, -g=sets the maximum number of generations/iterations for the solver"
};

constexpr option::Descriptor actPopulation_Size = {
	static_cast<TOption_Index>(NOption_Index::population_size),
	static_cast<TOption_Type>(NAction_Type::unused),
	"z",
	"population_size",
	option::Arg::Optional,
	"--population_size, -z=sets the population size/problem stepping for the solver, if applicable"
};

constexpr option::Descriptor actParameter = {
	static_cast<TOption_Index>(NOption_Index::parameter_to_optimize),
	static_cast<TOption_Type>(NAction_Type::unused),
	"p",
	"parameter",
	option::Arg::Optional,
	"--parameter, -p=filter_zero_index,parameter_name - possibly multiple options gives parameters to optimize"
};

constexpr option::Descriptor actVariable = {
	static_cast<TOption_Index>(NOption_Index::variable),
	static_cast<TOption_Type>(NAction_Type::unused),
	"v",
	"variable",
	option::Arg::Optional,
	"--variable, -v=name:=value sets internal variables to possibly complement operating-system variables"
};

constexpr option::Descriptor actHint = {
	static_cast<TOption_Index>(NOption_Index::hint),
	static_cast<TOption_Type>(NAction_Type::unused),
	"h",
	"hint",
	option::Arg::Optional,
	"--hint, -h=file_mask to files containing hints"
};

constexpr option::Descriptor actParameter_Hint = {
	static_cast<TOption_Index>(NOption_Index::parameters_hint),
	static_cast<TOption_Type>(NAction_Type::unused),
	"m",
	"parameters_hint",
	option::Arg::Optional,
	"--parameters_hint, -m=file_mask, but loads single hint from a parameters file"
};

constexpr option::Descriptor Zero_Terminating_Option = {
	static_cast<TOption_Index>(NOption_Index::invalid),
	static_cast<TOption_Type>(NAction_Type::unused),
	nullptr,
	nullptr,
	option::Arg::None,
	nullptr
};

constexpr std::array<option::Descriptor, 12> option_syntax{
	Unknown_Option,
	actExecute,
	actOptimize,
	actSave,
	actSolver_Id,
	actGeneration_Count,
	actPopulation_Size,
	actParameter,
	actVariable,
	actHint,
	actParameter_Hint,
	Zero_Terminating_Option
};

void Show_Help() {
	option::printUsage(std::cout, option_syntax.data());    

	const auto all_desc = scgms::get_solver_descriptor_list();
	if (all_desc.empty()) {
		std::wcout << L"Warning! There's no solver descriptor actually available!" << std::endl;
	}
	else {
		std::wcout << std::endl << L"Available solvers:" << std::endl;
		for (const auto& solver : scgms::get_solver_descriptor_list()) {
			if (!solver.specialized) {
				std::wcout << GUID_To_WString(solver.id) << " - " << solver.description << std::endl;
			}
		}
	}
}

std::vector<std::wstring> Gather_Values(const NOption_Index idx, std::vector<option::Option>& options) {
	std::vector<std::wstring> result;

	for (option::Option* opt = options[static_cast<size_t>(idx)]; opt; opt = opt->next()) {
		const auto a = opt->arg;
		if ((a == nullptr) || (*a == 0)) {
			std::wcerr << "Detected empty value for parameter " << opt->name << ".\n";
		}
		else {
			result.push_back(Widen_Char(a));
		}
	}

	return result;
}

TAction Resolve_Parameters(TAction &known_config, std::vector<option::Option>& options) {
	TAction result = known_config;

	//1. common parameters
	const auto& save_config_arg = options[static_cast<size_t>(NOption_Index::save_config)];
	result.save_config = static_cast<bool>(save_config_arg);

	//2. parameters applicable for optimization
	if (result.action == NAction::optimize) {
		//2.1 let's try to check preferred solver        
		const auto& solver_id_arg = options[static_cast<size_t>(NOption_Index::solver_id)];
		if (solver_id_arg) {
			bool ok = false;
			const GUID solver_id = WString_To_GUID(Widen_Char(solver_id_arg.arg), ok);
			if (!ok) {
				std::wcerr << L"Malformed solver id!" << std::endl;
				const auto all_desc = scgms::get_solver_descriptor_list();
				if (all_desc.empty()) {
					std::wcout << L"Warning! There's no solver descriptor currently available!" << std::endl;
				}
				else {
					std::wcout << L"Pass an id like this " << GUID_To_WString(all_desc[0].id) << std::endl;
				}

				option::printUsage(std::cout, option_syntax.data());
				result.action = NAction::failed_configuration;
				return result;
			}
			else {
				result.solver_id = solver_id;
			}
		}
		else {
			std::wcout << "Solver ID not set, will use the default one. ";
		}

		// solver descriptor scope
		{
			//id looks good, let's try to resolve it    
			scgms::TSolver_Descriptor solver_desc = scgms::Null_Solver_Descriptor;
			const bool ok = scgms::get_solver_descriptor_by_id(result.solver_id, solver_desc);
			if (!ok) {
				std::wcerr << L"Cannot resolve the solver id to a known solver descriptor!" << std::endl;
				result.action = NAction::failed_configuration;
				return result;
			}
			else {
				std::wcout << L"Resolved solver id to: " << solver_desc.description << std::endl;
			}
		}

		//2.2 generation count
		const auto& generation_count_arg = options[static_cast<size_t>(NOption_Index::generation_count)];
		if (generation_count_arg) {
			bool ok = false;
			const size_t generation_count = str_2_uint(generation_count_arg.arg, ok);
			if (ok) {
				result.generation_count = generation_count;
				std::wcout << L"Generation count set to: " << result.generation_count << std::endl;
			}
			else {
				std::wcerr << L"Cannot resolve generation count to a non-negative number!" << std::endl;
				result.action = NAction::failed_configuration;
				return result;
			}
		}
		else {
			std::wcout << L"Generation count not set, will use default value: " << result.generation_count << std::endl;
		}

		//2.3 population_size
		const auto& population_size_arg = options[static_cast<size_t>(NOption_Index::population_size)];
		if (population_size_arg) {
			bool ok = false;
			const size_t population_size = str_2_uint(population_size_arg.arg, ok);
			if (ok) {
				result.population_size = population_size;
				std::wcout << L"Population size set to: " << result.population_size << std::endl;
			}
			else {
				std::wcerr << L"Cannot resolve population size to a non-negative number!" << std::endl;
				result.action = NAction::failed_configuration;
				return result;
			}
		}
		else {
			std::wcout << L"Population size not set, will use default value: " << result.population_size << std::endl;
		}

		//2.4 gather parameters to optimize, must have at least one element
		for (option::Option* opt = options[static_cast<size_t>(NOption_Index::parameter_to_optimize)]; opt; opt = opt->next()) {
			
			bool resolved_ok = false;
			std::wstring str = Widen_Char(opt->arg);

			const auto delim_pos = str.find(L",");

			if (delim_pos != std::wstring::npos) {
				str[delim_pos] = 0;
				TOptimize_Parameter param_desc;
				param_desc.index = str_2_int(str.data(), resolved_ok);
				if (resolved_ok) {
					param_desc.name = str.data() + delim_pos + 1;

					resolved_ok = param_desc.name.size() > 0;
					if (resolved_ok) {
						result.parameters_to_optimize.push_back(param_desc);
					}
				}
			}

			if (!resolved_ok) {
				std::wcerr << L"Cannot resolve a parameter to optimize: " << str << std::endl;
				result.action = NAction::failed_configuration;
				return result;
			}
		}

		//2.5 gather variables to optimize, can be empty
		std::vector<std::wstring> vars = Gather_Values(NOption_Index::variable, options);
		for (auto& var_str : vars) {
			
			bool resolved_ok = false;
			const auto delim_pos = var_str.find(L":=");
			if (delim_pos != std::wstring::npos) {
				var_str[delim_pos] = 0;

				TVariable var_to_set;
				var_to_set.name = var_str.c_str();;
				var_to_set.value = var_str.data() + delim_pos + 2;

				resolved_ok = !var_to_set.name.empty() && var_to_set.value.empty();
				if (resolved_ok) {
					result.variables.push_back(var_to_set);
				}
			}

			if (!resolved_ok) {
				std::wcerr << L"Malformed variable parameter: " << var_str << std::endl;
				result.action = NAction::failed_configuration;
				return result;
			}
		}

		//2.6 gather hints for the optimization
		result.hints_to_load = Gather_Values(NOption_Index::hint, options);

		//2.7 gather hints for the optimization from parameters file
		result.hinting_parameters_to_load = Gather_Values(NOption_Index::parameters_hint, options);
	}

	return result;
}

TAction Parse_Options(const int argc, const char** argv) {

	TAction result;
	result.action = NAction::failed_configuration;

	if (argc < 2) {
		Show_Help();
		return result;
	}

	result.config_path = Widen_Char(argv[1]);
	const auto effective_argc = argc - 2;
	const auto effective_argv = argv + 2;

	if (result.config_path.empty()) {
		std::wcerr << L"Configuration file path cannot be empty!\n";
		return result;
	}

	//declared the required structures for parsing the command line
	option::Stats  stats(true, option_syntax.data(), effective_argc, effective_argv);   //stats and parse take true to allow non-switch options
	std::vector<option::Option> options(stats.options_max), buffer(stats.buffer_max);
	option::Parser parse(true, option_syntax.data(), effective_argc, effective_argv, options.data(), buffer.data());

	//1. parse the command line
	if (parse.error()) {
		std::wcerr << L"Failed to parse the command line options!" << std::endl;
		Show_Help();
		return result;
	}

	//2. determine desired action
	auto& action_arg = options[static_cast<size_t>(NOption_Index::action)];

	if (action_arg) {
		//OK, the user set the action -> let's choose which one
		const auto action_type = action_arg.last()->type();

		switch (action_type) {
			case static_cast<TOption_Type>(NAction_Type::optimize_config):
				result.action = NAction::optimize;
				break;
			case static_cast<TOption_Type>(NAction_Type::execute_config):
				result.action = NAction::execute;
				break;
			default:
				result.action = NAction::failed_configuration;
				std::wcerr << L"Unknown action code: " << static_cast<size_t>(action_type) << std::endl;
				std::cout << actExecute.help << std::endl;
				std::cout << actOptimize.help << std::endl;
				break;
		}
	}
	else {
		std::wcout << L"Action not specified, will default to execution." << std::endl;
		result.action = NAction::execute;
	}

	if (result.action != NAction::failed_configuration) {
		result = Resolve_Parameters(result, options);
	}

	return result;
}
