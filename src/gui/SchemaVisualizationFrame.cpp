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
    wxString html = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <title>Schema Visualization</title>\n"
"    <script src=\"https://unpkg.com/react@18/umd/react.production.min.js\"></script>\n"
"    <script src=\"https://unpkg.com/react-dom@18/umd/react-dom.production.min.js\"></script>\n"
"    <script src=\"https://unpkg.com/@babel/standalone/babel.min.js\"></script>\n"
"    <script src=\"https://cdn.tailwindcss.com\"></script>\n"
"    <style>\n"
"        html, body, body#viz-body {\n"
"            background-color: #0b0f19 !important;\n"
"            color: #f3f4f6 !important;\n"
"            margin: 0 !important;\n"
"            padding: 0 !important;\n"
"            font-family: ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, \"Helvetica Neue\", Arial, sans-serif !important;\n"
"            overflow: hidden !important;\n"
"            user-select: none !important;\n"
"        }\n"
"        .grid-bg {\n"
"            background-image: radial-gradient(rgba(255, 255, 255, 0.08) 1.5px, transparent 1.5px);\n"
"            background-size: 24px 24px;\n"
"        }\n"
"        /* Custom scrollbar */\n"
"        ::-webkit-scrollbar {\n"
"            width: 8px;\n"
"            height: 8px;\n"
"        }\n"
"        ::-webkit-scrollbar-track {\n"
"            background: rgba(0, 0, 0, 0.2);\n"
"        }\n"
"        ::-webkit-scrollbar-thumb {\n"
"            background: rgba(255, 255, 255, 0.15);\n"
"            border-radius: 4px;\n"
"        }\n"
"        ::-webkit-scrollbar-thumb:hover {\n"
"            background: rgba(255, 255, 255, 0.3);\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body id=\"viz-body\">\n"
"    <div id=\"root\"></div>\n"
"\n"
"    <script type=\"text/babel\">\n"
"        const { useState, useEffect, useRef, useMemo } = React;\n"
"\n"
"        const dbSchema = " + schemaJson + ";\n"
"\n"
"        function App() {\n"
"            const [zoom, setZoom] = useState(1);\n"
"            const [pan, setPan] = useState({ x: 0, y: 0 });\n"
"            const [search, setSearch] = useState('');\n"
"            const [draggedTable, setDraggedTable] = useState(null);\n"
"            const [dragOffset, setDragOffset] = useState({ x: 0, y: 0 });\n"
"            const [isPanning, setIsPanning] = useState(false);\n"
"            const [panStart, setPanStart] = useState({ x: 0, y: 0 });\n"
"            const [hoveredTable, setHoveredTable] = useState(null);\n"
"            const [hoveredFk, setHoveredFk] = useState(null);\n"
"\n"
"            const cardWidth = 260;\n"
"            const rowHeight = 30;\n"
"            const headerHeight = 46;\n"
"\n"
"            // Generate initial grid layout positions\n"
"            const initialPositions = useMemo(() => {\n"
"                const pos = {};\n"
"                const colsCount = Math.max(3, Math.ceil(Math.sqrt(dbSchema.tables.length)));\n"
"                const spacingX = 140;\n"
"                const spacingY = 100;\n"
"                \n"
"                dbSchema.tables.forEach((table, index) => {\n"
"                    const col = index % colsCount;\n"
"                    const row = Math.floor(index / colsCount);\n"
"                    pos[table.name] = {\n"
"                        x: col * (cardWidth + spacingX) + 100,\n"
"                        y: row * (300 + spacingY) + 100\n"
"                    };\n"
"                });\n"
"                return pos;\n"
"            }, []);\n"
"\n"
"            const [positions, setPositions] = useState(initialPositions);\n"
"            const containerRef = useRef(null);\n"
"\n"
"            // Zoom handlers\n"
"            const zoomIn = () => setZoom(z => Math.min(3, z + 0.1));\n"
"            const zoomOut = () => setZoom(z => Math.max(0.2, z - 0.1));\n"
"            const resetView = () => {\n"
"                setZoom(1);\n"
"                setPan({ x: 0, y: 0 });\n"
"                setPositions(initialPositions);\n"
"            };\n"
"\n"
"            // Drag table\n"
"            const handleTableMouseDown = (tableName, e) => {\n"
"                if (e.button !== 0) return; // Left click only\n"
"                e.stopPropagation();\n"
"                setDraggedTable(tableName);\n"
"                const pos = positions[tableName] || { x: 0, y: 0 };\n"
"                setDragOffset({\n"
"                    x: (e.clientX / zoom) - pos.x,\n"
"                    y: (e.clientY / zoom) - pos.y\n"
"                });\n"
"            };\n"
"\n"
"            // Pan canvas\n"
"            const handleCanvasMouseDown = (e) => {\n"
"                if (e.button === 0) {\n"
"                    setIsPanning(true);\n"
"                    setPanStart({ x: e.clientX - pan.x, y: e.clientY - pan.y });\n"
"                }\n"
"            };\n"
"\n"
"            const handleMouseMove = (e) => {\n"
"                if (draggedTable) {\n"
"                    setPositions(prev => ({\n"
"                        ...prev,\n"
"                        [draggedTable]: {\n"
"                            x: (e.clientX / zoom) - dragOffset.x,\n"
"                            y: (e.clientY / zoom) - dragOffset.y\n"
"                        }\n"
"                    }));\n"
"                } else if (isPanning) {\n"
"                    setPan({\n"
"                        x: e.clientX - panStart.x,\n"
"                        y: e.clientY - panStart.y\n"
"                    });\n"
"                }\n"
"            };\n"
"\n"
"            const handleMouseUp = () => {\n"
"                setDraggedTable(null);\n"
"                setIsPanning(false);\n"
"            };\n"
"\n"
"            // Calculate connector paths\n"
"            const connections = useMemo(() => {\n"
"                const list = [];\n"
"                dbSchema.tables.forEach(table => {\n"
"                    if (!table.foreignKeys) return;\n"
"                    table.foreignKeys.forEach(fk => {\n"
"                        const srcTable = table.name;\n"
"                        const refTable = fk.refTable;\n"
"                        \n"
"                        const srcPos = positions[srcTable];\n"
"                        const refPos = positions[refTable];\n"
"                        if (!srcPos || !refPos) return;\n"
"\n"
"                        fk.columns.forEach((colName, index) => {\n"
"                            const refColName = fk.refColumns[index];\n"
"                            \n"
"                            const srcColIdx = table.columns.findIndex(c => c.name === colName);\n"
"                            const refColIdx = dbSchema.tables.find(t => t.name === refTable)?.columns.findIndex(c => c.name === refColName);\n"
"\n"
"                            if (srcColIdx === -1 || refColIdx === -1) return;\n"
"\n"
"                            const srcColY = srcPos.y + headerHeight + srcColIdx * rowHeight + (rowHeight / 2);\n"
"                            const refColY = refPos.y + headerHeight + refColIdx * rowHeight + (rowHeight / 2);\n"
"                            const srcOnLeft = srcPos.x < refPos.x;\n"
"                            const startX = srcOnLeft ? srcPos.x + cardWidth : srcPos.x;\n"
"                            const endX = srcOnLeft ? refPos.x : refPos.x + cardWidth;\n"
"\n"
"                            list.push({\n"
"                                id: `${srcTable}.${colName}-${refTable}.${refColName}`,\n"
"                                start: { x: startX, y: srcColY },\n"
"                                end: { x: endX, y: refColY },\n"
"                                srcTable,\n"
"                                refTable,\n"
"                                fkName: fk.name\n"
"                            });\n"
"                        });\n"
"                    });\n"
"                });\n"
"                return list;\n"
"            }, [positions]);\n"
"\n"
"            // Filter tables\n"
"            const filteredTables = useMemo(() => {\n"
"                if (!search.trim()) return dbSchema.tables;\n"
"                const s = search.toLowerCase();\n"
"                return dbSchema.tables.filter(t => \n"
"                    t.name.toLowerCase().includes(s) || \n"
"                    t.columns.some(c => c.name.toLowerCase().includes(s))\n"
"                );\n"
"            }, [search]);\n"
"\n"
"            const getBezierPath = (startX, startY, endX, endY) => {\n"
"                const dx = Math.abs(endX - startX) * 0.6;\n"
"                return `M ${startX} ${startY} C ${startX + (endX > startX ? dx : -dx)} ${startY}, ${endX + (endX > startX ? -dx : dx)} ${endY}, ${endX} ${endY}`;\n"
"            };\n"
"\n"
"            return (\n"
"                <div \n"
"                    ref={containerRef}\n"
"                    className=\"relative w-screen h-screen select-none overflow-hidden bg-[#0c101b]\"\n"
"                    onMouseMove={handleMouseMove}\n"
"                    onMouseUp={handleMouseUp}\n"
"                    onMouseLeave={handleMouseUp}\n"
"                >\n"
"                    {/* Header Toolbar */}\n"
"                    <div className=\"absolute top-4 left-4 right-4 z-50 flex items-center justify-between pointer-events-none\">\n"
"                        <div className=\"flex items-center gap-3 bg-[#161b26]/90 backdrop-blur-md px-4 py-2.5 rounded-xl border border-white/10 shadow-2xl pointer-events-auto\">\n"
"                            <span className=\"text-indigo-400 font-bold tracking-wide text-sm\">SCHEMA VISUALIZER</span>\n"
"                            <div className=\"h-4 w-px bg-white/20\" />\n"
"                            <span className=\"text-white/80 text-xs font-semibold\">{dbSchema.databaseName}</span>\n"
"                            <span className=\"bg-white/10 text-white/60 px-2 py-0.5 rounded text-[10px]\">{dbSchema.tables.length} tables</span>\n"
"                        </div>\n"
"\n"
"                        <div className=\"flex items-center gap-3 pointer-events-auto\">\n"
"                            <input \n"
"                                type=\"text\"\n"
"                                placeholder=\"Search tables or columns...\"\n"
"                                value={search}\n"
"                                onChange={e => setSearch(e.target.value)}\n"
"                                className=\"bg-[#161b26]/90 backdrop-blur-md border border-white/10 rounded-xl px-4 py-2 text-sm text-white focus:outline-none focus:border-indigo-500/50 w-64 shadow-xl transition-all\"\n"
"                            />\n"
"                            \n"
"                            <div className=\"flex bg-[#161b26]/90 backdrop-blur-md border border-white/10 rounded-xl p-1 shadow-xl\">\n"
"                                <button onClick={zoomIn} title=\"Zoom In\" className=\"p-2 text-white/70 hover:text-white hover:bg-white/5 rounded-lg transition-colors\">\n"
"                                    <svg className=\"w-4 h-4\" fill=\"none\" stroke=\"currentColor\" viewBox=\"0 0 24 24\"><path strokeLinecap=\"round\" strokeLinejoin=\"round\" strokeWidth=\"2.5\" d=\"M12 4v16m8-8H4\"/></svg>\n"
"                                </button>\n"
"                                <button onClick={zoomOut} title=\"Zoom Out\" className=\"p-2 text-white/70 hover:text-white hover:bg-white/5 rounded-lg transition-colors\">\n"
"                                    <svg className=\"w-4 h-4\" fill=\"none\" stroke=\"currentColor\" viewBox=\"0 0 24 24\"><path strokeLinecap=\"round\" strokeLinejoin=\"round\" strokeWidth=\"2.5\" d=\"M20 12H4\"/></svg>\n"
"                                </button>\n"
"                                <button onClick={resetView} title=\"Reset View\" className=\"p-2 text-white/70 hover:text-white hover:bg-white/5 rounded-lg transition-colors\">\n"
"                                    <svg className=\"w-4 h-4\" fill=\"none\" stroke=\"currentColor\" viewBox=\"0 0 24 24\"><path strokeLinecap=\"round\" strokeLinejoin=\"round\" strokeWidth=\"2.5\" d=\"M4 4v5h.582m15.356 2A8.001 8.001 0 1121.21 8H18\"/></svg>\n"
"                                </button>\n"
"                            </div>\n"
"                        </div>\n"
"                    </div>\n"
"\n"
"                    {/* Canvas Area */}\n"
"                    <div \n"
"                        className=\"w-full h-full cursor-grab grid-bg\"\n"
"                        onMouseDown={handleCanvasMouseDown}\n"
"                    >\n"
"                        <div \n"
"                            className=\"absolute origin-top-left\"\n"
"                            style={{\n"
"                                transform: `translate(${pan.x}px, ${pan.y}px) scale(${zoom})`\n"
"                            }}\n"
"                        >\n"
"                            {/* SVG Connection Lines */}\n"
"                            <svg className=\"connections-svg overflow-visible\">\n"
"                                <defs>\n"
"                                    <marker id=\"arrow\" viewBox=\"0 0 10 10\" refX=\"6\" refY=\"5\" markerWidth=\"6\" markerHeight=\"6\" orient=\"auto-start-reverse\">\n"
"                                        <path d=\"M 0 1.5 L 8 5 L 0 8.5 z\" fill=\"#4f46e5\" />\n"
"                                    </marker>\n"
"                                    <marker id=\"dot\" viewBox=\"0 0 10 10\" refX=\"5\" refY=\"5\" markerWidth=\"6\" markerHeight=\"6\">\n"
"                                        <circle cx=\"5\" cy=\"5\" r=\"3\" fill=\"#2563eb\" />\n"
"                                    </marker>\n"
"                                </defs>\n"
"                                {connections.map(conn => {\n"
"                                    const isHighlighted = hoveredTable === conn.srcTable || \n"
"                                                          hoveredTable === conn.refTable ||\n"
"                                                          hoveredFk === conn.fkName;\n"
"                                    return (\n"
"                                        <path \n"
"                                            key={conn.id}\n"
"                                            d={getBezierPath(conn.start.x, conn.start.y, conn.end.x, conn.end.y)}\n"
"                                            className=\"transition-all duration-150\"\n"
"                                            style={{\n"
"                                                fill: 'none',\n"
"                                                stroke: isHighlighted ? '#4f46e5' : 'rgba(156, 163, 175, 0.25)',\n"
"                                                strokeWidth: isHighlighted ? 3.5 : 1.5,\n"
"                                                opacity: hoveredTable && !isHighlighted ? 0.2 : 1\n"
"                                            }}\n"
"                                            markerStart=\"url(#dot)\"\n"
"                                            markerEnd=\"url(#arrow)\"\n"
"                                        />\n"
"                                    );\n"
"                                })}\n"
"                            </svg>\n"
"\n"
"                            {/* Table Cards */}\n"
"                            {filteredTables.map(table => {\n"
"                                const pos = positions[table.name] || { x: 0, y: 0 };\n"
"                                const isTableHighlighted = hoveredTable === table.name;\n"
"                                const fade = hoveredTable && !isTableHighlighted;\n"
"\n"
"                                return (\n"
"                                    <div \n"
"                                        key={table.name}\n"
"                                        className={`absolute flex flex-col rounded-2xl border transition-all duration-150 ${\n"
"                                            isTableHighlighted \n"
"                                                ? 'border-indigo-500/50 shadow-[0_8px_32px_0_rgba(79,70,229,0.25)] scale-[1.02]' \n"
"                                                : 'border-white/10 shadow-2xl'\n"
"                                        } bg-[#161b26]/90 backdrop-blur-md w-[260px]`}\n"
"                                        style={{\n"
"                                            left: pos.x,\n"
"                                            top: pos.y,\n"
"                                            opacity: fade ? 0.4 : 1\n"
"                                        }}\n"
"                                        onMouseEnter={() => setHoveredTable(table.name)}\n"
"                                        onMouseLeave={() => setHoveredTable(null)}\n"
"                                    >\n"
"                                        {/* Header */}\n"
"                                        <div \n"
"                                            onMouseDown={e => handleTableMouseDown(table.name, e)}\n"
"                                            className=\"table-header bg-gradient-to-r from-gray-800 to-gray-950 px-4 py-3 rounded-t-2xl border-b border-white/5 flex items-center justify-between cursor-move\"\n"
"                                        >\n"
"                                            <span className=\"font-bold text-white text-xs tracking-wider truncate mr-2\">{table.name}</span>\n"
"                                            <svg className=\"w-3.5 h-3.5 text-gray-500 shrink-0\" fill=\"none\" stroke=\"currentColor\" viewBox=\"0 0 24 24\"><path strokeLinecap=\"round\" strokeLinejoin=\"round\" strokeWidth=\"2.5\" d=\"M4 6h16M4 12h16M4 18h16\"/></svg>\n"
"                                        </div>\n"
"\n"
"                                        {/* Columns List */}\n"
"                                        <div className=\"py-2 flex flex-col text-[12px]\">\n"
"                                            {table.columns.map(col => (\n"
"                                                <div \n"
"                                                    key={col.name} \n"
"                                                    className=\"px-4 py-1.5 flex items-center justify-between hover:bg-white/5\"\n"
"                                                    style={{ height: `${rowHeight}px` }}\n"
"                                                >\n"
"                                                    <div className=\"flex items-center gap-2 truncate\">\n"
"                                                        {col.pk && (\n"
"                                                            <svg className=\"w-3 h-3 text-yellow-500 shrink-0\" fill=\"currentColor\" viewBox=\"0 0 20 20\"><path fillRule=\"evenodd\" d=\"M18 8a6 6 0 01-7.743 5.743L10 14l-1 1-1 1H6v2H2v-4l4.257-4.257A6 6 0 1118 8zm-6-4a1 1 0 100 2 1 1 0 000-2z\" clipRule=\"evenodd\"/></svg>\n"
"                                                        )}\n"
"                                                        {col.fk && (\n"
"                                                            <svg className=\"w-3 h-3 text-blue-500 shrink-0\" fill=\"currentColor\" viewBox=\"0 0 20 20\"><path fillRule=\"evenodd\" d=\"M12.586 4.586a2 2 0 112.828 2.828l-3 3a2 2 0 01-2.828 0 1 1 0 00-1.414 1.414 4 4 0 005.656 0l3-3a4 4 0 00-5.656-5.656l-1.5 1.5a1 1 0 101.414 1.414l1.5-1.5zm-5 5a2 2 0 012.828 0 1 1 0 101.414-1.414 4 4 0 00-5.656 0l-3 3a4 4 0 105.656 5.656l1.5-1.5a1 1 0 10-1.414-1.414l-1.5 1.5a2 2 0 11-2.828-2.828l3-3z\" clipRule=\"evenodd\"/></svg>\n"
"                                                        )}\n"
"                                                        {!col.pk && !col.fk && (\n"
"                                                            <div className=\"w-3 h-3 shrink-0\" />\n"
"                                                        )}\n"
"                                                        <span className={`truncate ${col.pk ? 'text-yellow-400 font-semibold' : 'text-gray-300'}`}>{col.name}</span>\n"
"                                                    </div>\n"
"                                                    <span className=\"text-[10px] text-gray-500 shrink-0 ml-4\">{col.type.toLowerCase()}</span>\n"
"                                                </div>\n"
"                                            ))}\n"
"                                        </div>\n"
"                                    </div>\n"
"                                );\n"
"                            })}\n"
"                        </div>\n"
"                    </div>\n"
"                </div>\n"
"            );\n"
"        }\n"
"\n"
"        const root = ReactDOM.createRoot(document.getElementById('root'));\n"
"        root.render(<App />);\n"
"    </script>\n"
"</body>\n"
"</html>\n";
    return html;
}
