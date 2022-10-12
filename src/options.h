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


#pragma once

#include "../../common/rtl/FilesystemLib.h"
#include "../../common/iface/UIIface.h"


#include <vector>

enum class NAction : size_t {
	failed_configuration,
	execute,
	optimize
};


struct TOptimize_Parameters {	
	size_t index = std::numeric_limits<size_t>::max();
	std::wstring name;
};


struct TVariable {
	std::wstring name, value;
};

struct TAction {
	NAction action;								//what to do

	std::wstring config_path;
	bool save_config = false;
	GUID solver_id = { 0x1274b08, 0xf721, 0x42bc, { 0xa5, 0x62, 0x5, 0x56, 0x71, 0x4c, 0x56, 0x85 } };	//Halton MetaDE
	size_t generation_count = 100;
	size_t population_size = 1000;

	std::vector<TOptimize_Parameters> parameters_to_optimize;
	std::vector<TVariable> variables;
};

TAction Parse_Options(const int argc, const char** argv);
void Show_Help();