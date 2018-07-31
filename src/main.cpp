
#include "../../common/desktop-console/filter_chain.h"
#include "../../common/desktop-console/filter_chain_manager.h"
#include "../../common/desktop-console/config.h"

#include "../../common/rtl/FilesystemLib.h"

#include <iostream>
#include <csignal>
#include <mutex>
#include <condition_variable>

std::unique_ptr<CFilter_Chain_Manager> gFilter_Chain_Manager;

void sighandler(int signo)
{
	// SIGINT should terminate filters; this will eventually terminate whole app
	if (signo == SIGINT) {
		glucose::UDevice_Event shut_down_event{ glucose::NDevice_Event_Code::Shut_Down };
		gFilter_Chain_Manager->Send(shut_down_event);
	}		
}

int main(int argc, char** argv) {
	signal(SIGINT, sighandler);

	// create chain holder - it holds CFilter_Chain instance
	gFilter_Chain_Manager = std::make_unique<CFilter_Chain_Manager>();

	// load config and retrieve loaded filter chain
	Configuration.Resolve_And_Load_Config_File(argc > 1 ? std::wstring{ argv[1], argv[1] + strlen(argv[1]) } : std::wstring{});
	Configuration.Load(gFilter_Chain_Manager->Get_Filter_Chain());

	// attempt to initialize and start filters
	HRESULT rc = gFilter_Chain_Manager->Init_And_Start_Filters();
	if (rc != S_OK)
	{
		std::cerr << "Could not initialize filter chain, return code: " << rc << std::endl;
		return 1;
	}

	// wait for filters to finish, or user to close app
	gFilter_Chain_Manager->Join_Filters();

	return 0;
}
