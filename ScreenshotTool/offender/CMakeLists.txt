cmake_minimum_required(VERSION 3.12)

enable_language(C ASM_MASM)

set (SOURCES
	offender.cpp
	codeCave.asm
	Packet.h
)

add_library (offender SHARED ${SOURCES})

set_target_properties(offender PROPERTIES CXX_STANDARD 20)