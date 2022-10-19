/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/stream.h>
#include <wx/wfstream.h>

#include "AdvancedMessageDialog.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/CommandIds.h"
#include "gui/CommandManager.h"
#include "gui/controls/ControlUtils.h"
#include "gui/controls/DataGridTable.h"
#include "gui/EditBlobDialog.h"
#include "gui/FRLayoutConfig.h"
#include "gui/StyleGuide.h"

// Static members
/* maybe later needed if plugin will be implemented
int EditBlobDialog::m_libEditBlobUseCount = 0;
wxDynamicLibrary* EditBlobDialog::m_libEditBlob = NULL;
*/


// Helper Class of wxStyledTextCtrl to handle NULL values
class EditBlobDialogSTC : public wxStyledTextCtrl 
{
public:
    EditBlobDialogSTC(wxWindow *parent, wxWindowID id=wxID_ANY);
    
    void setIsNull(bool isNull);
    bool getIsNull() { return isNullM; }

    void ClearAll();
    void SetText(const wxString& text);
private:
    bool isNullM;

    void OnChar(wxKeyEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    DECLARE_EVENT_TABLE()
};

EditBlobDialogSTC::EditBlobDialogSTC(wxWindow *parent, wxWindowID id)
    : wxStyledTextCtrl(parent, id, wxDefaultPosition, wxDefaultSize,
        wxBORDER_THEME), isNullM(false)
{
    wxFont fontNull(frlayoutconfig().getEditorFontSize(),
        wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
        false);
    StyleSetFont(10, fontNull);
    StyleSetForeground(10, "RED");
}

void EditBlobDialogSTC::OnKeyDown(wxKeyEvent& event)
{
    if (GetReadOnly())
    {
        event.Skip();
        return;
    }

    int kc = event.GetKeyCode();
    if ((kc == WXK_DELETE) &&
        (GetText() == ""))
    {
        setIsNull(true);
        return;
    }
    if ((kc == WXK_DELETE) ||
        (kc == WXK_RETURN) ||
        (kc == WXK_BACK) ||
        (kc == WXK_TAB))
        setIsNull(false);

    event.Skip();
}

void EditBlobDialogSTC::OnChar(wxKeyEvent& event)
{
    if (GetReadOnly())
    {
        event.Skip();
        return;
    }

    setIsNull(false);
    event.Skip();
}

void EditBlobDialogSTC::setIsNull(bool isNull)
{
    if (isNull == isNullM)
        return;

    if (isNull)
    {
        wxStyledTextCtrl::SetText("[null]");
        SelectAll();
        StartStyling(0);
        SetStyling(GetTextLength(), 0x0A);
    }
    else
    {
        StartStyling(0);
        SetStyling(GetTextLength(), 0);
    }
    isNullM = isNull;
}

void EditBlobDialogSTC::ClearAll()
{
    setIsNull(false);
    wxStyledTextCtrl::ClearAll();
}

void EditBlobDialogSTC::SetText(const wxString& text)
{
    setIsNull(false);
    wxStyledTextCtrl::SetText(text);
}

//! event handling
BEGIN_EVENT_TABLE(EditBlobDialogSTC, wxStyledTextCtrl)
    EVT_KEY_DOWN(EditBlobDialogSTC::OnKeyDown)
    EVT_CHAR(EditBlobDialogSTC::OnChar)
END_EVENT_TABLE()


// Helper Class of wxStyledTextCtrl
// -> adds a context to toggle "Linkebreak" on / off
class EditBlobDialogSTCText : public EditBlobDialogSTC 
{
public:
    EditBlobDialogSTCText(wxWindow *parent, wxWindowID id=wxID_ANY);
    
    bool hasSelection();
private:
    void OnContextMenuCmd(wxCommandEvent& event);
    void OnContextMenu(wxContextMenuEvent& WXUNUSED(event));

    DECLARE_EVENT_TABLE()
};

EditBlobDialogSTCText::EditBlobDialogSTCText(wxWindow *parent, wxWindowID id)
    : EditBlobDialogSTC(parent,id)
{
}

bool EditBlobDialogSTCText::hasSelection()
{
    return GetSelectionStart() != GetSelectionEnd();
}

void EditBlobDialogSTCText::OnContextMenu(wxContextMenuEvent& event)
{
    if (AutoCompActive() || CallTipActive())
        return;
    SetFocus();

    bool isWrapModeWord = (GetWrapMode() == wxSTC_WRAP_WORD);
    wxMenu m;
    m.Append(wxID_UNDO, _("&Undo"))->Enable(CanUndo());
    m.Append(wxID_REDO, _("&Redo"))->Enable(CanRedo());
    m.AppendSeparator();
    m.Append(wxID_CUT,    _("Cu&t"))->Enable(hasSelection());
    m.Append(wxID_COPY,   _("&Copy"))->Enable(hasSelection());
    m.Append(wxID_PASTE,  _("&Paste"))->Enable(CanPaste());
    m.Append(wxID_DELETE, _("&Delete"))->Enable(hasSelection());
    m.AppendSeparator();
    m.Append(wxID_SELECTALL, _("Select &All"));
    m.AppendCheckItem(Cmds::BlobEditor_ChangeLineBreak, _("Line break"))->Check(isWrapModeWord);

    PopupMenu(&m, calcContextMenuPosition(event.GetPosition(), this));
}

void EditBlobDialogSTCText::OnContextMenuCmd(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case wxID_UNDO : Undo(); break;
        case wxID_REDO : Redo(); break;
        case wxID_CUT : Cut(); break;
        case wxID_COPY : Copy(); break;
        case wxID_PASTE : Paste(); break;
        case wxID_DELETE : Clear(); break;
        case wxID_SELECTALL : SelectAll(); break;
        case Cmds::BlobEditor_ChangeLineBreak : 
            int newWrapMode = (GetWrapMode() == wxSTC_WRAP_NONE) 
                ? wxSTC_WRAP_WORD : wxSTC_WRAP_NONE;
            SetWrapMode(newWrapMode);
            break;
    }
}

BEGIN_EVENT_TABLE(EditBlobDialogSTCText, EditBlobDialogSTC)
    EVT_CONTEXT_MENU(EditBlobDialogSTCText::OnContextMenu)

    EVT_MENU(wxID_UNDO, EditBlobDialogSTCText::OnContextMenuCmd)
    EVT_MENU(wxID_REDO, EditBlobDialogSTCText::OnContextMenuCmd)
    EVT_MENU(wxID_CUT, EditBlobDialogSTCText::OnContextMenuCmd)
    EVT_MENU(wxID_COPY, EditBlobDialogSTCText::OnContextMenuCmd)
    EVT_MENU(wxID_PASTE, EditBlobDialogSTCText::OnContextMenuCmd)
    EVT_MENU(wxID_DELETE, EditBlobDialogSTCText::OnContextMenuCmd)
    EVT_MENU(wxID_SELECTALL, EditBlobDialogSTCText::OnContextMenuCmd)
    EVT_MENU(Cmds::BlobEditor_ChangeLineBreak, EditBlobDialogSTCText::OnContextMenuCmd)
END_EVENT_TABLE()


// Helper-Class for streaming into blob / buffer
class FRInputBlobStream : public wxInputStream
{
    public:
        FRInputBlobStream(IBPP::Blob blob);
        virtual ~FRInputBlobStream();
        virtual size_t GetSize() const;
    protected:
        virtual size_t OnSysRead(void *buffer, size_t size);          
    private:
        IBPP::Blob blobM;
        int sizeM;
};

class FROutputBlobStream : public wxOutputStream
{
    public:
        FROutputBlobStream(IBPP::Blob blob);
        virtual ~FROutputBlobStream();
        
        virtual bool Close();
    protected:
        virtual size_t OnSysWrite(const void *buffer, size_t bufsize);
    private:
        IBPP::Blob blobM;
};


// Helper Class - ProgressPanel - Progress-info with Progressbar

class EditBlobDialogProgressSizer : public wxBoxSizer
{
public:
    EditBlobDialogProgressSizer(wxWindow* parent);

    void Hide();
   
    bool canCancel();
    void cancel();
    void initProgress(const wxString& progressTitle, int range, bool canCancel);
    bool isActive();
    bool isCanceled();
    void stepProgress(int stepAmount);
private:
    bool activeM;
    bool canceledM;
    bool canCancelM;
    int posM;
    int rangeM;

    wxButton* buttonCancelM;
    wxWindow* parentM;
    wxGauge* progressGaugeM;
    wxStaticText* progressTextM;
};

EditBlobDialogProgressSizer::EditBlobDialogProgressSizer(wxWindow* parent)
    : wxBoxSizer(wxHORIZONTAL), parentM(parent)
{
    activeM = false;
    rangeM = 0;
    posM = 0;
    canCancelM = false;

    progressTextM = new wxStaticText(parent, wxID_ANY, "");
    buttonCancelM = new wxButton(parent, Cmds::BlobEditor_ProgressCancel, _("&Cancel"));

    int gaugeHeight = wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y);
    progressGaugeM = new wxGauge(parent, wxID_ANY, 0, wxDefaultPosition,
        wxSize(10, gaugeHeight), wxGA_HORIZONTAL | wxGA_SMOOTH);

    Add(progressTextM, 0, wxALIGN_CENTER_VERTICAL);
    AddSpacer(styleguide().getFrameMargin(wxLEFT));
    Add(progressGaugeM, 1, wxALIGN_CENTER_VERTICAL);
    AddSpacer(styleguide().getFrameMargin(wxRIGHT));
    Add(buttonCancelM, 0, wxALIGN_CENTER_VERTICAL);

    //Layout();
    Hide();
};

bool EditBlobDialogProgressSizer::canCancel()
{
    return canCancelM;
}

void EditBlobDialogProgressSizer::cancel()
{
    if (canCancelM)
        canceledM = true;
}

void EditBlobDialogProgressSizer::Hide()
{ 
    progressTextM->Hide();
    buttonCancelM->Hide();
    progressGaugeM->Hide();
    activeM = false;
}

void EditBlobDialogProgressSizer::initProgress(const wxString& progressTitle,
    int range, bool canCancel)
{
    posM = 0;
    rangeM = range;
    canceledM = false;
    canCancelM = canCancel;
    activeM = true;

    progressTextM->SetLabel(progressTitle);
    progressGaugeM->SetRange(rangeM);
    progressGaugeM->SetValue(posM);

    progressTextM->Show();
    buttonCancelM->Show(canCancelM);
    progressGaugeM->Show();

    //parentM->Layout();
    Layout();
}

bool EditBlobDialogProgressSizer::isActive()
{
    return activeM;
}

bool EditBlobDialogProgressSizer::isCanceled()
{
    return canceledM;
}

void EditBlobDialogProgressSizer::stepProgress(int stepAmount)
{ 
    posM += stepAmount;
    progressGaugeM->SetValue(posM);
    wxYieldIfNeeded();
}


// Main (dialog) class for blob editor
EditBlobDialog::EditBlobDialog(wxWindow* parent, wxMBConv* converterM)
    :BaseDialog(parent, -1, wxEmptyString)    
{
    runningM = false; // disable wxNotebookPageChanged-Events
    dataModifiedM = false;
    editorModeM = noData;
    cacheM = 0;
    cacheIsNullM = false;
    dialogCaptionM = "";
    dataGridTableM = 0;
    dataGridM = 0;
    fieldNameM = "";
    rowM = 0;
    colM = 0;
    blobM = 0;
    loadingM = false;
    statementM = 0;
    readonlyM = false;

    this->converterM = converterM;

    notebook = new wxNotebook(getControlsPanel(), wxID_ANY);
    blob_noData = new wxPanel(notebook, wxID_ANY);
    blob_noDataText = new wxStaticText(blob_noData, wxID_ANY, "");
    blob_text = new EditBlobDialogSTCText(notebook, wxID_ANY);
    blob_binary = new EditBlobDialogSTC(notebook, wxID_ANY);
    progress = new EditBlobDialogProgressSizer(getControlsPanel());
   
    // dialog "menu" buttons
    button_menu_blob = new wxButton(getControlsPanel(),
        Cmds::BlobEditor_Menu_BLOB, _("&BLOB"));

    // dialog buttons
    button_reset = new wxButton(getControlsPanel(), wxID_RESET, _("&Reset"));
    button_save  = new wxButton(getControlsPanel(), wxID_SAVE, _("&Save"));

    CommandManager cm;
    buildMenus(cm);

    set_properties();
    do_layout();

    runningM = true;  // enable wxNotebookPageChanged-Events
}

EditBlobDialog::~EditBlobDialog()
{
    /*
    // activate later if plugin will be implemented
    m_libEditBlobUseCount--;
    if (m_libEditBlobUseCount = 0)
    {
        // TODO unload lib
      if (m_libEditBlob)
        delete m_libEditBlob;
    }
    */
    delete menu_blob;
    cacheDelete();
}

void EditBlobDialog::buildMenus(CommandManager& cm)
{
    menu_blob = new wxMenu(); // dynamic menus, created at runtime

    menu_blob->Append(Cmds::BlobEditor_Menu_BLOBLoadFromFile,
        cm.getMainMenuItemText(_("&Load from File..."), Cmds::BlobEditor_Menu_BLOBLoadFromFile));
    menu_blob->Append(Cmds::BlobEditor_Menu_BLOBSaveToFile,
        cm.getMainMenuItemText(_("&Save to File"), Cmds::BlobEditor_Menu_BLOBSaveToFile));
}

void EditBlobDialog::cacheDelete()
{
    dataValidM.clear();
    if (!cacheM)
        return;
    delete cacheM;
    cacheM = 0;
}

void EditBlobDialog::notebookAddPageById(int pageId)
{
    // if page is already added -> do nothing
    int i = notebookGetPageIndexById(pageId);
    if (i > -1)
        return;
    // add page to notebook
    switch (pageId)
    {
        case noData :
            blob_noData->Show();
            notebook->AddPage(blob_noData, _("No data"), false);
            break;
        case binary :
            blob_binary->Show();
            notebook->AddPage(blob_binary, _("Binary"), false);
            break;
        case text :
            blob_text->Show();
            notebook->AddPage(blob_text, _("Text"),   false);
            break;
    }
}

int EditBlobDialog::notebookGetPageIndexById(int pageId)
{
    for (unsigned int i = 0; i < notebook->GetPageCount(); i++)
        if (notebook->GetPage(i)->GetId() == pageId)
            return i;
    return -1;
}

void EditBlobDialog::notebookRemovePageById(int pageId)
{
    int i = notebookGetPageIndexById(pageId);
    if (i > -1)
    {
        // Hide the page that will be removed
        // That is needed otherwise the removed
        // page is still visible on the wxNotebook.
        switch (pageId)
        {
            case noData :
                blob_noData->Hide();
                break;
            case binary :
                blob_binary->Hide();
                break;
            case text :
                blob_text->Hide();
                break;
        }
        notebook->RemovePage(i);
    }
}

void EditBlobDialog::notebookSelectPageById(int pageId)
{
    int i = notebookGetPageIndexById(pageId);
    if (i > -1)
        notebook->ChangeSelection(i);
}

void EditBlobDialog::closeDontSave()
{
    // We dont want to save the blob data back to DB.
    dataModifiedM = false;
    Close();
}

bool EditBlobDialog::setBlob(DataGrid* dg, DataGridTable* dgt,
    IBPP::Statement* st, unsigned row, unsigned col, bool saveOldValue)
{
    // cancel load progress or wait for save progress
    progressCancel();
    
    // Save last blob value if modified
    if (saveOldValue)
        saveBlob();
    dataGridTableM = dgt;
    dataGridM = dg;
    statementM = st;
    rowM = row;
    colM = col;
    readonlyM = dataGridTableM->isReadonlyColumn(colM);

    // generator blob fieldname
    wxString tableName = dgt->getTableName();
    wxString fieldName = dg->GetColLabelValue(dg->GetGridCursorCol());
    fieldNameM  = tableName + "." + fieldName;

    dialogCaptionM = wxString::Format(_("Edit BLOB: %s #%i"), fieldNameM.c_str(), rowM+1);
    SetTitle(dialogCaptionM);

    return loadBlob();
}

bool EditBlobDialog::loadBlob()
{
    bool isTextual;
    bool isBlob = dataGridTableM->isBlobColumn(colM, &isTextual);

    // disable wxNotebookPageChanged-Events
    runningM = false;
    bool res = false;
    // we load data from blob now, so we dont need the cache ATM
    // if the user changes data and switches the notebook-page
    // the cach will be created again
    cacheDelete();
    dataUpdateGUI();

    if (isBlob)
    {
        // Loading BLOB into Editor
        IBPP::Blob* tmpBlob = dataGridTableM->getBlob(rowM, colM, false);
        if (tmpBlob != 0)
            blobM = *tmpBlob;
        else
            blobM = 0;

        FRInputBlobStream inpblob(blobM);

        if (!isTextual)
        {
            res = loadFromStreamAsBinary(inpblob, blobM == 0, _("Loading BLOB into editor."));
            editorModeM = binary;
        }
        else
        {
            res = loadFromStreamAsText(inpblob, blobM == 0, _("Loading BLOB into editor."));
            editorModeM = text;
        }

        dataValidM.insert(editorModeM);
    }

    if ((isBlob) && (res))
    {
        notebookAddPageById(binary);
        notebookAddPageById(text);
        notebookRemovePageById(noData);
        notebookSelectPageById(editorModeM);

        dataSetModified(false, editorModeM);
        button_reset->SetLabel(_("&Reset"));
        button_reset->Disable();
    }
    else
    {
        blobM = 0;
        notebookAddPageById(noData);
        notebookRemovePageById(binary);
        notebookRemovePageById(text);
        notebookSelectPageById(noData);

        wxString noDataInfo;
        if (isBlob)
            noDataInfo = _("The loading operation was canceled.");
        else
            noDataInfo = _("No BLOB.");
        blob_noDataText->SetLabel(noDataInfo);
        // needed to center blob_noDataText
        blob_noData->GetSizer()->Layout();

        editorModeM = noData;
        dataSetModified(false, editorModeM);
        if (isBlob)
        {
            button_reset->SetLabel(_("&Load"));
            button_reset->Enable();
        }
        else
        {
            button_reset->SetLabel(_("&Reset"));
            button_reset->Disable();
        }
        res = true;
    }

    // enable wxNotebookPageChanged-Events
    runningM = true;

    return res;
}

bool EditBlobDialog::loadFromStreamAsText(wxInputStream& stream, bool isNull, const wxString& progressTitle)
{
    if (isNull)
    {
        loadingM = true;
        blob_text->SetReadOnly(false);
        blob_text->setIsNull(true);
        blob_text->SetReadOnly(readonlyM);
        loadingM = false;
        dataSetModified(false, text);
        return true;
    }

    int toread = stream.GetSize();
    progressBegin(progressTitle, toread, true);
    // disable OnDataModified event
    loadingM = true;

    // set the wxStyledTextControl to ReadOnly = false to modify the text
    blob_text->SetReadOnly(false);
    blob_text->ClearAll();
    // allocate a buffer of the full size that is needed
    // for the text. So we have no troubles with splittet
    // multibyte-chars./amaier
    char* buffer = (char*)malloc(toread+1);
    if (buffer == NULL)
    {
        showErrorDialog(this, _("ERROR"), _("Not enough Memory!"),
            AdvancedMessageDialogButtonsOk());
        return false;
    }

    char* bufptr = buffer;
    int readed = 0;
    // Load text in 32k-Blocks.
    // So we can give the user the ability to cancel.
    while ((!progress->isCanceled()) && (readed < toread))
    {
        int nextread = std::min(32767, toread - readed);
        stream.Read((void*)bufptr, nextread);
        int lastread = stream.LastRead();
        if (lastread < 1)
            break;
        bufptr += lastread;
        readed += lastread;
        progress->stepProgress(lastread);
    }
    buffer[readed] = '\0';

    if (!progress->isCanceled())
    {
        blob_text->SetText(std2wxIdentifier(buffer, converterM));
    }

    free(buffer);
    progressEnd();
    blob_textSetReadonly(readonlyM);

    // enable OnDataModified event
    loadingM = false;
    dataSetModified(false, text);

    return !progress->isCanceled();
}

bool EditBlobDialog::loadFromStreamAsBinary(wxInputStream& stream, bool isNull, const wxString& progressTitle)
{
    if (isNull)
    {
        loadingM = true;
        blob_binary->SetReadOnly(false);
        blob_binary->setIsNull(true);
        blob_binary->SetReadOnly(true);
        loadingM = false;
        dataSetModified(false, binary);
        return true;
    }

    progressBegin(progressTitle, stream.GetSize(), true);

    // disable OnDataModified event
    loadingM = true;

    blob_binary->Freeze();

    // set the wxStyledTextControl to ReadOnly = false to modify the text
    blob_binary->SetReadOnly(false);
    blob_binary->ClearAll();
    int col  = 0;
    int line = 0;
    wxString txtLine;

    while (!progress->isCanceled())
    {
        char buffer[32768];
        stream.Read((void*)buffer, 32767);
        int size = stream.LastRead();
        if (size < 1)
            break;
        buffer[size] = '\0';

        int bufpos = 0;
        while (bufpos < size)
        {
            txtLine += wxString::Format( "%02X", (unsigned char)(buffer[bufpos]) );
            bufpos++;
            col++;

            if ((col % 8) == 0)
                txtLine += " ";
            if (col >= 32)
            {
                blob_binary->AddText(txtLine + "\n");
                txtLine = "";
                col = 0;
                line++;
            }
        }
        progress->stepProgress(size);
    }
    // add the last line if col > 0
    if (!progress->isCanceled())
    {
        if (col > 0)
            blob_binary->AddText(txtLine);
    }

    // Prepare text styling data
    const int Columns = 4;
    const int BytesPerColumn = 8;
    const int CharsPerColumn = 2 * BytesPerColumn;
    // a space after each column, and one additional byte for the end-of-line
    const int CharsPerLine = Columns * (CharsPerColumn + 1) + 1;
    std::vector<char> styleBytes(CharsPerLine, '\0');
    for (int col1 = 0; col1 < Columns; ++col1)
    {
        int colStart = col1 * (CharsPerColumn + 1);
        for (int charInCol = 1; charInCol < BytesPerColumn; charInCol += 2)
        {
            // the two chars of every odd byte have different color
            styleBytes[colStart + 2 * charInCol] = '\1';
            styleBytes[colStart + 2 * charInCol + 1] = '\1';
        }
    }
    // Set text styling
    blob_binary->StartStyling(0);
    for (int i = 0; i < blob_binary->GetLineCount(); i++)
        blob_binary->SetStyleBytes(CharsPerLine, &styleBytes[0]);
    progressEnd();
    blob_binary->SetReadOnly(true);

    blob_binary->Thaw();

    // enable OnDataModified event
    loadingM = false;
    dataSetModified(false, binary);

    return !progress->isCanceled();
}

bool EditBlobDialog::saveToStream(wxOutputStream& stream, bool* isNull, const wxString& progressTitle)
{
    progressBegin(progressTitle, 0, false);
    switch (editorModeM)
    {
        case binary :
            {
                *isNull = blob_binary->getIsNull();
                if (*isNull)
                  break;

                const int maxBufSize = 32768;
                char buffer[maxBufSize];
                int bufSize = 0;
                wxString txt = blob_binary->GetText();
                wxString::const_iterator txtIt;

                txtIt = txt.begin();
                while ((txtIt != txt.end()) && (!progress->isCanceled()))
                {
                    wxChar ch1 = *txtIt;
                    txtIt++;
                    // skip spaces and cr
                    if ((ch1 == ' ') || (ch1 == 0x0A) || (ch1 == 0x0D))
                        continue;
                    // that should never happen
                    // but it would be possible if binary data is corrupted
                    if (txtIt == txt.end())
                        throw FRError(_("Internal error. (Binary data seems corrupted.)"));
                    wxChar ch2 = *txtIt;
                    txtIt++;

                    int dig1 = 0;
                    int dig2 = 0;

                    if (isdigit(ch1))
                        dig1 = ch1 - '0';
                    else if ((ch1 >= 'A') && (ch1 <= 'F'))
                        dig1 = ch1 - 'A' + 10;
                    else if ((ch1 >= 'a') && (ch1 <= 'f'))
                        dig1 = ch1 - 'a' + 10;
                    else
                        throw FRError(wxString::Format(_("Wrong HEX-value: %s"), ch1));

                    if (isdigit(ch2))
                        dig2 = ch2 - '0';
                    else if ((ch2 >= 'A') && (ch2 <= 'F'))
                        dig2 = ch2 - 'A' + 10;
                    else if ((ch2 >= 'a') && (ch2 <= 'f'))
                        dig2 = ch2 - 'a' + 10;
                    else
                        throw FRError(wxString::Format(_("Wrong HEX-value: %s"), ch1));

                    buffer[bufSize] = dig1 * 16 + dig2;

                    if (bufSize >= maxBufSize-1)
                    {
                        stream.Write(buffer, bufSize);
                        progress->stepProgress(bufSize);
                        bufSize = 0;
                    }
                    else bufSize++;
                };
                if (bufSize > 0)
                {
                    stream.Write(buffer, bufSize);
                    progress->stepProgress(bufSize);
                }
            }
            break;
        case text :
            {
                *isNull = blob_text->getIsNull();
                if (*isNull)
                  break;

                
                std::string txt = wx2std(blob_text->GetText(), converterM);
                stream.Write(txt.c_str(), txt.length());
            }
            break;
        default :
            throw FRError(_("Unknown editormode!"));
    }
    progressEnd();

    return !progress->isCanceled();
}

void EditBlobDialog::set_properties()
{
    dialogCaptionM = "";
    SetTitle(dialogCaptionM);
    int style = GetWindowStyle();
    SetWindowStyle(style | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxSYSTEM_MENU
        | wxSTAY_ON_TOP);
    blob_noData->SetId(noData);
    blob_text->SetId(text);
    blob_text->SetModEventMask(wxSTC_MODEVENTMASKALL);
    blob_binary->SetId(binary);
    blob_binary->SetModEventMask(wxSTC_MODEVENTMASKALL);

    button_reset->Enable(false);
    button_save->Enable(false);
}

void EditBlobDialog::do_layout()
{
    // Maybe we can find a better solution to get the margin font
    // I assume here it is the default font./amaier
    int marginCharWidth = blob_binary->TextWidth(0, "9");
    // *** TEXT-EDIT-LAYOUT ***
    blob_text->SetSizeHints(200, 100);
    blob_text->SetSize(200, 100);
    // fixed-width font (i think it is better for showing)
    wxFont fTxt(frlayoutconfig().getEditorFontSize(),
        wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
        false);
    blob_text->StyleSetFont(0, fTxt);
    // numbering on text should be usefull
    blob_text->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    blob_text->SetMarginWidth(0, marginCharWidth * 5);
    blob_text->SetMarginWidth(1, 0);
    blob_text->SetMarginWidth(2, 0);

    // *** BINARY-SHOW-LAYOUT ***
    // fixed-width font
    wxFont fBin(frlayoutconfig().getEditorFontSize(),
        wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
        false);
    blob_binary->StyleSetFont(0, fBin);
    blob_binary->StyleSetBackground(0, frlayoutconfig().getReadonlyColour());
    blob_binary->StyleSetFont(1, fBin);
    blob_binary->StyleSetBackground(1, frlayoutconfig().getReadonlyColour());
    blob_binary->StyleSetForeground(1, "MEDIUM BLUE");
    blob_binary->StyleSetBackground(10, frlayoutconfig().getReadonlyColour());
    // set with of the viewport to avoid "empty" space after each line
    blob_binary->SetScrollWidth(600);
    // set up a left-margin for numbering lines
    // TODO - calc with of dialog to fit excact a binary line without scrolling"
    blob_binary->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    blob_binary->SetMarginWidth(0, marginCharWidth * 5);
    blob_binary->SetMarginWidth(1, 0);
    blob_binary->SetMarginWidth(2, 0);
    // Background-color for araes with no text
    blob_binary->SetSizeHints(200, 100);
    blob_binary->SetSize(200, 100);
    blob_binary->StyleSetBackground(wxSTC_STYLE_DEFAULT, frlayoutconfig().getReadonlyColour());
    blob_binary->SetReadOnly(true);

    // center no-data-label on no-data-panel horizontal and vertical
    wxBoxSizer* blob_noDataSizerH = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* blob_noDataSizerV = new wxBoxSizer(wxVERTICAL);
    blob_noDataSizerV->Add(blob_noDataText, 0, wxALIGN_CENTER_HORIZONTAL, 0);
    blob_noDataSizerH->Add(blob_noDataSizerV, 1, wxALIGN_CENTER_VERTICAL, 0);
    blob_noData->SetSizer(blob_noDataSizerH);

    notebookAddPageById(noData);
    notebookAddPageById(binary);
    notebookAddPageById(text);

    wxBoxSizer* sizerTop = new wxBoxSizer(wxHORIZONTAL);
    sizerTop->Add(button_menu_blob, 0, wxALIGN_LEFT);
    sizerTop->AddSpacer(styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerTop->Add(progress, 1, wxEXPAND);
    
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    //sizerControls->Add(button_menu_blob, 0, wxALIGN_LEFT);
    sizerControls->Add(sizerTop, 0, wxEXPAND);
    sizerControls->AddSpacer(styleguide().getFrameMargin(wxLEFT));
    sizerControls->Add(notebook, 1, wxEXPAND);
    //sizerControls->AddSpacer(styleguide().getFrameMargin(wxLEFT));
    //sizerControls->Add(progress, 0, wxEXPAND);
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_reset, button_save);

    layoutSizers(sizerControls, sizerButtons, true);

    SetSize(620, 400);
    Centre();
    
    progress->Layout();
}

void EditBlobDialog::saveBlob()
{
    // If there is no blob loaded (no Data) OR
    // data wasn't modified and cache isn't initialized (cacheM = 0)
    // then data was not changed.
    if ((editorModeM == noData) || ((!dataModifiedM) && (!cacheM)))
        return;

    // Save Editor-Data into BLOB
    DataGridRowsBlob b = dataGridTableM->setBlobPrepare(rowM, colM);
    // if nothing was modified and the cache is created we can
    // directly write the cache to the blob. This saves time.
    char buffer[32768];
    bool ok = false;
    wxString progressTitle = _("Saving editor-data.");
    if ((!dataModifiedM) && (cacheM))
    {
        progressBegin(progressTitle, 0, false);

        if (cacheIsNullM)
        {

            b.blob = 0;
        }
        else
        {
            wxMemoryInputStream inBuf(*cacheM);

            inBuf.Read((void*)buffer, 32767);
            int bufLen = inBuf.LastRead();
            b.blob->Create();
            while ((bufLen > 0) && (!progress->isCanceled()))
            {
                b.blob->Write(buffer, bufLen);
                inBuf.Read((void*)buffer, 32767);
                bufLen = inBuf.LastRead();
            }
            b.blob->Close();
        }

        ok = !progress->isCanceled();
        
        progressEnd();
    }
    else
    {
        bool isNull;
        FROutputBlobStream bs(b.blob);
        ok = saveToStream(bs, &isNull, progressTitle);
        bs.Close();

        if (isNull)
            b.blob = 0;
    }

    if (!ok)
        return;

    if (b.blob == 0)
        dataGridTableM->setValueToNull(b.row, b.col);
    else
    {
        dataGridTableM->setBlob(b);
    }
    blobM = b.blob;

    // update datagrid to force an update (in GUI) of the changed blob-value
    // NOTE: There are two reasons to call it
    // 1) The data grid has to be updated to show the new blob value
    // 2) There will be a error if user changes to another blob and
    //    then again to the same. If the blob is selected again the
    //    cell will be updated and opens the blob. This will happen
    //    while the cancel-dialog in loadBlob is shown. (wxYieldIfNeeded)
    //    At this moment the blob is already opend by loadBlob and
    //    the IBPP::LocicalException - blob already open will occur.
    // amaier/2009-07-19
    dataGridM->refreshAndInvalidateAttributes();

    cacheDelete();

    dataSetModified(false, editorModeM);
    dataUpdateGUI();
}

void EditBlobDialog::OnClose(wxCloseEvent& event)
{
    // Save implicit if data was modified
    saveBlob();
    // destroy the window
    if (event.CanVeto())
    {
        event.Veto();
        this->Hide();
    }
    else
        this->Destroy();
}

void EditBlobDialog::OnResetButtonClick(wxCommandEvent& WXUNUSED(event))
{
    loadBlob();
}

void EditBlobDialog::OnSaveButtonClick(wxCommandEvent& WXUNUSED(event))
{
    saveBlob();
}

void EditBlobDialog::OnNotebookPageChanged(wxNotebookEvent& event)
{
    if (!runningM)
        return;
    int page = event.GetSelection();
    int oldPage = event.GetOldSelection();
    if ((page < 0) || (oldPage < 0))
        return;

    int pageId = notebook->GetPage(page)->GetId();

    if (pageId == noData)
        return;

    // Save data to cache
    // We only store the data if it was changed by the user
    if (dataModifiedM)
    {
        // Maybe we have to initialize the cache (cacheM)
        if (cacheM)
            delete cacheM;
        cacheM = new wxMemoryOutputStream(0, 0);

        if (!saveToStream(*cacheM, &cacheIsNullM, _("Switching editor-mode. (Saving)")))
        {
            showErrorDialog(this, _("ERROR"),
                _("An error occurred while switching editor-mode. (Saving)"),
                AdvancedMessageDialogButtonsOk());
            notebook->ChangeSelection(oldPage);
            return;
        }
    }

    EditorMode newEditorMode = (EditorMode)pageId;
    // Load data from cache or blob - only if the data is not already loaded (valid)
    if ((dataModifiedM) || dataValidM.find(newEditorMode) == dataValidM.end())
    {
        bool loadOk = false;
        bool isNull;
        wxString loadTitle = _("Switching editor-mode. (Loading)");
        wxInputStream* inBuf;
        if (cacheM)
        {
            inBuf = new wxMemoryInputStream(*cacheM);
            isNull = cacheIsNullM;
        }
        else
        {
            inBuf = new FRInputBlobStream(blobM);
            isNull = (blobM == 0);
        }

        switch (pageId)
        {
            case binary :
                loadOk = loadFromStreamAsBinary(*inBuf, isNull, loadTitle);
                break;
            case text :
                loadOk = loadFromStreamAsText(*inBuf, isNull, loadTitle);
                break;
        }

        delete inBuf;

        if (!loadOk)
        {
            showErrorDialog(this, _("ERROR"),
                _("An error occurred while switching editor-mode. (Loading)"),
                AdvancedMessageDialogButtonsOk());
            notebook->ChangeSelection(oldPage);
            //event.Veto();
            return;
        }
        dataValidM.insert(newEditorMode);
    }
    editorModeM = newEditorMode;
}

void EditBlobDialog::OnProgressCancel(wxCommandEvent& WXUNUSED(event))
{ 
    progress->cancel();
}

void EditBlobDialog::OnDataModified(wxStyledTextEvent& WXUNUSED(event))
{
    if (loadingM)
        return;
    dataSetModified(true, editorModeM);
}

void EditBlobDialog::OnMenuBLOBButtonClick(wxCommandEvent& WXUNUSED(event))
{   
    int h = button_menu_blob->GetSize().GetHeight();
    button_menu_blob->PopupMenu(menu_blob, 0, h);
}

void EditBlobDialog::OnMenuBLOBLoadFromFile(wxCommandEvent& WXUNUSED(event))
{
    if (editorModeM == noData)
        throw FRError(_("Not a BLOB column"));
    wxString filename = ::wxFileSelector(_("Select a file"), "",
        "", "", "*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
    if (filename.IsEmpty())
        return;

    cacheDelete();
    
    bool res;
    wxFileInputStream fs(filename);
    if (editorModeM == binary)
        res = loadFromStreamAsBinary(fs, false, _("Loading BLOB into editor."));
    else
        res = loadFromStreamAsText(fs, false, _("Loading BLOB into editor."));

    if (res)
        dataSetModified(true, editorModeM);
}

void EditBlobDialog::OnMenuBLOBSaveToFile(wxCommandEvent& WXUNUSED(event))
{
    //DataGridTable* dgt = grid_data->getDataGridTable();
    //if (!dgt || !grid_data->GetNumberRows())
    //    return;
    //if (!dgt->isBlobColumn(grid_data->GetGridCursorCol()))
    //    throw FRError(_("Not a BLOB column"));
    if (editorModeM == noData)
        throw FRError(_("Not a BLOB column"));
    wxString filename = ::wxFileSelector(_("Select a file"), "",
        "", "", "*",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (filename.IsEmpty())
        return;

    //ProgressDialog pd(this, _("Saving BLOB to file"));
    //pd.Show();
    //dgt->exportBlobFile(filename, grid_data->GetGridCursorRow(),
    //    grid_data->GetGridCursorCol(), &pd);
    wxFileOutputStream fs(filename);
    bool dummy;
    saveToStream(fs, &dummy, _("Importing BLOB from file"));
}

void EditBlobDialog::dataUpdateGUI()
{
    wxString status;
    bool canSave;
    if ((dataModifiedM) || (cacheM))
    {
        status = "*";
        canSave = true;
    }
    else
    {
        status = "";
        canSave = false;
    }

    SetTitle(dialogCaptionM+status);
    button_reset->Enable(canSave);
    button_save->Enable(canSave);
}

void EditBlobDialog::dataSetModified(bool value, EditorMode editorMode)
{
    if (dataModifiedM == value)
        return;
    dataModifiedM = value;
    // if the data is modified all other allready loaded data
    // (binary,text,http,image,...) gets invalid
    dataValidM.clear();
    dataValidM.insert(editorMode);

    dataUpdateGUI();
}

void EditBlobDialog::blob_textSetReadonly(bool readonly)
{
    if (readonly)
    {
        blob_text->StyleSetBackground(0, frlayoutconfig().getReadonlyColour());
        blob_text->StyleSetBackground(wxSTC_STYLE_DEFAULT, frlayoutconfig().getReadonlyColour());
    }
    else
    {
        blob_text->StyleSetBackground(0, "WHITE");
        blob_text->StyleResetDefault();
    }
    blob_text->SetReadOnly(readonly);
}

void EditBlobDialog::progressBegin(const wxString& progressTitle, int maxPosition, bool canCancel)
{
    button_menu_blob->Enable(false);
    notebook->Enable(false);
    
    progressCancel();
    progress->initProgress(progressTitle, maxPosition, canCancel);
    progress->Show(true);
    Update();
}

void EditBlobDialog::progressCancel()
{
    // cancel load progress or 
    if (progress->canCancel())
    {
        while (!progress->isCanceled())
            progress->cancel();
    }
    // wait for save progress
    else
    {
        while (progress->isActive())
            wxSleep(500);
    }
}

void EditBlobDialog::progressEnd()
{
    progress->Hide();

    button_menu_blob->Enable(true);
    notebook->Enable(true);
}

//! event handling
BEGIN_EVENT_TABLE(EditBlobDialog, BaseDialog)
    EVT_MENU(wxID_RESET, EditBlobDialog::OnResetButtonClick) 

    EVT_BUTTON(wxID_RESET, EditBlobDialog::OnResetButtonClick)
    EVT_BUTTON(wxID_SAVE, EditBlobDialog::OnSaveButtonClick)

    EVT_CLOSE(EditBlobDialog::OnClose)

    EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, EditBlobDialog::OnNotebookPageChanged)

    EVT_STC_MODIFIED(wxID_ANY, EditBlobDialog::OnDataModified)
    
    // Menu events
    EVT_BUTTON(Cmds::BlobEditor_Menu_BLOB, EditBlobDialog::OnMenuBLOBButtonClick)
    EVT_MENU(Cmds::BlobEditor_Menu_BLOBLoadFromFile, EditBlobDialog::OnMenuBLOBLoadFromFile)
    EVT_MENU(Cmds::BlobEditor_Menu_BLOBSaveToFile, EditBlobDialog::OnMenuBLOBSaveToFile)

    // Progress
    EVT_BUTTON(Cmds::BlobEditor_ProgressCancel, EditBlobDialog::OnProgressCancel)
END_EVENT_TABLE()

// Helper-Class for streaming into blob / buffer
// frInputBlobStream
FRInputBlobStream::FRInputBlobStream(IBPP::Blob blob)
    :wxInputStream()
{
    blobM = blob;
    if (blobM != 0)
    {
        blobM->Close();
        blobM->Open();
        blobM->Info(&sizeM, 0, 0);
    }
    else
        sizeM = 0;
}

FRInputBlobStream::~FRInputBlobStream()
{
    if (blobM != 0)
        blobM->Close();
}

size_t FRInputBlobStream::OnSysRead(void* buffer, size_t size)
{
    if ((blobM != 0) && (sizeM > 0))
        return blobM->Read(buffer, size);
    else
        return 0;
}

size_t FRInputBlobStream::GetSize() const
{
    return sizeM;
}

// Helper-Class for streaming into blob / buffer
// frOutputBlobStream
FROutputBlobStream::FROutputBlobStream(IBPP::Blob blob)
    :wxOutputStream()
{
    blobM = blob;
    blobM->Create();
}

FROutputBlobStream::~FROutputBlobStream()
{
    Close();
}

size_t FROutputBlobStream::OnSysWrite(const void* buffer, size_t bufsize)
{
    if (bufsize == 0)
        return 0;

    blobM->Write(buffer, bufsize);
    return bufsize;
}

bool FROutputBlobStream::Close()
{
    if (blobM != 0)
        blobM->Close();
    return true;
}

