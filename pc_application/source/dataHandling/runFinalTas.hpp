#pragma once

#include <wx/spinctrl.h>
#include <wx/wfstream.h>
#include <wx/wx.h>

#include "../helpers.hpp"
#include "../sharedNetworkCode/networkInterface.hpp"
#include "../sharedNetworkCode/serializeUnserializeData.hpp"
#include "buttonConstants.hpp"
#include "buttonData.hpp"
#include "dataProcessing.hpp"

class TasRunner : public wxDialog {
private:
	std::shared_ptr<CommunicateWithNetwork> networkInstance;
	rapidjson::Document* mainSettings;
	DataProcessing* dataProcessing;

	SerializeProtocol serializeProtocol;

	wxBoxSizer* mainSizer;
	wxBoxSizer* hookSelectionSizer;

	wxSpinCtrl* firstSavestateHook;
	wxSpinCtrl* lastSavestateHook;

	wxBitmapButton* startTasHomebrew;
	wxBitmapButton* startTasArduino;
	// More will be added as needed

	// wxBitmapButton* pauseTas;
	// Stopping will also close the dialog
	wxBitmapButton* stopTas;

	void onStartTasHomebrewPressed(wxCommandEvent& event);
	void onStartTasArduinoPressed(wxCommandEvent& event);

	void onStopTasPressed(wxCommandEvent& event);

public:
	TasRunner(wxFrame* parent, std::shared_ptr<CommunicateWithNetwork> networkImp, rapidjson::Document* settings, DataProcessing* inputData);
};