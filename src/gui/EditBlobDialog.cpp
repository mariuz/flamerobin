/*
  Copyright (c) 2004-2009 The FlameRobin Development Team

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


  $Id$

*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/stream.h>

#include "AdvancedMessageDialog.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/EditBlobDialog.h"
#include "gui/ProgressDialog.h"
#include "gui/StyleGuide.h"

// Static members
/* maybe later needed if plugin will be implemented
int EditBlobDialog::m_libEditBlobUseCount = 0;
wxDynamicLibrary* EditBlobDialog::m_libEditBlob = NULL;
*/

EditBlobDialog::EditBlobDialog(wxWindow* parent, wxString& blobName, IBPP::Blob blob,
    DataGridTable* dgt, unsigned row, unsigned col)
    :BaseDialog(parent, -1, wxEmptyString)
{
    runningM = false; // disable wxNotebookPageChanged-Events
    dataModifiedM = false;
    cacheM = 0;
    dialogCaptionM = wxT("");
    dataGridTableM = dgt;
    blobNameM = blobName;
    rowM = row;
    colM = col;
    blobM = blob;
    loadingM = false;

    notebook      = new wxNotebook(getControlsPanel(), wxID_ANY); 
    blob_text     = new wxStyledTextCtrl(notebook, wxID_ANY);
    blob_binary   = new wxStyledTextCtrl(notebook, wxID_ANY);
    
    button_save   = new wxButton(getControlsPanel(), wxID_SAVE, _("&Save"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("&Cancel"));
    
    set_properties();
    do_layout();

    runningM = true;  // enable wxNotebookPageChanged-Events
}

bool EditBlobDialog::Init()
{
    // disable wxNotebookPageChanged-Events
    runningM = false;  
    
    bool isTextual;
    dataGridTableM->isBlobColumn(colM, &isTextual);

    // Loading BLOB into Editor
    blobM->Open();
    FRInputBlobStream inpblob(blobM);

    bool res;
    if (!isTextual)
    {
        notebook->ChangeSelection(0);
        blob_text->Show(false);
        res = LoadFromStreamAsBinary(inpblob, _("Loading BLOB into editor."));
        editorModeM = Binary;
    }
    else
    {
        notebook->ChangeSelection(1);
        res = LoadFromStreamAsText(inpblob, _("Loading BLOB into editor."));
        editorModeM = Text;
    }

    blobM->Close();   
    dataValidM.insert(editorModeM);
    
    // enable wxNotebookPageChanged-Events
    runningM = true;  

    return res;
}

bool EditBlobDialog::LoadFromStreamAsText(wxInputStream& stream, const wxString& progressTitle)
{
    int toread = stream.GetSize();
    ProgressDialog pd(this, wxT(""));
    pd.initProgress(progressTitle, toread);
    pd.Show();
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
    while ((!pd.isCanceled()) && (readed < toread))
    {
        int nextread = std::min(32767, toread - readed);
        stream.Read((void*)bufptr, nextread);
        int lastread = stream.LastRead();
        if (lastread < 1)
            break;
        bufptr += lastread;
        readed += lastread;
        pd.stepProgress(lastread);
    }
    buffer[readed] = '\0';
    
    if (!pd.isCanceled())
    {
        // disable OnDataModified event
        loadingM = true;  
        blob_text->SetText(std2wx(buffer));
        // enable OnDataModified event
        loadingM = false; 
    }
      
    free(buffer);
    pd.Hide();
    
    SetDataModified(false);
    
    return !pd.isCanceled();
}

bool EditBlobDialog::LoadFromStreamAsBinary(wxInputStream& stream, const wxString& progressTitle)
{
    int size = stream.GetSize();
    ProgressDialog pd(this, wxT(""));
    pd.initProgress(progressTitle, size);
    pd.Show();
    
    // disable OnDataModified event
    loadingM = true;  
    // set the wxStyledTextControl to ReadOnly = false to modify the text
    blob_binary->SetReadOnly(false);
    blob_binary->ClearAll();
    int col  = 0;
    int line = 0;
    wxString txtLine = wxT("");
    while (!pd.isCanceled())
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
            txtLine += wxString::Format( wxT("%02X"), (unsigned char)(buffer[bufpos]) );
            bufpos++;
            col++;
            
            if ((col % 8) == 0) 
                txtLine += wxT(" ");
            if (col >= 32)
            {
                blob_binary->AddText(txtLine);
                blob_binary->AddText(wxT("\n"));
                txtLine = wxT("");
                col = 0;
                line++;
            }
        }
        pd.stepProgress(size);
    }
    // add the last line if col > 0
    if (!pd.isCanceled())
    {
        if (col > 0)
            blob_binary->AddText(txtLine);
    }
    // Set textstyling
    blob_binary->StartStyling(0, 0xff);
    for (int i = 0;i < blob_binary->GetLineCount();i++)
    {
        blob_binary->SetStyleBytes(69, "\0\0\1\1\0\0\1\1\0\0\1\1\0\0\1\1\0\0\0\1\1\0\0\1\1\0\0\1\1\0\0\1\1\0\0\0\1\1\0\0\1\1\0\0\1\1\0\0\1\1\0\0\0\1\1\0\0\1\1\0\0\1\1\0\0\1\1\0\0");
    }
    blob_binary->SetReadOnly(true);
    pd.Hide();
    
    // enable OnDataModified event
    loadingM = false; 
    SetDataModified(false);

    return !pd.isCanceled();
}

bool EditBlobDialog::SaveToStream(wxOutputStream& stream, const wxString& progressTitle)
{
    ProgressDialog pd(this, progressTitle); 
    pd.Show();    
    switch (editorModeM)
    {
        case Binary :
            {
                const int maxBufSize = 32768;
                char buffer[maxBufSize]; 
                int bufSize = 0;
                wxString txt = blob_binary->GetText();
                wxString::const_iterator txtIt;

                txtIt = txt.begin();
                while ((txtIt != txt.end()) && (!pd.isCanceled()))
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
                        pd.stepProgress(bufSize);
                        bufSize = 0;
                    } 
                    else bufSize++;
                };
                if (bufSize > 0) 
                {
                    stream.Write(buffer, bufSize);
                    pd.stepProgress(bufSize);
                }
            }
            break;
        case Text : 
            {
                std::string txt = wx2std(blob_text->GetText());
                stream.Write(txt.c_str(), txt.length());
            }
            break;
        default :
            throw FRError(_("Unknown editormode!"));
    }
    pd.Hide();     

    return !pd.isCanceled();
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
    //delete blob_text;
    //delete blob_binary;
    //delete notebook; 
    //delete button_save;
    //delete button_cancel;
    delete cacheM;
}

void EditBlobDialog::set_properties()
{
    dialogCaptionM = wxString::Format(_("Edit BLOB: %s"), blobNameM.c_str());
    SetTitle(dialogCaptionM);
    int style = GetWindowStyle();
    SetWindowStyle(style | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxSYSTEM_MENU);
    blob_text->SetId(Text);
    blob_text->SetModEventMask(wxSTC_MODEVENTMASKALL);
    blob_binary->SetId(Binary);
    blob_binary->SetModEventMask(wxSTC_MODEVENTMASKALL);

    button_save->SetDefault();
    button_save->Enable(false);
}

void EditBlobDialog::do_layout()
{
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);

    // Maybe we can find a better solution to get the margin font
    // I assume here it is the default font./amaier
    int marginCharWidth = blob_binary->TextWidth(0, wxT("9"));
    // *** TEXT-EDIT-LAYOUT ***
    blob_text->SetSizeHints(200, 100);
    blob_text->SetSize(200, 100);
    // fixed-width font (i think it is better for showing)
    wxFont fTxt(styleguide().getEditorFontSize(),
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
    wxFont fBin(styleguide().getEditorFontSize(),
        wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
        false);
    blob_binary->StyleSetFont(0, fBin);
    blob_binary->StyleSetFont(1, fBin);
    blob_binary->StyleSetForeground(1, wxT("MEDIUM BLUE"));
    // set with of the viewport to avoid "empty" space after each line
    blob_binary->SetScrollWidth(600);
    // set up a left-margin for numbering lines
    // TODO - calc with of dialog to fit excact a binary line without scrolling"
    blob_binary->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    blob_binary->SetMarginWidth(0, marginCharWidth * 5);
    blob_binary->SetMarginWidth(1, 0);
    blob_binary->SetMarginWidth(2, 0);
    
    blob_binary->SetSizeHints(200, 100);
    blob_binary->SetSize(200, 100);
    blob_binary->SetReadOnly(true);
    
    notebook->AddPage(blob_binary, _("Binary"), false);
    notebook->AddPage(blob_text, _("Text"),   false);
    
    sizerControls->Add(notebook, 1, wxEXPAND);

    wxSizer* sizerButtons = styleguide().createButtonSizer(button_save, button_cancel);

    layoutSizers(sizerControls, sizerButtons, true);
    
    SetSize(620, 400);
    Centre();
}

void EditBlobDialog::OnSaveButtonClick(wxCommandEvent& WXUNUSED(event))
{
    // Save Editor-Data into BLOB
    DataGridRowsBlob b = dataGridTableM->setBlobPrepare(rowM, colM);
    b.blob->Create();
    // if nothing was modified and the cache is createt we can
    // directly write the cache to the blob to save time.
    char buffer[32768];
    bool ok = false;
    wxString progressTitle = _("Saving editor-data.");
    if ((!dataModifiedM) && (cacheM))
    {
        ProgressDialog pd(this, progressTitle); 
        pd.Show();
        
        wxMemoryInputStream inBuf(*cacheM);

        inBuf.Read((void*)buffer, 32767);
        int bufLen = inBuf.LastRead();
        while ((bufLen > 0) && (!pd.isCanceled())) 
        {
            b.blob->Write(buffer, bufLen);
            inBuf.Read((void*)buffer, 32767);
            bufLen = inBuf.LastRead();
        }
        ok = !pd.isCanceled();
    }
    else
    {
        FROutputBlobStream bs(b.blob);
        ok = SaveToStream(bs, progressTitle);
    }
    b.blob->Close();
    
    if (!ok)
        return;
    
    dataGridTableM->setBlob(b);
    
    runningM = false;
    
    // Close window
    Close();
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
    
    // Save data to cache
    // We only store the data if it was changed by the user
    if (dataModifiedM)
    {
        // Maybe we have to initialize the cache (cacheM)
        if (cacheM)
            delete cacheM;
        cacheM = new wxMemoryOutputStream(0, 0);

        if (!SaveToStream(*cacheM, _("Switching editor-mode. (Saving)")))
        {
            showErrorDialog(this, _("ERROR"), 
                _("A error occured while switching editor-mode. (Saving)"), 
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
        wxString loadTitle = _("Switching editor-mode. (Loading)");
        wxInputStream* inBuf;
        if (cacheM)
        {
            inBuf = new wxMemoryInputStream(*cacheM);
        }
        else
        {
            blobM->Open();
            inBuf = new FRInputBlobStream(blobM);
        }
    
        switch (pageId)
        {
            case Binary :
                loadOk = LoadFromStreamAsBinary(*inBuf, loadTitle);
                break;
            case Text : 
                loadOk = LoadFromStreamAsText(*inBuf, loadTitle);
                break;
        }
    
        // close blob if data was loaded from it
        if (!cacheM)
            blobM->Close();
        delete inBuf;
    
        if (!loadOk)
        {
            showErrorDialog(this, _("ERROR"), 
                _("A error occured while switching editor-mode. (Loading)"), 
                AdvancedMessageDialogButtonsOk());
            notebook->ChangeSelection(oldPage);
            //event.Veto();
            return;
        }
        dataValidM.insert(newEditorMode);
    }
    editorModeM = newEditorMode;
}

void EditBlobDialog::OnDataModified(wxStyledTextEvent& event)
{
    if (loadingM)
        return;
    SetDataModified(true);
}

void EditBlobDialog::SetDataModified(bool value)
{
    if (dataModifiedM == value)
        return;
    dataModifiedM = value;
    
    wxString status;
    bool canSave;
    if ((value) || (cacheM))
    {
        status = wxT("*");
        canSave = true;
    }
    else    
    {
        status = wxT("");
        canSave = false;
    }
    
    SetTitle(dialogCaptionM+status);
    button_save->Enable(canSave);
    
    // if the data is modified all other allready loaded data 
    // (binary,text,http,image,...) gets invalid
    dataValidM.clear();
    dataValidM.insert(editorModeM);
}

//! event handling
BEGIN_EVENT_TABLE(EditBlobDialog, BaseDialog)
    EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, EditBlobDialog::OnNotebookPageChanged)
    EVT_BUTTON(wxID_SAVE, EditBlobDialog::OnSaveButtonClick)
    EVT_STC_MODIFIED(EditBlobDialog::Text, EditBlobDialog::OnDataModified)
    EVT_STC_MODIFIED(EditBlobDialog::Binary, EditBlobDialog::OnDataModified)
END_EVENT_TABLE()

// Helper-Class for streaming into blob / buffer
// frInputBlobStream
FRInputBlobStream::FRInputBlobStream(IBPP::Blob blob)
    :wxInputStream()
{
    blobM = blob;
    blobM->Info(&sizeM, 0, 0);
}

FRInputBlobStream::~FRInputBlobStream()
{
}

size_t FRInputBlobStream::OnSysRead(void* buffer, size_t size)
{
    return blobM->Read(buffer, size);
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
}

FROutputBlobStream::~FROutputBlobStream()
{
}

size_t FROutputBlobStream::OnSysWrite(const void* buffer, size_t bufsize)
{
    if (bufsize == 0)
        return 0;
        
    blobM->Write(buffer, bufsize);
    return bufsize;
}
