#include "projectHandler.hpp"

ProjectHandler::ProjectHandler(DataProcessing* dataProcessingInstance, rapidjson::Document* settings)
	: wxDialog(NULL, wxID_ANY, "Select Project", wxDefaultPosition, wxDefaultSize) {
	// Get global settings file
	dataProcessing = dataProcessingInstance;
	mainSettings   = settings;

	mainSizer = new wxBoxSizer(wxVERTICAL);

	projectList = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE);

	rapidjson::GenericArray<false, rapidjson::Value> recentProjectsArray = (*mainSettings)["recentProjects"].GetArray();

	std::size_t listboxSize = recentProjectsArray.Size() + 1;
	wxString listboxItems[listboxSize];
	std::size_t listboxIndex = 0;

	for(auto const& recentProject : recentProjectsArray) {
		wxString recentProjectItem = wxString::Format("%s %s", wxString::FromUTF8(recentProject["projectName"].GetString()), wxString::FromUTF8(recentProject["projectDirectory"].GetString()));
		listboxItems[listboxIndex] = recentProjectItem;
		listboxIndex++;
	}

	listboxItems[listboxIndex] = createNewProjectText;

	projectList->InsertItems(listboxSize, listboxItems, 0);

	projectList->Bind(wxEVT_LISTBOX, &ProjectHandler::onClickProject, this);

	mainSizer->Add(projectList, 0, wxEXPAND | wxALL, 5);

	// Print create project input

	SetSizer(mainSizer);
	mainSizer->SetSizeHints(this);
	Layout();
	Fit();
	Center(wxBOTH);

	Layout();
}

void ProjectHandler::onClickProject(wxCommandEvent& event) {
	wxString selectedProject = event.GetString();
	if(selectedProject == createNewProjectText) {
		// Open up a new project dialog
		wxDirDialog dlg(NULL, "Choose Project Directory", "", wxDD_DEFAULT_STYLE);
		if(dlg.ShowModal() == wxID_OK) {
			// Directory chosen
			projectDir.Open(dlg.GetPath());
			projectChosen = true;
			Close(true);
		}
	} else {
		// It's a project folder
		projectDir.Open(selectedProject);
		// Fill up the data found here
		loadProject();
		projectChosen = true;
		Close(true);
	}
}

void ProjectHandler::loadProject() {
	wxFileName inputsFileName;
	inputsFileName.AppendDir(projectDir.GetNameWithSep());
	inputsFileName.SetName("inputs");
	inputsFileName.SetExt("bin");

	if(inputsFileName.FileExists()) {
		// Load up the inputs
		wxFFileStream inputsFileStream(inputsFileName.GetFullPath(), "rb");
		wxZlibInputStream inputsDecompressStream(inputsFileStream, wxZLIB_ZLIB | wxZLIB_NO_HEADER);

		wxMemoryOutputStream dataStream;
		dataStream.Write(inputsDecompressStream);
		inputsFileStream.Close();
		wxStreamBuffer* streamBuffer = dataStream.GetOutputStreamBuffer();

		std::vector<std::shared_ptr<ControllerData>>* inputsList = dataProcessing->getInputsList();

		// Set DataProcessing to the data found here
		serializeProtocol.binaryToData<std::vector<std::shared_ptr<ControllerData>>>(*inputsList, (uint8_t*)streamBuffer->GetBufferStart(), streamBuffer->GetBufferSize());
	}
}

void ProjectHandler::saveProject() {
	// Save each set of data one by one
	// Serialize the entire vector, first of all, with ZPP
	std::vector<std::shared_ptr<ControllerData>>* inputsList = dataProcessing->getInputsList();

	uint8_t* data;
	std::size_t dataSize;
	serializeProtocol.dataToBinary<std::vector<std::shared_ptr<ControllerData>>>(*inputsList, &data, &dataSize);

	wxFileName inputsFileName;
	inputsFileName.AppendDir(projectDir.GetNameWithSep());
	inputsFileName.SetName("inputs");
	inputsFileName.SetExt("bin");

	// Delete file if already present and use binary mode
	wxFFileStream inputsFileStream(inputsFileName.GetFullPath(), "wb");
	wxZlibOutputStream inputsCompressStream(inputsFileStream, compressionLevel, wxZLIB_ZLIB | wxZLIB_NO_HEADER);
	inputsCompressStream.WriteAll(data, dataSize);
	inputsCompressStream.Sync();
	inputsFileStream.Close();

	// For now, that's it
}

void ProjectHandler::createTempProjectDir() {
	// Open standard documents folder
	wxString documentsFolder = wxStandardPaths::Get().GetDocumentsDir();
	wxFileName dir(documentsFolder);
	dir.AppendDir(wxString::Format("TAS_PROJECT_%lld", wxGetLocalTimeMillis().GetValue()));
	projectDir.Open(dir.GetFullPath());
	dir.Mkdir();
}

ProjectHandler::~ProjectHandler() {
	// Write JSON settings
	wxFFileStream inputsFileStream("../mainSettings.json", "w");

	rapidjson::StringBuffer sb;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
	writer.SetIndent('\t', 1);
	mainSettings->Accept(writer);

	inputsFileStream.WriteAll(sb.GetString(), sb.GetLength());
	inputsFileStream.Close();
}