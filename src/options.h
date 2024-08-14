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


#pragma once

#include <scgms/rtl/FilesystemLib.h>
#include <scgms/iface/UIIface.h>

#include <vector>

enum class NAction : size_t {
	failed_configuration,
	execute,
	optimize
};

struct TOptimize_Parameter {
	size_t index = std::numeric_limits<size_t>::max();
	std::wstring name;
};

struct TVariable {
	std::wstring name, value;
};

struct TAction {
	// what to do
	NAction action = NAction::failed_configuration;

	std::wstring config_path;
	bool save_config = false;

	// default: Halton MetaDE
	GUID solver_id = { 0x1274b08, 0xf721, 0x42bc, { 0xa5, 0x62, 0x5, 0x56, 0x71, 0x4c, 0x56, 0x85 } };
	// ideally a number of CPU cores divisible by 4, 8 and 16 and 32
	size_t generation_count = 96;
	size_t population_size = 1000;

	std::vector<TOptimize_Parameter> parameters_to_optimize;
	std::vector<TVariable> variables;

	// filenames of hints to be loaded; may include wildcard
	std::vector<std::wstring> hints_to_load;
	// filenames of hinting parameters to be loaded; may include wildcard
	std::vector<std::wstring> hinting_parameters_to_load;
};

TAction Parse_Options(const int argc, const char** argv);
void Show_Help();
