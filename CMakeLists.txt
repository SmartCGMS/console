# SmartCGMS - continuous glucose monitoring and controlling framework
# https://diabetes.zcu.cz/
#
# Copyright (c) since 2018 University of West Bohemia.
#
# Contact:
# diabetes@mail.kiv.zcu.cz
# Medical Informatics, Department of Computer Science and Engineering
# Faculty of Applied Sciences, University of West Bohemia
# Univerzitni 8, 301 00 Pilsen
# Czech Republic
# 
# 
# Purpose of this software:
# This software is intended to demonstrate work of the diabetes.zcu.cz research
# group to other scientists, to complement our published papers. It is strictly
# prohibited to use this software for diagnosis or treatment of any medical condition,
# without obtaining all required approvals from respective regulatory bodies.
#
# Especially, a diabetic patient is warned that unauthorized use of this software
# may result into severe injure, including death.
#
#
# Licensing terms:
# Unless required by applicable law or agreed to in writing, software
# distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# a) This file is available under the Apache License, Version 2.0.
# b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
#    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
#    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
#    Volume 177, pp. 354-362, 2020

CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

SET(PROJ "scgms-console")

PROJECT(${PROJ})

MESSAGE(STATUS "Configuring console project")

IF (NOT DEFINED SMARTCGMS_COMMON_DIR)
	SET(SMARTCGMS_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common/" CACHE PATH "SmartCGMS 'common' directory location")
ENDIF()

INCLUDE("${SMARTCGMS_COMMON_DIR}/cmake/Functions.cmake")

DISCOVER_DEPENDENCIES()

INCLUDE_DIRECTORIES("${SMARTCGMS_COMMON_DIR}/")

FILE(GLOB SRC_BASE src/*.cpp src/*.h)

IF (NOT Qt_DISABLE)
	DISCOVER_QT_LIBRARY(Core Sql Widgets)
ENDIF()

SCGMS_ADD_EXECUTABLE(${PROJ} ${SRC_BASE})
CONFIGURE_TARGET_OUTPUT(${PROJ} "")
TARGET_LINK_LIBRARIES(${PROJ} scgms-common)

IF (NOT Qt_DISABLE)
	TARGET_LINK_LIBRARIES(${PROJ} Qt::Core Qt::Sql Qt::Widgets)
ENDIF()

TARGET_COMPILE_DEFINITIONS(${PROJ} PUBLIC "-DNOGUI")

APPLY_SCGMS_LIBRARY_BUILD_SETTINGS(${PROJ})
CONFIGURE_ROOT_TARGET_OUTPUT(${PROJ})
