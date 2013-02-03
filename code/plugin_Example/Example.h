/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2013 Charles Joly Beauparlant

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).
	see <http://www.gnu.org/licenses/>

    This is derived from RayPlatform-example.

*/

#ifndef _Example_h
#define _Example_h

#include <RayPlatform/core/ComputeCore.h>

__DeclarePlugin(Example);

__DeclareMasterModeAdapter(Example,RAY_MASTER_MODE_STEP_A);
__DeclareMasterModeAdapter(Example,RAY_MASTER_MODE_STEP_B);
__DeclareMasterModeAdapter(Example,RAY_MASTER_MODE_STEP_C);

__DeclareSlaveModeAdapter(Example,RAY_SLAVE_MODE_STEP_A);
__DeclareSlaveModeAdapter(Example,RAY_SLAVE_MODE_STEP_B);
__DeclareSlaveModeAdapter(Example,RAY_SLAVE_MODE_STEP_C);

__DeclareMessageTagAdapter(Example,RAY_MPI_TAG_STOP_AND_DIE);
__DeclareMessageTagAdapter(Example,RAY_MPI_TAG_TIME_BOMB);

/**
 * The plugin Example
 *
 * \author Charles Joly Beauparlant
 **/
class Example:  public CorePlugin{

	__AddAdapter(Example,RAY_MASTER_MODE_STEP_A);
	__AddAdapter(Example,RAY_MASTER_MODE_STEP_B);
	__AddAdapter(Example,RAY_MASTER_MODE_STEP_C);

	__AddAdapter(Example,RAY_SLAVE_MODE_STEP_A);
	__AddAdapter(Example,RAY_SLAVE_MODE_STEP_B);
	__AddAdapter(Example,RAY_SLAVE_MODE_STEP_C);

	__AddAdapter(Example,RAY_MPI_TAG_STOP_AND_DIE);
	__AddAdapter(Example,RAY_MPI_TAG_TIME_BOMB);

/**
 * A list of master modes
 */
	MasterMode RAY_MASTER_MODE_STEP_A;
	MasterMode RAY_MASTER_MODE_STEP_B;
	MasterMode RAY_MASTER_MODE_STEP_C;

/** slave modes **/
	SlaveMode RAY_SLAVE_MODE_STEP_A;
	SlaveMode RAY_SLAVE_MODE_STEP_B;
	SlaveMode RAY_SLAVE_MODE_STEP_C;
	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;


	MessageTag RAY_MPI_TAG_START_STEP_A;
	MessageTag RAY_MPI_TAG_START_STEP_B;
	MessageTag RAY_MPI_TAG_START_STEP_C;

	MessageTag RAY_MPI_TAG_TIME_BOMB;
	MessageTag RAY_MPI_TAG_STOP_AND_DIE;

	ComputeCore*m_core;

/** states for progression **/
	bool m_doneA;
	bool m_doneB;
	bool m_doneC;
	bool m_doneExample;

/** Example state **/
	bool m_example;
public:

	Example();

/** callbacks for master modes **/
	void call_RAY_MASTER_MODE_STEP_A();
	void call_RAY_MASTER_MODE_STEP_B();
	void call_RAY_MASTER_MODE_STEP_C();

/** callbacks for slave modes **/
	void call_RAY_SLAVE_MODE_STEP_A();
	void call_RAY_SLAVE_MODE_STEP_B();
	void call_RAY_SLAVE_MODE_STEP_C();

	void call_RAY_MPI_TAG_TIME_BOMB(Message*message);
	void call_RAY_MPI_TAG_STOP_AND_DIE(Message*message);

/** the following two methods are required by the interface CorePlugin **/

/** register the plugin with the core **/
	void registerPlugin(ComputeCore*core);

/** resolve symbols not owned by the current plugin **/
	void resolveSymbols(ComputeCore*core);
};

#endif
