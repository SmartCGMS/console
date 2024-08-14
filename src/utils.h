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

#include <scgms/rtl/scgmsLib.h>
#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/FilesystemLib.h>
#include <scgms/rtl/referencedImpl.h>
#include <scgms/rtl/SolverLib.h>
#include <scgms/rtl/UILib.h>
#include <scgms/utils/winapi_mapping.h>

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
	#include <scgms/rtl/qdb_connector.h>
	#include <QtCore/QCoreApplication>
#endif

std::tuple<HRESULT, scgms::SPersistent_Filter_Chain_Configuration> Load_Experimental_Setup(int argc, char** argv, const std::vector<TVariable> &variables);
bool Load_Hints(const std::vector<std::wstring>& hint_paths, const size_t parameters_file_type, const bool parameters_file, std::vector<std::vector<double>>& hints_container); //paths may include wildcard

std::tuple<HRESULT, size_t> Count_Parameters_Size(scgms::SPersistent_Filter_Chain_Configuration& configuration, const std::vector<TOptimize_Parameter>& parameters);
