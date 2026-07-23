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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "gui/SessionMonitorFrame.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/StyleGuide.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"

enum {
    ID_timer_refresh = 3001,
    ID_button_refresh_now,
    ID_check_auto_refresh,
    ID_list_attachments,
    ID_list_statements,
    ID_list_transactions,
    ID_button_disconnect_attachment,
    ID_button_cancel_statement
};

BEGIN_EVENT_TABLE(SessionMonitorFrame, BaseFrame)
    EVT_TIMER(ID_timer_refresh, SessionMonitorFrame::OnTimer)
    EVT_BUTTON(ID_button_refresh_now, SessionMonitorFrame::OnRefreshClick)
    EVT_CHECKBOX(ID_check_auto_refresh, SessionMonitorFrame::OnAutoRefreshToggle)
    EVT_LIST_ITEM_SELECTED(ID_list_statements, SessionMonitorFrame::OnStatementSelected)
    EVT_BUTTON(ID_button_cancel_statement, SessionMonitorFrame::OnCancelStatementClick)
    EVT_BUTTON(ID_button_disconnect_attachment, SessionMonitorFrame::OnDisconnectAttachmentClick)
END_EVENT_TABLE()

SessionMonitorFrame::SessionMonitorFrame(wxWindow* parent, DatabasePtr db)
    : BaseFrame(parent, -1, wxEmptyString), databasePtrM(db), databaseM(db.get())
{
    if (databaseM)
    {
        SetTitle(wxString::Format(_("Live Session & Transaction Monitor - %s"), databaseM->getName_().c_str()));
        databaseM->attachObserver(this, false);
    }

    wxPanel* mainPanel = new wxPanel(this, -1);
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);

    // Top control bar
    wxBoxSizer* sizerTopBar = new wxBoxSizer(wxHORIZONTAL);
    button_refresh_now = new wxButton(mainPanel, ID_button_refresh_now, _("Refresh Now"));
    check_auto_refresh = new wxCheckBox(mainPanel, ID_check_auto_refresh, _("Auto Refresh (5s)"));
    check_auto_refresh->SetValue(true);

    sizerTopBar->Add(button_refresh_now, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 8);
    sizerTopBar->Add(check_auto_refresh, 0, wxALIGN_CENTER_VERTICAL);
    sizerMain->Add(sizerTopBar, 0, wxEXPAND | wxALL, 6);

    notebookM = new wxNotebook(mainPanel, -1);

    // --- Tab 1: Attachments ---
    wxPanel* panelAttachments = new wxPanel(notebookM, -1);
    wxBoxSizer* sizerAtt = new wxBoxSizer(wxVERTICAL);
    list_attachments = new wxListCtrl(panelAttachments, ID_list_attachments, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    list_attachments->InsertColumn(0, _("Attachment ID"), wxLIST_FORMAT_LEFT, 110);
    list_attachments->InsertColumn(1, _("User"), wxLIST_FORMAT_LEFT, 110);
    list_attachments->InsertColumn(2, _("Role"), wxLIST_FORMAT_LEFT, 90);
    list_attachments->InsertColumn(3, _("Remote Address"), wxLIST_FORMAT_LEFT, 130);
    list_attachments->InsertColumn(4, _("Remote Process"), wxLIST_FORMAT_LEFT, 180);
    list_attachments->InsertColumn(5, _("State"), wxLIST_FORMAT_LEFT, 80);
    list_attachments->InsertColumn(6, _("Connected Since"), wxLIST_FORMAT_LEFT, 150);
    list_attachments->InsertColumn(7, _("Wire Crypt"), wxLIST_FORMAT_LEFT, 100);

    button_disconnect_attachment = new wxButton(panelAttachments, ID_button_disconnect_attachment, _("Disconnect Attachment"));
    sizerAtt->Add(list_attachments, 1, wxEXPAND | wxALL, 4);
    sizerAtt->Add(button_disconnect_attachment, 0, wxALIGN_RIGHT | wxALL, 4);
    panelAttachments->SetSizer(sizerAtt);
    notebookM->AddPage(panelAttachments, _("Active Attachments"));

    // --- Tab 2: Statements ---
    wxPanel* panelStatements = new wxPanel(notebookM, -1);
    wxBoxSizer* sizerStmt = new wxBoxSizer(wxVERTICAL);
    list_statements = new wxListCtrl(panelStatements, ID_list_statements, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    list_statements->InsertColumn(0, _("Statement ID"), wxLIST_FORMAT_LEFT, 110);
    list_statements->InsertColumn(1, _("Attachment ID"), wxLIST_FORMAT_LEFT, 110);
    list_statements->InsertColumn(2, _("State"), wxLIST_FORMAT_LEFT, 80);
    list_statements->InsertColumn(3, _("Started At"), wxLIST_FORMAT_LEFT, 150);
    list_statements->InsertColumn(4, _("SQL Snippet"), wxLIST_FORMAT_LEFT, 300);

    editor_sql_preview = new SqlEditor(panelStatements, wxID_ANY);
    if (databaseM)
        editor_sql_preview->setKeywords(databaseM->getODSMajor(), databaseM->getODSMinor());
    button_cancel_statement = new wxButton(panelStatements, ID_button_cancel_statement, _("Cancel Query Statement"));

    sizerStmt->Add(list_statements, 1, wxEXPAND | wxALL, 4);
    sizerStmt->Add(new wxStaticText(panelStatements, -1, _("Full Statement SQL Preview:")), 0, wxLEFT | wxTOP, 4);
    sizerStmt->Add(editor_sql_preview, 1, wxEXPAND | wxALL, 4);
    sizerStmt->Add(button_cancel_statement, 0, wxALIGN_RIGHT | wxALL, 4);
    panelStatements->SetSizer(sizerStmt);
    notebookM->AddPage(panelStatements, _("Executing Statements"));

    // --- Tab 3: Transactions ---
    wxPanel* panelTransactions = new wxPanel(notebookM, -1);
    wxBoxSizer* sizerTx = new wxBoxSizer(wxVERTICAL);
    list_transactions = new wxListCtrl(panelTransactions, ID_list_transactions, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    list_transactions->InsertColumn(0, _("Transaction ID"), wxLIST_FORMAT_LEFT, 120);
    list_transactions->InsertColumn(1, _("Attachment ID"), wxLIST_FORMAT_LEFT, 120);
    list_transactions->InsertColumn(2, _("State"), wxLIST_FORMAT_LEFT, 80);
    list_transactions->InsertColumn(3, _("Isolation Mode"), wxLIST_FORMAT_LEFT, 160);
    list_transactions->InsertColumn(4, _("Lock Timeout"), wxLIST_FORMAT_LEFT, 100);
    list_transactions->InsertColumn(5, _("Read Only"), wxLIST_FORMAT_LEFT, 80);
    list_transactions->InsertColumn(6, _("Started At"), wxLIST_FORMAT_LEFT, 150);

    sizerTx->Add(list_transactions, 1, wxEXPAND | wxALL, 4);
    panelTransactions->SetSizer(sizerTx);
    notebookM->AddPage(panelTransactions, _("Active Transactions"));

    sizerMain->Add(notebookM, 1, wxEXPAND | wxALL, 6);

    mainPanel->SetSizer(sizerMain);

    timerRefreshM.SetOwner(this, ID_timer_refresh);
    timerRefreshM.Start(5000); // 5s

    loadMonitoringData();
}

SessionMonitorFrame::~SessionMonitorFrame()
{
    if (timerRefreshM.IsRunning())
        timerRefreshM.Stop();

    if (databaseM && !databaseM->getIsVolative())
        databaseM->detachObserver(this);
}

void SessionMonitorFrame::subjectRemoved(Subject* subject)
{
    if (subject == databaseM)
    {
        databaseM = nullptr;
        databasePtrM.reset();
        Close();
    }
}

void SessionMonitorFrame::update()
{
}

void SessionMonitorFrame::loadMonitoringData()
{
    attachmentsM.clear();
    statementsM.clear();
    transactionsM.clear();

    if (!databaseM || !databaseM->isConnected())
        return;

    try
    {
        auto dalDb = databaseM->getDALDatabase();
        if (!dalDb || !dalDb->isConnected()) return;

        auto tr = dalDb->createTransaction();
        tr->start();

        // 1. Attachments
        try
        {
            auto st = dalDb->createStatement(tr);
            bool isFb4 = databaseM->getInfo().isFB40OrHigher();
            if (isFb4)
            {
                st->prepare("SELECT MON$ATTACHMENT_ID, MON$STATE, MON$USER, MON$ROLE, MON$REMOTE_ADDRESS, MON$REMOTE_PROCESS, MON$TIMESTAMP, MON$WIRE_CRYPT "
                            "FROM MON$ATTACHMENTS ORDER BY MON$ATTACHMENT_ID");
            }
            else
            {
                st->prepare("SELECT MON$ATTACHMENT_ID, MON$STATE, MON$USER, MON$ROLE, MON$REMOTE_ADDRESS, MON$REMOTE_PROCESS, MON$TIMESTAMP "
                            "FROM MON$ATTACHMENTS ORDER BY MON$ATTACHMENT_ID");
            }
            st->execute();
            while (st->fetch())
            {
                SessionAttachmentInfo att;
                att.id = st->getInt64(0);
                att.state = st->isNull(1) ? 0 : st->getInt32(1);
                att.user = wxString::FromUTF8(st->getString(2).c_str());
                att.role = wxString::FromUTF8(st->getString(3).c_str());
                att.remoteAddress = wxString::FromUTF8(st->getString(4).c_str());
                att.remoteProcess = wxString::FromUTF8(st->getString(5).c_str());
                att.timestamp = wxString::FromUTF8(st->getTimestamp(6).c_str());
                if (isFb4 && !st->isNull(7))
                {
                    att.wireCrypt = st->getBool(7) ? _("Enabled") : _("Disabled");
                }
                else
                {
                    att.wireCrypt = _("N/A");
                }
                attachmentsM.push_back(att);
            }
        } catch (...) {}

        // 2. Statements
        try
        {
            auto st = dalDb->createStatement(tr);
            st->prepare("SELECT MON$STATEMENT_ID, MON$ATTACHMENT_ID, MON$STATE, MON$SQL_TEXT, MON$TIMESTAMP "
                        "FROM MON$STATEMENTS WHERE MON$SQL_TEXT IS NOT NULL ORDER BY MON$STATEMENT_ID");
            st->execute();
            while (st->fetch())
            {
                SessionStatementInfo stmt;
                stmt.id = st->getInt64(0);
                stmt.attachmentId = st->getInt64(1);
                stmt.state = st->isNull(2) ? 0 : st->getInt32(2);
                stmt.sqlText = wxString::FromUTF8(st->getString(3).c_str());
                stmt.timestamp = wxString::FromUTF8(st->getTimestamp(4).c_str());
                statementsM.push_back(stmt);
            }
        } catch (...) {}

        // 3. Transactions
        try
        {
            auto st = dalDb->createStatement(tr);
            st->prepare("SELECT MON$TRANSACTION_ID, MON$ATTACHMENT_ID, MON$STATE, MON$ISOLATION_MODE, MON$LOCK_TIMEOUT, MON$READ_ONLY, MON$TIMESTAMP "
                        "FROM MON$TRANSACTIONS ORDER BY MON$TRANSACTION_ID");
            st->execute();
            while (st->fetch())
            {
                SessionTransactionInfo tx;
                tx.id = st->getInt64(0);
                tx.attachmentId = st->getInt64(1);
                tx.state = st->isNull(2) ? 0 : st->getInt32(2);
                int iso = st->isNull(3) ? 0 : st->getInt32(3);
                switch (iso) {
                    case 0: tx.isolationMode = "Consistency"; break;
                    case 1: tx.isolationMode = "Concurrency"; break;
                    case 2: tx.isolationMode = "Read Committed (RV)"; break;
                    case 3: tx.isolationMode = "Read Committed (NRV)"; break;
                    default: tx.isolationMode = "Unknown"; break;
                }
                tx.lockTimeout = st->isNull(4) ? 0 : st->getInt32(4);
                tx.readOnly = st->isNull(5) ? false : (st->getInt32(5) != 0);
                tx.timestamp = wxString::FromUTF8(st->getTimestamp(6).c_str());
                transactionsM.push_back(tx);
            }
        } catch (...) {}

        tr->commit();
    }
    catch (...) {}

    updateAttachmentsUI();
    updateStatementsUI();
    updateTransactionsUI();
}

void SessionMonitorFrame::updateAttachmentsUI()
{
    list_attachments->DeleteAllItems();
    for (size_t i = 0; i < attachmentsM.size(); ++i)
    {
        const auto& att = attachmentsM[i];
        long idx = list_attachments->InsertItem(i, wxString::Format("%lld", (long long)att.id));
        list_attachments->SetItem(idx, 1, att.user);
        list_attachments->SetItem(idx, 2, att.role);
        list_attachments->SetItem(idx, 3, att.remoteAddress);
        list_attachments->SetItem(idx, 4, att.remoteProcess);
        list_attachments->SetItem(idx, 5, att.state == 1 ? _("Active") : _("Idle"));
        list_attachments->SetItem(idx, 6, att.timestamp);
        list_attachments->SetItem(idx, 7, att.wireCrypt.IsEmpty() ? _("N/A") : att.wireCrypt);
    }
}

void SessionMonitorFrame::updateStatementsUI()
{
    list_statements->DeleteAllItems();
    for (size_t i = 0; i < statementsM.size(); ++i)
    {
        const auto& stmt = statementsM[i];
        long idx = list_statements->InsertItem(i, wxString::Format("%lld", (long long)stmt.id));
        list_statements->SetItem(idx, 1, wxString::Format("%lld", (long long)stmt.attachmentId));
        list_statements->SetItem(idx, 2, stmt.state == 1 ? _("Active") : _("Idle"));
        list_statements->SetItem(idx, 3, stmt.timestamp);
        list_statements->SetItem(idx, 4, stmt.sqlText.Mid(0, 80));
    }
}

void SessionMonitorFrame::updateTransactionsUI()
{
    list_transactions->DeleteAllItems();
    for (size_t i = 0; i < transactionsM.size(); ++i)
    {
        const auto& tx = transactionsM[i];
        long idx = list_transactions->InsertItem(i, wxString::Format("%lld", (long long)tx.id));
        list_transactions->SetItem(idx, 1, wxString::Format("%lld", (long long)tx.attachmentId));
        list_transactions->SetItem(idx, 2, tx.state == 1 ? _("Active") : _("Idle"));
        list_transactions->SetItem(idx, 3, tx.isolationMode);
        list_transactions->SetItem(idx, 4, wxString::Format("%d", tx.lockTimeout));
        list_transactions->SetItem(idx, 5, tx.readOnly ? _("Yes") : _("No"));
        list_transactions->SetItem(idx, 6, tx.timestamp);
    }
}

void SessionMonitorFrame::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    if (check_auto_refresh->IsChecked())
        loadMonitoringData();
}

void SessionMonitorFrame::OnRefreshClick(wxCommandEvent& WXUNUSED(event))
{
    loadMonitoringData();
}

void SessionMonitorFrame::OnAutoRefreshToggle(wxCommandEvent& WXUNUSED(event))
{
    if (check_auto_refresh->IsChecked())
    {
        if (!timerRefreshM.IsRunning())
            timerRefreshM.Start(5000);
    }
    else
    {
        if (timerRefreshM.IsRunning())
            timerRefreshM.Stop();
    }
}

void SessionMonitorFrame::OnStatementSelected(wxListEvent& event)
{
    long idx = event.GetIndex();
    if (idx >= 0 && idx < (long)statementsM.size())
    {
        editor_sql_preview->SetText(statementsM[idx].sqlText);
    }
}

void SessionMonitorFrame::OnCancelStatementClick(wxCommandEvent& WXUNUSED(event))
{
    long itemIdx = list_statements->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (itemIdx < 0 || itemIdx >= (long)statementsM.size())
    {
        wxMessageBox(_("Please select an executing statement from the list to cancel."), _("Info"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    const auto& stmt = statementsM[itemIdx];
    int res = ::wxMessageBox(
        wxString::Format(_("Are you sure you want to cancel executing statement ID %lld?\n\nSQL Snippet:\n%s"),
            (long long)stmt.id, stmt.sqlText.Mid(0, 150).c_str()),
        _("Confirm Cancel Query"), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this);

    if (res != wxYES) return;

    try
    {
        auto dalDb = databaseM->getDALDatabase();
        auto tr = dalDb->createTransaction();
        tr->start();
        auto st = dalDb->createStatement(tr);
        std::string cancelSql = "DELETE FROM MON$STATEMENTS WHERE MON$STATEMENT_ID = " + std::to_string(stmt.id) + ";";
        st->prepare(cancelSql);
        st->execute();
        tr->commit();

        ::wxMessageBox(_("Statement cancel signal sent successfully."), _("Query Cancelled"), wxOK | wxICON_INFORMATION, this);
        loadMonitoringData();
    }
    catch (const std::exception& e)
    {
        ::wxMessageBox(wxString::FromUTF8(e.what()), _("Cancel Failed"), wxOK | wxICON_ERROR, this);
    }
}

void SessionMonitorFrame::OnDisconnectAttachmentClick(wxCommandEvent& WXUNUSED(event))
{
    long itemIdx = list_attachments->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (itemIdx < 0 || itemIdx >= (long)attachmentsM.size())
    {
        wxMessageBox(_("Please select an attachment from the list to disconnect."), _("Info"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    const auto& att = attachmentsM[itemIdx];
    int res = ::wxMessageBox(
        wxString::Format(_("Are you sure you want to forcibly disconnect attachment ID %lld (User: %s, App: %s)?"),
            (long long)att.id, att.user.c_str(), att.remoteProcess.c_str()),
        _("Confirm Disconnect Attachment"), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, this);

    if (res != wxYES) return;

    try
    {
        auto dalDb = databaseM->getDALDatabase();
        auto tr = dalDb->createTransaction();
        tr->start();
        auto st = dalDb->createStatement(tr);
        std::string discSql = "DELETE FROM MON$ATTACHMENTS WHERE MON$ATTACHMENT_ID = " + std::to_string(att.id) + ";";
        st->prepare(discSql);
        st->execute();
        tr->commit();

        ::wxMessageBox(_("Disconnect signal sent successfully."), _("Attachment Disconnected"), wxOK | wxICON_INFORMATION, this);
        loadMonitoringData();
    }
    catch (const std::exception& e)
    {
        ::wxMessageBox(wxString::FromUTF8(e.what()), _("Disconnect Failed"), wxOK | wxICON_ERROR, this);
    }
}
