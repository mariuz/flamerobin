/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#ifndef FR_SESSIONMONITORFRAME_H
#define FR_SESSIONMONITORFRAME_H

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/timer.h>
#include <vector>

#include "gui/BaseFrame.h"
#include "core/Observer.h"
#include "metadata/database.h"

class SqlEditor;

struct SessionAttachmentInfo
{
    int64_t id;
    int state;
    wxString user;
    wxString role;
    wxString remoteAddress;
    wxString remoteProcess;
    wxString timestamp;
    wxString wireCrypt;
};

struct SessionStatementInfo
{
    int64_t id;
    int64_t attachmentId;
    int state;
    wxString sqlText;
    wxString timestamp;
};

struct SessionTransactionInfo
{
    int64_t id;
    int64_t attachmentId;
    int state;
    wxString isolationMode;
    int lockTimeout;
    bool readOnly;
    wxString timestamp;
};

class SessionMonitorFrame: public BaseFrame, public Observer
{
private:
    DatabasePtr databasePtrM;
    Database* databaseM;
    wxTimer timerRefreshM;

    wxNotebook* notebookM;

    // Attachments tab
    wxListCtrl* list_attachments;
    wxButton* button_disconnect_attachment;
    std::vector<SessionAttachmentInfo> attachmentsM;

    // Statements tab
    wxListCtrl* list_statements;
    SqlEditor* editor_sql_preview;
    wxButton* button_cancel_statement;
    std::vector<SessionStatementInfo> statementsM;

    // Transactions tab
    wxListCtrl* list_transactions;
    std::vector<SessionTransactionInfo> transactionsM;

    wxCheckBox* check_auto_refresh;
    wxButton* button_refresh_now;
    wxButton* button_close;

    virtual void subjectRemoved(Subject* subject);
    virtual void update();

    void loadMonitoringData();
    void updateAttachmentsUI();
    void updateStatementsUI();
    void updateTransactionsUI();

    void OnTimer(wxTimerEvent& event);
    void OnRefreshClick(wxCommandEvent& event);
    void OnAutoRefreshToggle(wxCommandEvent& event);
    void OnStatementSelected(wxListEvent& event);
    void OnCancelStatementClick(wxCommandEvent& event);
    void OnDisconnectAttachmentClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
public:
    SessionMonitorFrame(wxWindow* parent, DatabasePtr db);
    virtual ~SessionMonitorFrame();
};

#endif
