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

#include "options.h"

#include "../../common/rtl/scgmsLib.h"
#include "../../common/rtl/FilterLib.h"
#include "../../common/rtl/FilesystemLib.h"
#include "../../common/rtl/referencedImpl.h"
#include "../../common/rtl/SolverLib.h"
#include "../../common/rtl/UILib.h"
#include "../../common/utils/winapi_mapping.h"

#include <iostream>
#include <csignal>
#include <thread>
#include <climits>
#include <chrono>
#include <cmath>

 /*
  *	If you do not need database access, or do not want to use Qt, then
  *  #define DDO_NOT_USE_QT
  */
#ifndef DDO_NOT_USE_QT
	#include "../../common/rtl/qdb_connector.h"
	#include <QtCore/QCoreApplication>
#endif


std::tuple<HRESULT, scgms::SPersistent_Filter_Chain_Configuration> Load_Experimental_Setup(int argc, char** argv, const std::vector<TVariable> &variables);
bool Load_Hints(const std::vector<std::wstring>& hint_paths, const size_t parameters_file_type, const bool parameters_file, std::vector<std::vector<double>>& hints_container); //paths may include wildcard

std::tuple<HRESULT, size_t> Count_Parameters_Size(scgms::SPersistent_Filter_Chain_Configuration& configuration, const std::vector<TOptimize_Parameter>& parameters);
