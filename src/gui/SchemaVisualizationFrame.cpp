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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "config/Config.h"
#include "core/ArtProvider.h"
#include "gui/controls/PrintableHtmlWindow.h"
#include "gui/SchemaVisualizationFrame.h"
#include "gui/SchemaHtmlGenerator.h"
#include "metadata/table.h"
#include "metadata/column.h"
#include "metadata/constraints.h"
#include "sql/Identifier.h"

static wxString escapeJsonString(const wxString& str)
{
    wxString res = str;
    res.Replace("\\", "\\\\");
    res.Replace("\"", "\\\"");
    res.Replace("\n", "\\n");
    res.Replace("\r", "\\r");
    res.Replace("\t", "\\t");
    return res;
}

static wxString getDatabaseSchemaAsJson(Database* db)
{
    wxString json = "{\n";
    json += "  \"databaseName\": \"" + escapeJsonString(db->getName_()) + "\",\n";

    // Resolve theme setting
    bool isDark = false;
    int activeStyle = 0;
    config().getValue("StyleActive", activeStyle);
    if (activeStyle == 1) // ThemeLight
        isDark = false;
    else if (activeStyle == 2) // ThemeDark
        isDark = true;
    else // ThemeSystem (0)
        isDark = wxSystemSettings::GetAppearance().IsDark();

    json += wxString::Format("  \"isDark\": %s,\n", isDark ? "true" : "false");
    json += "  \"tables\": [\n";

    TablesPtr tables = db->getTables();
    tables->ensureChildrenLoaded();

    bool firstTable = true;
    for (Tables::iterator it = tables->begin(); it != tables->end(); ++it)
    {
        TablePtr t = *it;
        if (!t)
            continue;
        t->ensureChildrenLoaded();

        if (!firstTable)
            json += ",\n";
        firstTable = false;

        json += "    {\n";
        json += "      \"name\": \"" + escapeJsonString(t->getName_()) + "\",\n";
        
        // Columns
        json += "      \"columns\": [\n";
        bool firstCol = true;
        for (ColumnPtrs::iterator itCol = t->begin(); itCol != t->end(); ++itCol)
        {
            ColumnPtr col = *itCol;
            if (!col)
                continue;

            if (!firstCol)
                json += ",\n";
            firstCol = false;

            json += "        {\n";
            json += "          \"name\": \"" + escapeJsonString(col->getName_()) + "\",\n";
            json += "          \"type\": \"" + escapeJsonString(col->getDatatype()) + "\",\n";
            json += wxString::Format("          \"pk\": %s,\n", col->isPrimaryKey() ? "true" : "false");
            json += wxString::Format("          \"fk\": %s\n", col->isForeignKey() ? "true" : "false");
            json += "        }";
        }
        json += "\n      ]";

        // Foreign keys
        std::vector<ForeignKey>* fks = t->getForeignKeys();
        if (fks && !fks->empty())
        {
            json += ",\n      \"foreignKeys\": [\n";
            bool firstFk = true;
            for (std::vector<ForeignKey>::iterator itFk = fks->begin(); itFk != fks->end(); ++itFk)
            {
                if (!firstFk)
                    json += ",\n";
                firstFk = false;

                json += "        {\n";
                json += "          \"name\": \"" + escapeJsonString((*itFk).getName_()) + "\",\n";
                json += "          \"refTable\": \"" + escapeJsonString((*itFk).getReferencedTable()) + "\",\n";
                
                // local columns
                json += "          \"columns\": [";
                bool firstLocalCol = true;
                for (std::vector<wxString>::const_iterator itColName = (*itFk).begin(); itColName != (*itFk).end(); ++itColName)
                {
                    if (!firstLocalCol)
                        json += ", ";
                    firstLocalCol = false;
                    json += "\"" + escapeJsonString(*itColName) + "\"";
                }
                json += "],\n";

                // referenced columns
                json += "          \"refColumns\": [";
                bool firstRefCol = true;
                const std::vector<wxString>& refCols = (*itFk).getReferencedColumns();
                for (std::vector<wxString>::const_iterator itRefCol = refCols.begin(); itRefCol != refCols.end(); ++itRefCol)
                {
                    if (!firstRefCol)
                        json += ", ";
                    firstRefCol = false;
                    json += "\"" + escapeJsonString(*itRefCol) + "\"";
                }
                json += "]\n";
                json += "        }";
            }
            json += "\n      ]";
        }

        json += "\n    }";
    }

    json += "\n  ]\n";
    json += "}";
    return json;
}

wxString SchemaVisualizationFrame::getFrameId(DatabasePtr db)
{
    if (db)
        return "SchemaVisualizationFrame/" + db->getPath();
    return wxEmptyString;
}

SchemaVisualizationFrame* SchemaVisualizationFrame::findFrameFor(DatabasePtr db)
{
    BaseFrame* bf = frameFromIdString(getFrameId(db));
    if (!bf)
        return 0;
    return dynamic_cast<SchemaVisualizationFrame*>(bf);
}

void SchemaVisualizationFrame::showFrame(wxWindow* parent, DatabasePtr db)
{
    SchemaVisualizationFrame* frame = findFrameFor(db);
    if (frame)
    {
        frame->Raise();
        return;
    }
    frame = new SchemaVisualizationFrame(parent, db);
    frame->Show();
}

SchemaVisualizationFrame::SchemaVisualizationFrame(wxWindow* parent, DatabasePtr db)
    : BaseFrame(parent, -1, wxEmptyString)
{
    databaseM = db;
    htmlWindowM = new PrintableHtmlWindow(this);
    
    SetTitle(_("Schema Visualization - ") + db->getName_());

    wxString json = getDatabaseSchemaAsJson(db.get());
    wxString html = getHtmlTemplate(json);
    htmlWindowM->setPageSource(html);

    setIdString(this, getFrameId(db));
    SetIcon(wxArtProvider::GetIcon(ART_FlameRobin, wxART_FRAME_ICON));
}

SchemaVisualizationFrame::~SchemaVisualizationFrame()
{
}

const wxRect SchemaVisualizationFrame::getDefaultRect() const
{
    return wxRect(-1, -1, 1024, 768);
}

const wxString SchemaVisualizationFrame::getName() const
{
    return "SchemaVisualizationFrame";
}

const wxString SchemaVisualizationFrame::getStorageName() const
{
    wxString name(getName());
    if (databaseM)
        name += Config::pathSeparator + databaseM->getName_();
    return name;
}

wxString SchemaVisualizationFrame::getHtmlTemplate(const wxString& schemaJson)
{
    return fr::getSchemaHtmlTemplate(schemaJson);
}
