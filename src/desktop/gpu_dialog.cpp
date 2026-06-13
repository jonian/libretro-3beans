/*
    Copyright 2023-2026 Hydr8gon

    This file is part of 3Beans.

    3Beans is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    3Beans is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with 3Beans. If not, see <https://www.gnu.org/licenses/>.
*/

#include "gpu_dialog.h"
#include "../core/settings.h"

enum GpuEvent {
    GPU_RENDER_SOFT = 1,
    GPU_RENDER_OGL,
    VTX_SHADER_INTERP,
    VTX_SHADER_GLSL,
    FRAG_SHADER_UBER,
    FRAG_SHADER_JIT,
    THREADED_GPU
};

wxBEGIN_EVENT_TABLE(GpuDialog, wxDialog)
EVT_RADIOBUTTON(GPU_RENDER_SOFT, GpuDialog::gpuRenderer<0>)
EVT_RADIOBUTTON(GPU_RENDER_OGL, GpuDialog::gpuRenderer<1>)
EVT_RADIOBUTTON(VTX_SHADER_INTERP, GpuDialog::gpuVtxShader<0>)
EVT_RADIOBUTTON(VTX_SHADER_GLSL, GpuDialog::gpuVtxShader<1>)
EVT_RADIOBUTTON(FRAG_SHADER_UBER, GpuDialog::gpuFragShader<0>)
EVT_RADIOBUTTON(FRAG_SHADER_JIT, GpuDialog::gpuFragShader<1>)
EVT_CHECKBOX(THREADED_GPU, GpuDialog::threadedGpu)
EVT_BUTTON(wxID_CANCEL, GpuDialog::cancel)
EVT_BUTTON(wxID_OK, GpuDialog::confirm)
wxEND_EVENT_TABLE()

GpuDialog::GpuDialog(bool glSupport): wxDialog(nullptr, wxID_ANY, "GPU Settings") {
    // Remember previous settings in case changes are discarded
    prevSettings[0] = Settings::threadedGpu;
    prevSettings[1] = Settings::gpuRenderer;
    prevSettings[2] = Settings::gpuVtxShader;
    prevSettings[3] = Settings::gpuFragShader;

    // Use the height of a button as a unit to scale pixel values based on DPI/font
    wxButton *dummy = new wxButton(this, wxID_ANY, "");
    int size = dummy->GetSize().y;
    delete dummy;

    // Set up the render backend selections
    wxBoxSizer *renderSizer = new wxBoxSizer(wxHORIZONTAL);
    renderSizer->Add(new wxStaticText(this, wxID_ANY, "Render Backend:",
        wxDefaultPosition, wxSize(wxDefaultSize.GetWidth(), size)));
    renderSizer->Add(renderBtns[0] = new wxRadioButton(this, GPU_RENDER_SOFT, "Software",
        wxDefaultPosition, wxDefaultSize, wxRB_GROUP), 0, wxLEFT, size / 4);
    renderSizer->Add(renderBtns[1] = new wxRadioButton(this, GPU_RENDER_OGL, "OpenGL"), 0, wxLEFT, size / 4);

    // Set up the vertex shader selections
    wxBoxSizer *vtxSizer = new wxBoxSizer(wxHORIZONTAL);
    vtxSizer->Add(new wxStaticText(this, wxID_ANY, "Vertex Shader:",
        wxDefaultPosition, wxSize(wxDefaultSize.GetWidth(), size)));
    vtxSizer->Add(vtxBtns[0] = new wxRadioButton(this, VTX_SHADER_INTERP, "Interpreter",
        wxDefaultPosition, wxDefaultSize, wxRB_GROUP), 0, wxLEFT, size / 4);
    vtxSizer->Add(vtxBtns[1] = new wxRadioButton(this, VTX_SHADER_GLSL, "GLSL JIT"), 0, wxLEFT, size / 4);

    // Set up the fragment shader selections
    wxBoxSizer *fragSizer = new wxBoxSizer(wxHORIZONTAL);
    fragSizer->Add(new wxStaticText(this, wxID_ANY, "Fragment Shader:",
        wxDefaultPosition, wxSize(wxDefaultSize.GetWidth(), size)));
    fragSizer->Add(fragBtns[0] = new wxRadioButton(this, FRAG_SHADER_UBER, "Ubershader",
        wxDefaultPosition, wxDefaultSize, wxRB_GROUP), 0, wxLEFT, size / 4);
    fragSizer->Add(fragBtns[1] = new wxRadioButton(this, FRAG_SHADER_JIT, "GLSL JIT"), 0, wxLEFT, size / 4);

    // Set up the threaded GPU checkbox
    wxBoxSizer *checkSizer = new wxBoxSizer(wxHORIZONTAL);
    wxCheckBox *threadBox = new wxCheckBox(this, THREADED_GPU, "Run on Separate Thread");
    checkSizer->Add(threadBox, 0, wxLEFT, size / 8);

    // Set the initial setting states
    threadBox->SetValue(Settings::threadedGpu);
    renderBtns[std::min(Settings::gpuRenderer, 1)]->SetValue(true);
    vtxBtns[std::min(Settings::gpuVtxShader, 1)]->SetValue(true);
    fragBtns[std::min(Settings::gpuFragShader, 1)]->SetValue(true);

    // Disable settings based on GL availability and current renderer
    renderBtns[1]->Enable(glSupport);
    bool ogl = (Settings::gpuRenderer == 1);
    for (int i = 0; i < 2; i++) {
        vtxBtns[i]->Enable(ogl);
        fragBtns[i]->Enable(ogl);
    }

    // Set up the cancel and confirm buttons
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxStaticText(this, wxID_ANY, ""), 1);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, size / 16);
    buttonSizer->Add(new wxButton(this, wxID_OK, "Confirm"), 0, wxLEFT, size / 16);

    // Combine all the contents
    wxBoxSizer *contents = new wxBoxSizer(wxVERTICAL);
    contents->Add(renderSizer, 1, wxEXPAND);
    contents->Add(vtxSizer, 1, wxEXPAND);
    contents->Add(fragSizer, 1, wxEXPAND);
    contents->Add(checkSizer, 1, wxEXPAND);
    contents->Add(buttonSizer, 1, wxEXPAND);

    // Add a final border around everything
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(contents, 1, wxEXPAND | wxALL, size / 4);
    SetSizerAndFit(sizer);
}

void GpuDialog::threadedGpu(wxCommandEvent &event) {
    // Toggle the threaded GPU setting
    Settings::threadedGpu = !Settings::threadedGpu;
}

template <int i> void GpuDialog::gpuRenderer(wxCommandEvent &event) {
    // Set the GPU renderer to a specific value
    Settings::gpuRenderer = i;

    // Enable shader settings only when OpenGL is selected
    bool ogl = (Settings::gpuRenderer == 1);
    for (int j = 0; j < 2; j++) {
        vtxBtns[j]->Enable(ogl);
        fragBtns[j]->Enable(ogl);
    }
}

template <int i> void GpuDialog::gpuVtxShader(wxCommandEvent &event) {
    // Set the GPU vertex shader to a specific value
    Settings::gpuVtxShader = i;
}

template <int i> void GpuDialog::gpuFragShader(wxCommandEvent &event) {
    // Set the GPU fragment shader to a specific value
    Settings::gpuFragShader = i;
}

void GpuDialog::cancel(wxCommandEvent &event) {
    // Reset settings to their previous values
    Settings::threadedGpu = prevSettings[0];
    Settings::gpuRenderer = prevSettings[1];
    Settings::gpuVtxShader = prevSettings[2];
    Settings::gpuFragShader = prevSettings[3];
    event.Skip(true);
}

void GpuDialog::confirm(wxCommandEvent &event) {
    // Save the modified settings
    Settings::save();
    event.Skip(true);
}
