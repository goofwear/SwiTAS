#pragma once

//#include "../thirdParty/win32darkmode/DarkMode.h"
#include <ffms.h>
#include <fstream>
#include <memory>
#include <rapidjson/document.h>
#include <string>
#include <vector>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/wx.h>

#include "../sharedNetworkCode/networkInterface.hpp"

#include "../dataHandling/buttonData.hpp"
#include "../dataHandling/dataProcessing.hpp"
#include "../dataHandling/gameCorruptor.hpp"
#include "../dataHandling/projectHandler.hpp"
#include "../dataHandling/runFinalTas.hpp"
#include "../helpers.hpp"
#include "bottomUI.hpp"
#include "debugWindow.hpp"
#include "scriptExporter.hpp"
#include "sideUI.hpp"

class MainWindow : public wxFrame {
private:
	const uint8_t NETWORK_CALLBACK_ID = 0;

	// Simplify things immesnly
	wxBoxSizer* mainSizer;

	// Main settings variable
	rapidjson::Document mainSettings;

	// Button data variable
	std::shared_ptr<ButtonData> buttonData;

	// Project handler, creates projects and saves them
	std::shared_ptr<ProjectHandler> projectHandler;

	// The pointers to the classes containing the uis
	std::shared_ptr<SideUI> sideUI;
	std::shared_ptr<BottomUI> bottomUI;
	// Pointer to the class containing important input stuff
	DataProcessing* dataProcessingInstance;
	// Networking stuff
	std::shared_ptr<CommunicateWithNetwork> networkInstance;

	// For sideUI
	wxTimer* autoFrameAdvanceTimer;
	wxTimer* wakeUpIdleTimer;

	int autoFrameTimerID;
	int wakeupIdleTimerID;

	// Main logging window
	wxLogWindow* logWindow;
	// Main debug command window
	DebugWindow* debugWindow;

	// Menubar
	wxMenuBar* menuBar;
	// Menubar menus
	wxWindowID selectIPID;
	wxWindowID exportAsText;
	wxWindowID importAsText;
	wxWindowID saveProject;
	wxWindowID setNameID;
	wxWindowID toggleLoggingID;
	wxWindowID toggleDebugMenuID;
	wxWindowID openGameCorruptorID;
	wxWindowID runFinalTasID;

	void handlePreviousWindowTransform();

	void handleMenuBar(wxCommandEvent& commandEvent);

	void onRecentVideosMenuOpen(wxMenuEvent& event) {
		// Pass on to project handler
		projectHandler->onRecentVideosMenuOpen(event);
	}

	void addMenuBar();
	void addStatusBar();

	// Override default signal handler:
	void keyDownHandler(wxKeyEvent& event);
	void OnSize(wxSizeEvent& event);
	void onClose(wxCloseEvent& event);
	void onIdle(wxIdleEvent& event);

	void onTimer(wxTimerEvent& event);

	bool askForIP();
	void handleNetworkQueues();

	// Used to freeze the current frame view to make it look good
	void startedIncrementFrame();

public:
	MainWindow();

	// Called by wxApp
	void onStart();

	DECLARE_EVENT_TABLE();
};