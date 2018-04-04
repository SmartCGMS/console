
#include "../../common/desktop-console/filter_chain.h"
#include "../../common/desktop-console/filter_chain_holder.h"
#include "../../common/desktop-console/config.h"

#include "../../common/rtl/FilesystemLib.h"

#include <iostream>
#include <csignal>
#include <mutex>
#include <condition_variable>

// closing flag (to avoid spurious wakeup problems)
bool gClosing = false;
// mutex for closing condition variable
std::mutex gClose_Mtx;
// closing condition variable
std::condition_variable gClose_Cv;

void sighandler(int signo)
{
	if (signo == SIGINT)
	{
		std::unique_lock<std::mutex> lck(gClose_Mtx);

		// set closing flag, notify main thread
		gClosing = true;
		gClose_Cv.notify_all();
	}
}

int _cdecl main(int argc, char** argv)
{
	signal(SIGINT, sighandler);

	// create chain holder - it holds CFilter_Chain instance
	CFilter_Chain_Holder filterChainHolder;

	// load config and retrieve loaded filter chain
	Configuration.Resolve_And_Load_Config_File();
	Configuration.Load(filterChainHolder.Get_Filter_Chain());

	// attempt to initialize and start filters
	if (filterChainHolder.Init_And_Start_Filters() != S_OK)
		return 1;

	// wait for user to close app
	std::unique_lock<std::mutex> lck(gClose_Mtx);
	gClose_Cv.wait(lck, []() { return gClosing; });

	return 0;
}
