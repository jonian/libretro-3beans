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

#pragma once

#include <wx/wx.h>

class GpuDialog: public wxDialog {
public:
    GpuDialog(bool glSupport);

private:
    wxRadioButton *renderBtns[2];
    wxRadioButton *vtxBtns[2];
    wxRadioButton *fragBtns[2];
    int prevSettings[4];

    template <int i> void gpuRenderer(wxCommandEvent &event);
    template <int i> void gpuVtxShader(wxCommandEvent &event);
    template <int i> void gpuFragShader(wxCommandEvent &event);
    void threadedGpu(wxCommandEvent &event);
    void cancel(wxCommandEvent &event);
    void confirm(wxCommandEvent &event);
    wxDECLARE_EVENT_TABLE();
};
