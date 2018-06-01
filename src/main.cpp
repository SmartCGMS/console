
#include "../../common/desktop-console/filter_chain.h"
#include "../../common/desktop-console/filter_chain_manager.h"
#include "../../common/desktop-console/config.h"

#include "../../common/rtl/FilesystemLib.h"

#include <iostream>
#include <csignal>
#include <mutex>
#include <condition_variable>

#include <QtCore/QCoreApplication>

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
	QCoreApplication app(argc, argv);

	signal(SIGINT, sighandler);

	// create chain manager - it holds CFilter_Chain instance
	CFilter_Chain_Manager filterChainManager;

	// load config and retrieve loaded filter chain
	Configuration.Resolve_And_Load_Config_File(argc > 1 ? std::wstring{ argv[1], argv[1] + strlen(argv[1]) } : std::wstring{});	//config uses QApp to determine the file path (to be platorm indepenedent) and it has to be initialized first
																																//but it tries to load custom config as well


	Configuration.Load(filterChainManager.Get_Filter_Chain());

	// attempt to initialize and start filters
	if (filterChainManager.Init_And_Start_Filters() != S_OK)
		return 1;

	// wait for user to close app
	std::unique_lock<std::mutex> lck(gClose_Mtx);
	gClose_Cv.wait(lck, []() { return gClosing; });

	return 0;
}
