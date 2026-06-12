/*
  Copyright (c) 2004-2026 FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef CREATEDOCKERFIREBIRDDIALOG_H
#define CREATEDOCKERFIREBIRDDIALOG_H

#include <wx/wx.h>
#include <wx/filepicker.h>
#include "gui/BaseDialog.h"
#include "metadata/root.h"

class CreateDockerFirebirdDialog : public BaseDialog
{
private:
    RootPtr rootM;

    wxTextCtrl* textCtrlContainerNameM;
    wxTextCtrl* textCtrlHostPortM;
    wxTextCtrl* textCtrlPasswordM;
    wxChoice* choiceVersionM;
    wxDirPickerCtrl* dirPickerVolumeM;
    wxCheckBox* checkBoxAutoRegisterM;

    wxButton* buttonOkM;
    wxButton* buttonCancelM;

    void layoutControls();
    void updateButtons();
    void OnTextChange(wxCommandEvent& event);
    void OnOkButtonClick(wxCommandEvent& event);
protected:
    virtual const wxString getName() const;
public:
    CreateDockerFirebirdDialog(wxWindow* parent, RootPtr root);
    virtual ~CreateDockerFirebirdDialog();

    wxString getContainerName() const;
    wxString getHostPort() const;
    wxString getPassword() const;
    wxString getVersionTag() const;
    wxString getVolumeDir() const;
    bool getAutoRegister() const;

    bool runCreateContainer();
};

#endif // CREATEDOCKERFIREBIRDDIALOG_H
