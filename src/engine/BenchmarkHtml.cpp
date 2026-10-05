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

#include <wx/intl.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "engine/Benchmark.h"

// The HTML report: one self-contained page without scripts or external
// resources, shown in the benchmark dialog and saved as a file. It has two
// views. The overview is for everybody, also for administrators who know
// neither Firebird nor databases: the measured values of every part of the
// system in plain words, or, compared with another run, the points of both
// runs, what got faster or slower and what could be the reason. A single
// run is not judged, because whether a value is fast or slow depends on the
// environment. The detailed report is for developers: one tile per part
// whose panel opens on a click (a check box, so that no script is needed),
// charts, every measured value and the explanations. Charts are inline SVG
// or CSS bars drawn to scale; all colors are theme tokens.

namespace fr
{

namespace
{

// The style sheet is split into several literals, because MSVC limits a
// single string literal to about 16 KB.
const char* const styleSheet = R"CSS(
:root {
  --bg: #ffffff; --ink: #2d3748; --muted: #718096; --line: #e2e8f0; --head: #edf2f7;
  --head-ink: #1a202c; --alt: #f7fafc; --link: #3182ce; --link-hover: #2b6cb0;
  --ok: #2f855a; --ok-bg: #e6f4ea; --warn: #b7791f; --warn-bg: #fdf3dc;
  --bad: #c53030; --bad-bg: #fde8e8; --info-bg: #ebf4ff;
  --c1: #3182ce; --c2: #dd6b20; --c3: #a0aec0;
  --body: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  --mono: SFMono-Regular, Consolas, "Liberation Mono", Menlo, Courier, monospace;
  color-scheme: light;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --bg: #1e1e1e; --ink: #e0e0e0; --muted: #a0aec0; --line: #3d3d3d; --head: #2c2c3e;
    --head-ink: #ffffff; --alt: #252535; --link: #4a90e2; --link-hover: #6ba4e8;
    --ok: #68d391; --ok-bg: #1f3a2a; --warn: #f6c35b; --warn-bg: #3d3216;
    --bad: #fc8181; --bad-bg: #3f2222; --info-bg: #1f2c40;
    --c1: #4a90e2; --c2: #ed8936; --c3: #718096;
    color-scheme: dark;
  }
}
:root[data-theme="dark"] {
  --bg: #1e1e1e; --ink: #e0e0e0; --muted: #a0aec0; --line: #3d3d3d; --head: #2c2c3e;
  --head-ink: #ffffff; --alt: #252535; --link: #4a90e2; --link-hover: #6ba4e8;
  --ok: #68d391; --ok-bg: #1f3a2a; --warn: #f6c35b; --warn-bg: #3d3216;
  --bad: #fc8181; --bad-bg: #3f2222; --info-bg: #1f2c40;
  --c1: #4a90e2; --c2: #ed8936; --c3: #718096;
  color-scheme: dark;
}

/* the look of the FlameRobin property pages */
* { box-sizing: border-box; }
html, body { margin: 0; background: var(--bg); color: var(--ink); font: 13px/1.5 var(--body); }
main { max-width: 1100px; padding: 12px; }
a, .link { color: var(--link); text-decoration: none; cursor: pointer; }
a:hover, .link:hover { color: var(--link-hover); text-decoration: underline; }
h1 { font-size: 18px; line-height: 1.3; margin: 0 0 2px; overflow-wrap: anywhere; }
h2 { font-size: 14px; margin: 18px 0 6px; }
h4 { font-size: 13px; margin: 14px 0 6px; }
.num, .n, .points-num { font-variant-numeric: tabular-nums; }
.brand, .meta, .quiet, small, .muted { color: var(--muted); }
.brand { font-size: 12px; }
.specs, .meta { margin: 0; overflow-wrap: anywhere; }
.notice { border: 1px solid var(--line); border-radius: 6px; padding: 6px 10px; margin-top: 8px; overflow-wrap: anywhere; }
.notice.bad { background: var(--bad-bg); }
.notice.warn { background: var(--warn-bg); }
.notice.info { background: var(--info-bg); }
code { font-family: var(--mono); font-size: 12px; overflow-wrap: anywhere; }
p { max-width: 100ch; }

/* page navigation like on the property pages: Overview | Detailed report */
.view-toggle { position: fixed; top: 0; left: 0; width: 1px; height: 1px; opacity: 0; pointer-events: none; }
.pages { margin: 10px 0 4px; }
.pages label { color: var(--link); cursor: pointer; }
.pages label:hover { text-decoration: underline; }
#v1:checked ~ .pages label[for="v1"], #v2:checked ~ .pages label[for="v2"] { color: var(--ink); font-weight: 600; cursor: default; text-decoration: none; }
#v1:checked ~ .details, #v2:checked ~ .overview { display: none; }

/* tables and boxes: a light header row, thin lines, rounded corners */
table { width: 100%; border-collapse: separate; border-spacing: 0; border: 1px solid var(--line); border-radius: 6px; margin: 6px 0; }
thead th:first-child { border-top-left-radius: 5px; } thead th:last-child { border-top-right-radius: 5px; }
tbody tr:last-child td:first-child { border-bottom-left-radius: 5px; } tbody tr:last-child td:last-child { border-bottom-right-radius: 5px; }
th { background: var(--head); color: var(--head-ink); font-weight: 600; text-align: left; padding: 6px 10px; border-bottom: 1px solid var(--line); white-space: nowrap; }
td { padding: 6px 10px; border-bottom: 1px solid var(--line); vertical-align: top; overflow-wrap: anywhere; }
tr:last-child td { border-bottom: none; }
tbody tr:nth-child(even) td { background: var(--alt); }
td.n, th.n { text-align: right; white-space: nowrap; }
td small { display: block; font-size: 12px; white-space: normal; }
tr.group td { background: var(--head) !important; color: var(--head-ink); font-weight: 600; }
.box { border: 1px solid var(--line); border-radius: 6px; margin: 10px 0; }
.box-head { background: var(--head); color: var(--head-ink); font-weight: 600; padding: 6px 10px; border-bottom: 1px solid var(--line); border-radius: 5px 5px 0 0; }
.box > :last-child { border-bottom-left-radius: 5px; border-bottom-right-radius: 5px; }
.box-body { padding: 10px; }
.box-body p { margin: 0 0 6px; } .box-body p:last-child { margin-bottom: 0; }
.box-foot { padding: 6px 10px; border-top: 1px solid var(--line); background: var(--alt); }
.box > table { border: none; border-radius: 0; margin: 0; }
.box > table thead th { border-radius: 0; }

/* faster and slower in a comparison: green, red, grey within the variation */
.ok-text { color: var(--ok); } .warn-text { color: var(--warn); } .bad-text { color: var(--bad); } .info-text { color: var(--muted); }
.dot { display: inline-block; width: 9px; height: 9px; border-radius: 50%; margin-right: 7px; background: var(--line); vertical-align: 0; }
.dot.ok { background: var(--ok); } .dot.bad { background: var(--bad); }
.chg { display: inline-block; font-size: 12px; font-weight: 600; padding: 0 6px; border-radius: 4px; white-space: nowrap; }
.chg.up2 { background: var(--ok); color: var(--bg); } .chg.up1 { background: var(--ok-bg); color: var(--ok); }
.chg.down2 { background: var(--bad); color: var(--bg); } .chg.down1 { background: var(--bad-bg); color: var(--bad); }
.chg.same { color: var(--muted); }

/* the points of a comparison */
.points-grid { display: grid; grid-template-columns: minmax(170px, auto) 1fr; gap: 10px 24px; align-items: center; }
.points-num { font-size: 30px; font-weight: 700; line-height: 1.1; }
.points-unit { color: var(--muted); margin-left: 4px; }
.rating { font-weight: 600; }

/* the parts of the system: one line each, a click shows the values */
.part { border-bottom: 1px solid var(--line); }
.part:last-child { border-bottom: none; }
.row { display: grid; grid-template-columns: minmax(150px, 1.2fr) minmax(70px, .5fr) minmax(70px, .5fr) minmax(0, 2fr) 14px; gap: 4px 12px; align-items: baseline; padding: 6px 10px; }
.row.plain { grid-template-columns: minmax(150px, 1fr) minmax(0, 3fr) 14px; }
.parts-head { background: var(--head); color: var(--head-ink); font-weight: 600; border-bottom: 1px solid var(--line); border-radius: 5px 5px 0 0; }
.part > summary { cursor: pointer; list-style: none; }
.part > summary::-webkit-details-marker { display: none; }
.part > summary:hover { background: var(--alt); }
.part-name { font-weight: 600; }
.part-text { color: var(--muted); }
.part-body { padding: 0 10px 10px 26px; }
.part-body table { margin-top: 0; }
.reasons { border: 1px solid var(--line); border-left: 3px solid var(--warn); border-radius: 6px; padding: 6px 10px; margin-top: 8px; }
.reasons h4 { margin: 0 0 4px; }
.reasons ul { margin: 0; padding-left: 18px; }
.reasons li { margin: 2px 0; }

)CSS"
R"CSS(/* triangles of the collapsible parts */
details > summary { list-style: none; cursor: pointer; }
details > summary::-webkit-details-marker { display: none; }
.chev { width: 7px; height: 7px; border-right: 2px solid var(--muted); border-bottom: 2px solid var(--muted); transform: rotate(-45deg); transition: transform .12s; justify-self: end; margin-top: 4px; }
details[open] > summary .chev { transform: rotate(45deg); }
.block > summary { display: flex; align-items: center; gap: 10px; padding: 6px 10px; background: var(--head); color: var(--head-ink); font-weight: 600; }
.block > summary .chev { margin-left: auto; }
.block > summary .hint { color: var(--muted); font-weight: 400; }
.block-body { padding: 8px 10px; }
.deeper { margin-top: 8px; }
.deeper > summary { display: inline-flex; align-items: center; gap: 8px; color: var(--link); }
summary:focus-visible { outline: 2px solid var(--link); outline-offset: -2px; }

/* system details */
.sys-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 10px; }
.sys-card { border: 1px solid var(--line); border-radius: 6px; padding: 8px 10px; min-width: 0; }
.sys-card h3 { margin: 0 0 6px; font-size: 13px; }
.facts { margin: 0; display: grid; grid-template-columns: max-content minmax(0, 1fr); gap: 2px 14px; }
.facts dt { color: var(--muted); }
.facts dd { margin: 0; overflow-wrap: anywhere; }
.sys-more { margin-top: 10px; }

/* detailed report: one tile per part, a click opens its panel */
.cats > .toggle { position: fixed; top: 0; left: 0; width: 1px; height: 1px; opacity: 0; pointer-events: none; }
.tiles { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 8px; grid-auto-flow: row dense; }
.tile { display: flex; flex-direction: column; gap: 4px; border: 1px solid var(--line); border-top: 3px solid var(--line); border-radius: 6px; padding: 8px 10px; cursor: pointer; min-width: 0; }
.tile:hover { background: var(--alt); }
.tile .name { font-weight: 600; }
.tile .about { color: var(--muted); }
.tile .key { display: flex; justify-content: space-between; gap: 8px; font-size: 12px; }
.tile .key b { font-variant-numeric: tabular-nums; white-space: nowrap; }
.tile .more-link { margin-top: auto; color: var(--link); }
.tile .more-link .shut { display: none; }
.panel { display: none; grid-column: 1 / -1; border: 1px solid var(--line); border-top: 3px solid var(--link); border-radius: 6px; padding: 8px 10px; min-width: 0; }
.panel-head { display: flex; align-items: baseline; gap: 12px; flex-wrap: wrap; }
.panel-head h3 { margin: 0; font-size: 15px; }
.panel-head .close { margin-left: auto; color: var(--link); cursor: pointer; }
.where { color: var(--muted); margin: 0 0 8px; overflow-wrap: anywhere; }
.explain dt { font-weight: 600; margin-top: 8px; }
.explain dd { margin: 2px 0 0; color: var(--muted); max-width: 90ch; }
.explain dd b { color: var(--ink); font-weight: 600; }

/* tooltips: a small question mark, the text on hover or focus */
.tip { position: relative; display: inline-grid; place-items: center; width: 14px; height: 14px; margin-left: 4px; border-radius: 50%; border: 1px solid var(--line); color: var(--muted); font: 600 10px/1 var(--body); cursor: help; vertical-align: 1px; }
.tip-box { display: none; position: absolute; z-index: 20; top: calc(100% + 5px); left: -8px; width: max-content; max-width: 280px; padding: 6px 9px; border-radius: 4px; border: 1px solid var(--line); background: var(--bg); color: var(--ink); box-shadow: 0 3px 10px rgba(0,0,0,.18); font: 400 12px/1.4 var(--body); text-align: left; white-space: normal; text-transform: none; }
td.n .tip-box, th.n .tip-box { left: auto; right: -8px; }
.tip:hover .tip-box, .tip:focus .tip-box { display: block; }

/* charts */
.charts { display: grid; grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); gap: 10px; }
.chart { border: 1px solid var(--line); border-radius: 6px; padding: 8px 10px; min-width: 0; }
.chart h5 { font-size: 13px; margin: 0; }
.chart .chart-sub { color: var(--muted); font-size: 12px; margin: 2px 0 8px; overflow-wrap: anywhere; }
.chart svg { width: 100%; height: auto; display: block; }
.chart svg text { font-family: var(--mono); }
.axis { fill: var(--muted); font-size: 10px; }
.gridline { stroke: var(--line); stroke-width: 1; }
.c1 { fill: var(--c1); background: var(--c1); }
.c2 { fill: var(--c2); background: var(--c2); }
.c3 { fill: var(--c3); background: var(--c3); }
.ideal { stroke: var(--muted); stroke-width: 1.5; stroke-dasharray: 4 3; fill: none; }
.ideal-dot { fill: var(--bg); stroke: var(--muted); stroke-width: 1.5; }
.best { stroke: var(--ok); stroke-width: 1; stroke-dasharray: 3 3; }
.vlabel { fill: var(--ink); font-size: 10px; paint-order: stroke; stroke: var(--bg); stroke-width: 3px; }
.hbars { display: grid; gap: 6px; }
.hb { display: grid; grid-template-columns: minmax(90px, 38%) 1fr auto; gap: 10px; align-items: center; font-size: 12px; }
.hb-label { color: var(--muted); overflow-wrap: anywhere; }
.hb-track { height: 9px; background: var(--alt); border-radius: 3px; overflow: hidden; }
.hb-fill { display: block; height: 100%; border-radius: 3px; min-width: 2px; }
.hb-val { font-family: var(--mono); font-variant-numeric: tabular-nums; white-space: nowrap; }
.stack { display: flex; height: 18px; border-radius: 4px; overflow: hidden; background: var(--alt); }
.stack span { display: block; height: 100%; min-width: 2px; }
.legend { display: flex; flex-wrap: wrap; gap: 4px 14px; font-size: 12px; color: var(--muted); margin-top: 8px; }
.legend i { display: inline-block; width: 10px; height: 10px; border-radius: 2px; margin-right: 5px; vertical-align: -1px; }
.legend i.dash { height: 0; border-top: 2px dashed var(--muted); border-radius: 0; vertical-align: 3px; }

/* two runs side by side */
.runs { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
.runs .box { margin: 0; }

footer { margin-top: 16px; color: var(--muted); font-size: 12px; max-width: 100ch; }

@media (max-width: 860px) {
  .tiles { grid-template-columns: repeat(2, minmax(0, 1fr)); }
}
@media (max-width: 640px) {
  .points-grid, .runs { grid-template-columns: 1fr; }
  .tiles { grid-template-columns: 1fr; }
  /* a part: name and points in one line, the text below */
  .row { grid-template-columns: minmax(0, 1fr) auto auto 14px; }
  .row.plain { grid-template-columns: minmax(0, 1fr) 14px; }
  .row .part-text { grid-column: 1 / -1; grid-row: 2; }
  .parts-head .part-text { display: none; }
  .part-body { padding-left: 10px; }
  .facts { grid-template-columns: minmax(0, 2fr) minmax(0, 3fr); gap: 3px 10px; }
  .block > summary { flex-wrap: wrap; }
  .block > summary .hint { flex-basis: 100%; order: 2; }
  /* tables become cards: one row per block, every cell with its label */
  table.cards thead { display: none; }
  table.cards, table.cards tbody, table.cards tr, table.cards td { display: block; width: 100%; }
  table.cards { border: 1px solid var(--line); }
  table.cards tr { border-bottom: 1px solid var(--line); padding: 6px 10px; }
  table.cards tr:last-child { border-bottom: none; }
  table.cards td { border: 0; padding: 1px 0; text-align: left; white-space: normal; background: none !important; }
  table.cards tr.group { background: var(--head); }
  table.cards td:first-child { font-weight: 600; }
  table.cards td[data-label]:not(:first-child)::before { content: attr(data-label) ": "; color: var(--muted); }
  table.cards td:empty { display: none; }
  .tip-box { max-width: 70vw; }
}
@media print { .panel { display: block !important; } details > div { display: block !important; } .view-toggle, .pages { display: none; } }
)CSS";

wxString esc(const wxString& text)
{
    wxString s;
    s.reserve(text.length());
    for (wxUniChar c : text)
    {
        switch (c.GetValue())
        {
            case '&': s += "&amp;"; break;
            case '<': s += "&lt;"; break;
            case '>': s += "&gt;"; break;
            case '"': s += "&quot;"; break;
            case '\'': s += "&#39;"; break;
            default: s += c;
        }
    }
    return s;
}

// numbers in SVG attributes and CSS must not depend on the locale
wxString num(double value, int precision = 1)
{
    return wxString::FromCDouble(value, precision);
}

// short labels for axis ticks: 0.3, 5, 15k
wxString compact(double value)
{
    if (value >= 1000.0)
    {
        double k = value / 1000.0;
        return wxString::FromCDouble(k, k >= 10.0 || k == std::floor(k) ? 0 : 1) + "k";
    }
    if (value == std::floor(value))
        return wxString::FromCDouble(value, 0);
    return wxString::FromCDouble(value, value < 1.0 ? 2 : 1);
}

// an axis maximum of 1, 2 or 5 times a power of ten
double niceMaximum(double value)
{
    if (value <= 0.0)
        return 1.0;
    double magnitude = std::pow(10.0, std::floor(std::log10(value)));
    for (double step : { 1.0, 2.0, 2.5, 5.0, 10.0 })
    {
        if (step * magnitude >= value)
            return step * magnitude;
    }
    return 10.0 * magnitude;
}

// a question mark that shows text on hover and focus
wxString tip(const wxString& text)
{
    if (text.empty())
        return wxEmptyString;
    return "<span class=\"tip\" tabindex=\"0\" aria-label=\"" + esc(text) + "\">?"
        "<span class=\"tip-box\" role=\"tooltip\">" + esc(text) + "</span></span>";
}

// a value with the explanation of its unit
wxString value(const wxString& text)
{
    return esc(text) + tip(getBenchmarkUnitHint(text));
}

// an opening <td> that names its column for the card layout on small
// screens; an empty cell has no label, so that it can be hidden
wxString cellLabel(const wxString& label, const wxString& cls = wxEmptyString,
    const wxString& content = "-")
{
    return "<td" + (cls.empty() ? wxString() : " class=\"" + cls + "\"")
        + (content.empty() ? wxString() : " data-label=\"" + esc(label) + "\"") + ">";
}

// a column of a table; numbers are right aligned
struct Column
{
    wxString title;
    bool numeric;
};

// the start of a table that becomes cards on small screens
wxString tableStart(std::initializer_list<Column> columns)
{
    wxString html = "<table class=\"cards\"><thead><tr>";
    for (const Column& c : columns)
        html += wxString(c.numeric ? "<th class=\"n\">" : "<th>") + esc(c.title) + "</th>";
    return html + "</tr></thead><tbody>";
}

wxString facts(const std::vector<std::pair<wxString, wxString>>& rows)
{
    wxString html = "<dl class=\"facts\">";
    for (const auto& row : rows)
        html += "<dt>" + esc(row.first) + "</dt><dd>" + esc(row.second) + "</dd>";
    return html + "</dl>";
}

// ---------------------------------------------------------------- charts

struct Bar
{
    wxString label;
    double value;
    wxString text;
    int color;      // 1..3
};

wxString chartPanel(const wxString& title, const wxString& subtitle,
    const wxString& body)
{
    return "<div class=\"chart\"><h5>" + esc(title) + "</h5><p class=\"chart-sub\">"
        + esc(subtitle) + "</p>" + body + "</div>";
}

wxString legend(const std::vector<std::pair<int, wxString>>& entries)
{
    wxString html = "<div class=\"legend\">";
    for (const auto& e : entries)
    {
        html += e.first == 0 ? wxString("<span><i class=\"dash\"></i>")
            : wxString::Format("<span><i class=\"c%d\"></i>", e.first);
        html += esc(e.second) + "</span>";
    }
    return html + "</div>";
}

// horizontal bars as HTML, so that labels wrap at any width
wxString horizontalBars(const std::vector<Bar>& bars)
{
    double maxValue = 0.0;
    for (const auto& b : bars)
        maxValue = std::max(maxValue, b.value);
    if (maxValue <= 0.0)
        return wxEmptyString;
    wxString html = "<div class=\"hbars\">";
    for (const auto& b : bars)
    {
        html += "<div class=\"hb\"><span class=\"hb-label\">" + esc(b.label)
            + "</span><span class=\"hb-track\"><span class=\"hb-fill "
            + wxString::Format("c%d", b.color) + "\" style=\"width:"
            + num(100.0 * b.value / maxValue) + "%\"></span></span>"
            + "<span class=\"hb-val\">" + esc(b.text) + "</span></div>";
    }
    return html + "</div>";
}

// columns with a value axis; groups of up to three series
wxString columnChart(const std::vector<wxString>& groups,
    const std::vector<std::vector<Bar>>& series, double axisMaximum,
    const wxString& axisTitle, const std::vector<double>& ideal,
    double bestLine)
{
    const double width = 320.0, height = 196.0;
    const double left = 40.0, right = 312.0, top = 14.0, bottom = 158.0;
    auto y = [&](double v)
    {
        return bottom - std::clamp(v / axisMaximum, 0.0, 1.0) * (bottom - top);
    };
    wxString svg = "<svg viewBox=\"0 0 " + num(width, 0) + " " + num(height, 0)
        + "\" role=\"img\" aria-label=\"" + esc(axisTitle) + "\">";
    for (int i = 0; i <= 4; ++i)
    {
        double v = axisMaximum * i / 4.0;
        svg += "<line class=\"gridline\" x1=\"" + num(left) + "\" x2=\"" + num(right)
            + "\" y1=\"" + num(y(v)) + "\" y2=\"" + num(y(v)) + "\"/>";
        svg += "<text class=\"axis\" x=\"" + num(left - 6) + "\" y=\"" + num(y(v) + 3.5)
            + "\" text-anchor=\"end\">" + esc(compact(v)) + "</text>";
    }
    const double slot = (right - left) / groups.size();
    // value labels are drawn last, so that the lines never cover them
    wxString labels;
    for (size_t g = 0; g < groups.size(); ++g)
    {
        const std::vector<Bar>& bars = series[g];
        const double barWidth = std::min(26.0, slot * 0.7 / std::max<size_t>(1, bars.size()));
        double x = left + slot * g + (slot - barWidth * bars.size()) / 2.0;
        for (const auto& b : bars)
        {
            svg += "<rect class=\"" + wxString::Format("c%d", b.color) + "\" x=\""
                + num(x) + "\" y=\"" + num(y(b.value)) + "\" width=\"" + num(barWidth)
                + "\" height=\"" + num(bottom - y(b.value)) + "\" rx=\"2\"/>";
            if (bars.size() == 1)
            {
                labels += "<text class=\"vlabel\" x=\"" + num(x + barWidth / 2) + "\" y=\""
                    + num(y(b.value) - 5) + "\" text-anchor=\"middle\">" + esc(b.text)
                    + "</text>";
            }
            x += barWidth;
        }
        svg += "<text class=\"axis\" x=\"" + num(left + slot * (g + 0.5)) + "\" y=\""
            + num(bottom + 15) + "\" text-anchor=\"middle\">" + esc(groups[g]) + "</text>";
    }
    if (!ideal.empty())
    {
        wxString points;
        for (size_t g = 0; g < ideal.size(); ++g)
            points += num(left + slot * (g + 0.5)) + "," + num(y(ideal[g])) + " ";
        svg += "<polyline class=\"ideal\" points=\"" + points.Trim() + "\"/>";
        for (size_t g = 0; g < ideal.size(); ++g)
        {
            svg += "<circle class=\"ideal-dot\" cx=\"" + num(left + slot * (g + 0.5))
                + "\" cy=\"" + num(y(ideal[g])) + "\" r=\"2.5\"/>";
        }
    }
    if (bestLine > 0.0)
    {
        svg += "<line class=\"best\" x1=\"" + num(left) + "\" x2=\"" + num(right)
            + "\" y1=\"" + num(y(bestLine)) + "\" y2=\"" + num(y(bestLine)) + "\"/>";
    }
    svg += labels;
    svg += "<text class=\"axis\" x=\"" + num((left + right) / 2) + "\" y=\""
        + num(height - 6) + "\" text-anchor=\"middle\">" + esc(axisTitle) + "</text>";
    return svg + "</svg>";
}

// the charts that belong to one section of the report
wxString sectionCharts(BenchmarkCategory section, const BenchmarkMetrics& m)
{
    const BenchmarkSettings& s = m.settings;
    const BenchmarkMachines machines = getBenchmarkMachines(m);
    auto ms = [](double v) { return formatBenchmarkMilliseconds(v); };
    auto addBar = [&ms](std::vector<Bar>& bars, const wxString& label,
        const std::optional<double>& v, int color)
    {
        if (v)
            bars.push_back({ label, *v, ms(*v), color });
    };
    wxString charts;
    switch (section)
    {
        case BenchmarkCategory::ServerCpu:
            if (m.serverBurstIdleMs && m.serverBurstHotMs)
            {
                charts += chartPanel(_("Power management"),
                    _("the same short CPU burst inside the server; with power "
                      "saving the burst after an idle pause is slower"),
                    horizontalBars({ { _("After an idle pause"), *m.serverBurstIdleMs,
                        ms(*m.serverBurstIdleMs), 2 }, { _("Back to back"),
                        *m.serverBurstHotMs, ms(*m.serverBurstHotMs), 1 } }));
            }
            break;
        case BenchmarkCategory::ServerStorage:
        {
            // rows per second with the normal and with the minimal cache
            std::vector<Bar> bars;
            auto rate = [&bars](const wxString& label, double rows,
                const std::optional<double>& ms, int color)
            {
                if (ms && *ms > 0.0 && rows > 0.0)
                {
                    const double v = rows * 1000.0 / *ms;
                    bars.push_back({ label, v, formatBenchmarkCount(v), color });
                }
            };
            rate(_("Insert"), s.diskRows, m.insertMs, 1);
            rate(_("Insert, minimal cache"), s.diskRows, m.smallCacheInsertMs, 2);
            rate(_("Read by key, minimal cache"), m.smallCacheLookups, m.smallCacheLookupMs, 2);
            rate(_("Update"), s.diskRows, m.updateMs, 1);
            rate(_("Update, minimal cache"), m.smallCacheUpdates, m.smallCacheUpdateMs, 2);
            rate(_("Delete"), s.diskRows, m.deleteMs, 1);
            rate(_("Delete, minimal cache"), m.smallCacheDeletes, m.smallCacheDeleteMs, 2);
            rate(wxString::Format(_("Insert, %d connections, minimal cache"),
                s.parallelConnections), double(m.parallelDriveRows), m.parallelDriveMs, 3);
            if (!bars.empty())
            {
                charts += chartPanel(_("Rows per second"),
                    _("normal page cache and the drive test with a minimal page cache"),
                    horizontalBars(bars) + legend({ { 1, _("normal cache") },
                        { 2, _("minimal cache") }, { 3, _("parallel, minimal cache") } }));
            }
            if (m.pageSizes.size() >= 2)
            {
                auto best = [&m](double BenchmarkPageSizeResult::*field)
                {
                    double b = 0.0;
                    for (const auto& p : m.pageSizes)
                    {
                        if (p.*field > 0.0 && (b == 0.0 || p.*field < b))
                            b = p.*field;
                    }
                    return b;
                };
                const double bestInsert = best(&BenchmarkPageSizeResult::insertMs);
                const double bestScan = best(&BenchmarkPageSizeResult::warmScanMs);
                const double bestDrive = best(&BenchmarkPageSizeResult::smallCacheInsertMs);
                std::vector<wxString> groups;
                std::vector<std::vector<Bar>> series;
                double maximum = 1.0;
                for (const auto& p : m.pageSizes)
                {
                    groups.push_back(wxString::Format("%dK", p.pageSize / 1024));
                    std::vector<Bar> group;
                    if (bestInsert > 0.0)
                        group.push_back({ wxEmptyString, p.insertMs / bestInsert, "", 1 });
                    if (bestScan > 0.0)
                        group.push_back({ wxEmptyString, p.warmScanMs / bestScan, "", 2 });
                    if (bestDrive > 0.0)
                    {
                        group.push_back({ wxEmptyString, p.smallCacheInsertMs / bestDrive,
                            "", 3 });
                    }
                    for (const auto& b : group)
                        maximum = std::max(maximum, b.value);
                    series.push_back(group);
                }
                charts += chartPanel(_("Page sizes"),
                    _("time relative to the best page size (1 = best, lower is better)"),
                    columnChart(groups, series, niceMaximum(maximum * 1.05), _("page size"),
                        {}, 1.0)
                    + legend({ { 1, _("insert") }, { 2, _("cached scan") },
                        { 3, _("insert, minimal cache") } }));
            }
            break;
        }
        case BenchmarkCategory::ServerCache:
        {
            std::vector<Bar> bars;
            addBar(bars, _("First scan after reconnect"), m.coldScanMs, 2);
            addBar(bars, _("Repeated scan"), m.warmScanMs, 1);
            addBar(bars, _("Scan, minimal cache"), m.smallCacheScanMs, 3);
            addBar(bars, _("Read inside the server"), m.serverReadMs, 1);
            addBar(bars, _("Sort inside the server"), m.serverSortMs, 2);
            if (!bars.empty())
            {
                charts += chartPanel(_("Cache and sort"), wxString::Format(_("%s scanned"),
                    wxString::Format(_("%.1f MB"), m.scanBytes / (1024.0 * 1024.0))),
                    horizontalBars(bars));
            }
            break;
        }
        case BenchmarkCategory::Network:
        {
            if (m.latencyMedianMs)
            {
                std::vector<Bar> bars;
                addBar(bars, _("Median"), m.latencyMedianMs, 1);
                addBar(bars, _("95th percentile"), m.latencyP95Ms, 1);
                addBar(bars, _("Maximum"), m.latencyMaxMs, 3);
                addBar(bars, _("After idle pauses"), m.latencySparseMs, 2);
                addBar(bars, _("8 KB answer"), m.largeRoundTripMs, 3);
                charts += chartPanel(_("Round trips"),
                    wxString::Format(_("%d queries"), s.latencyQueries), horizontalBars(bars));
            }
            if (m.streamMs && m.serverReadMs && *m.streamMs > 0.0)
            {
                double client = m.streamClientMs.value_or(0.0);
                double server = std::min(*m.serverReadMs, *m.streamMs);
                double transfer = std::max(0.0, *m.streamMs - server - client);
                double total = server + transfer + client;
                if (total > 0.0)
                {
                    auto part = [&](int color, double v)
                    {
                        return wxString::Format("<span class=\"c%d\" style=\"width:", color)
                            + num(100.0 * v / total) + "%\"></span>";
                    };
                    charts += chartPanel(_("Where the time goes"),
                        wxString::Format(_("%lld rows streamed in %s"),
                            (long long)m.streamRows, ms(*m.streamMs)),
                        "<div class=\"stack\">" + part(1, server) + part(2, transfer)
                            + part(3, client) + "</div>"
                        + legend({ { 1, _("server") + " " + ms(server) },
                            { 2, _("transfer") + " " + ms(transfer) },
                            { 3, _("FlameRobin") + " " + ms(client) } }));
                }
            }
            break;
        }
        case BenchmarkCategory::MultiUser:
            if (m.oltp.size() >= 2 && m.oltp.front().seconds > 0.0)
            {
                std::vector<wxString> groups;
                std::vector<std::vector<Bar>> series;
                std::vector<double> ideal;
                const BenchmarkOltpResult& first = m.oltp.front();
                const double base = first.transactions / first.seconds;
                double maximum = 0.0;
                for (const auto& o : m.oltp)
                {
                    double tps = o.seconds > 0.0 ? o.transactions / o.seconds : 0.0;
                    groups.push_back(wxString::Format("%d", o.clients));
                    series.push_back({ { wxEmptyString, tps, compact(std::round(tps)), 1 } });
                    ideal.push_back(base * o.clients / first.clients);
                    maximum = std::max({ maximum, tps, ideal.back() });
                }
                charts += chartPanel(_("Throughput by parallel clients"),
                    _("transactions per second"),
                    columnChart(groups, series, niceMaximum(maximum), _("parallel clients"),
                        ideal, 0.0)
                    + legend({ { 1, _("measured tx/s") }, { 0, _("linear scaling") } }));
            }
            break;
        case BenchmarkCategory::LocalMachine:
            if (m.clientBurstIdleMs && m.clientBurstHotMs)
            {
                charts += chartPanel(_("Power management"),
                    wxString::Format(_("the same short CPU burst on %s"), machines.local),
                    horizontalBars({ { _("After an idle pause"), *m.clientBurstIdleMs,
                        ms(*m.clientBurstIdleMs), 2 }, { _("Back to back"),
                        *m.clientBurstHotMs, ms(*m.clientBurstHotMs), 1 } }));
            }
            break;
        case BenchmarkCategory::Application:
            if (m.rowByRowMs && m.batchMs)
            {
                charts += chartPanel(_("Row by row vs. batch"),
                    wxString::Format(_("%d inserts"), s.rowByRowRows),
                    horizontalBars({ { _("One statement per row"), *m.rowByRowMs,
                        ms(*m.rowByRowMs), 2 }, { _("One server call"), *m.batchMs,
                        ms(*m.batchMs), 1 } }));
            }
            if (m.versionReadBeforeMs && m.versionReadWithOldTxMs)
            {
                std::vector<Bar> bars;
                addBar(bars, _("Before the updates"), m.versionReadBeforeMs, 1);
                addBar(bars, _("Old transaction still open"), m.versionReadWithOldTxMs, 3);
                addBar(bars, _("First read after it ended"), m.versionReadAfterCommitMs, 2);
                charts += chartPanel(_("Record versions"),
                    wxString::Format(_("read of rows updated %d times"), s.versionRounds),
                    horizontalBars(bars));
            }
            break;
        default:
            break;
    }
    return charts.empty() ? charts : "<div class=\"charts\">" + charts + "</div>";
}

// ---------------------------------------------------------------- parts

struct Section
{
    BenchmarkCategory category;
    int index;                      // for the ids of the page
    const BenchmarkPart* part;      // nullptr: no key values
    std::vector<const BenchmarkResult*> results;

    bool empty() const
    {
        return !part && results.empty();
    }
};

std::vector<Section> collectSections(const BenchmarkReport& report)
{
    std::vector<Section> sections;
    int index = 0;
    for (BenchmarkCategory category : getBenchmarkReportSections())
    {
        Section s{ category, index++, nullptr, {} };
        for (const auto& p : report.parts)
        {
            if (p.category == category)
                s.part = &p;
        }
        for (const auto& r : report.results)
        {
            if (getBenchmarkReportSection(r.category) == category)
                s.results.push_back(&r);
        }
        sections.push_back(s);
    }
    return sections;
}

wxString machineOf(const Section& s, const BenchmarkReport& report)
{
    if (s.part)
        return s.part->machine;
    const BenchmarkMachines machines = getBenchmarkMachines(report.metrics);
    switch (s.category)
    {
        case BenchmarkCategory::Network:
            return machines.network;
        case BenchmarkCategory::LocalMachine:
            return machines.local;
        case BenchmarkCategory::Application:
        case BenchmarkCategory::MultiUser:
            return machines.both;
        default:
            return machines.server;
    }
}

wxString valuesTable(const std::vector<const BenchmarkResult*>& results)
{
    if (results.empty())
        return wxEmptyString;
    wxString html = tableStart({ { _("Test"), false }, { _("Result"), true },
        { _("Details"), false }, { _("Machine"), false } });
    for (const BenchmarkResult* r : results)
    {
        html += "<tr><td>" + esc(r->test) + tip(r->hint) + "</td>" + cellLabel(_("Result"), "n")
            + value(r->value) + "</td>" + cellLabel(_("Details")) + esc(r->details) + "</td>"
            + cellLabel(_("Machine")) + esc(r->machine) + "</td></tr>";
    }
    return html + "</tbody></table>";
}

// what each test does; results that share an explanation are listed once
wxString explanations(const std::vector<const BenchmarkResult*>& results)
{
    std::vector<std::pair<wxString, const BenchmarkResult*>> items;
    for (const BenchmarkResult* r : results)
    {
        if (r->explanation.empty())
            continue;
        auto it = std::find_if(items.begin(), items.end(),
            [r](const auto& item) { return item.second->explanation == r->explanation; });
        if (it == items.end())
            items.push_back({ r->test, r });
        else
            it->first += ", " + r->test;
    }
    if (items.empty())
        return wxEmptyString;
    wxString html = "<dl class=\"explain\">";
    for (const auto& item : items)
    {
        html += "<dt>" + esc(item.first) + "</dt><dd>"
            + (item.second->hint.empty() ? wxString() : "<b>" + esc(item.second->hint) + "</b> ")
            + esc(item.second->explanation) + "</dd>";
    }
    return html + "</dl>";
}

wxString settingsTable(const std::vector<BenchmarkSetting>& settings)
{
    if (settings.empty())
        return wxEmptyString;
    wxString html = tableStart({ { _("Setting"), false }, { _("Where"), false },
        { _("Value"), false }, { _("What it does"), false } });
    for (const auto& s : settings)
    {
        html += "<tr><td><code>" + esc(s.name) + "</code></td>" + cellLabel(_("Where"))
            + esc(s.scope) + "</td>" + cellLabel(_("Value")) + "<code>" + esc(s.value)
            + "</code></td>" + cellLabel(_("What it does")) + esc(s.meaning) + "</td></tr>";
    }
    return html + "</tbody></table>";
}

// a value with the work behind it: "0.44 ms" and "200 commits in 88 ms"
wxString valueCell(const wxString& label, const wxString& text, const wxString& work)
{
    return cellLabel(label, "n") + "<b>" + value(text) + "</b>"
        + (work.empty() ? wxString() : "<small>" + esc(work) + "</small>") + "</td>";
}

// the key values of a part with the work behind them, then its other
// measurements, and for the settings the settings of Firebird
wxString partValues(const Section& s, const BenchmarkReport& report)
{
    const wxString result = _("Result");
    wxString rows;
    if (s.part)
    {
        for (const auto& v : s.part->values)
        {
            wxString hint;
            for (const BenchmarkResult* r : s.results)
            {
                if (r->valueKey == v.key)
                    hint = r->hint;
            }
            rows += "<tr><td>" + esc(v.title) + tip(hint) + "</td>"
                + valueCell(result, v.valueText, formatBenchmarkWork(v.amountText,
                    v.durationMs)) + "</tr>";
        }
    }
    wxString further;
    for (const BenchmarkResult* r : s.results)
    {
        if (!r->valueKey.empty() || (r->durationMs <= 0.0 && r->rate.empty()
            && s.category != BenchmarkCategory::Application))
        {
            continue;
        }
        further += "<tr><td>" + esc(r->test) + tip(r->hint) + "</td>"
            + valueCell(result, r->value, formatBenchmarkWork(r->amount, r->durationMs))
            + "</tr>";
    }
    if (!further.empty())
    {
        rows += (rows.empty() ? wxString() : "<tr class=\"group\"><td colspan=\"2\">"
            + esc(_("Further measurements")) + "</td></tr>") + further;
    }

    wxString html;
    if (!rows.empty())
    {
        html += tableStart({ { _("Test"), false }, { result, true } }) + rows
            + "</tbody></table>";
    }
    if (s.category == BenchmarkCategory::Environment)
        html += settingsTable(report.settings);
    if (html.empty())
        html = "<p class=\"quiet\">" + esc(_("Not tested in this run.")) + "</p>";
    return html;
}

wxString tile(const Section& s, const BenchmarkReport& report)
{
    wxString body = "<span class=\"name\">" + esc(getBenchmarkReportSectionName(s.category))
        + "</span><span class=\"about\">"
        + esc(getBenchmarkSectionDescription(s.category, report.metrics)) + "</span>";
    if (s.part)
    {
        for (const auto& v : s.part->values)
        {
            body += "<span class=\"key\"><span>" + esc(v.title) + "</span><b>"
                + esc(v.valueText) + "</b></span>";
        }
    }
    body += "<span class=\"more-link\"><span class=\"open\">" + esc(_("Details"))
        + "</span><span class=\"shut\">" + esc(_("Hide details")) + "</span></span>";
    return wxString::Format("<label class=\"tile t%d\" for=\"c%d\">", s.index, s.index)
        + body + "</label>";
}

wxString panel(const Section& s, const BenchmarkReport& report)
{
    wxString html = wxString::Format("<div class=\"panel p%d\">", s.index)
        + "<div class=\"panel-head\"><h3>" + esc(getBenchmarkReportSectionName(s.category))
        + "</h3>" + wxString::Format("<label class=\"close\" for=\"c%d\">", s.index)
        + esc(_("Close")) + " &#x2715;</label></div><p class=\"where\">"
        + esc(wxString::Format(_("Measured on %s"), machineOf(s, report))) + "</p>"
        + partValues(s, report);

    // everything else on request
    wxString deeper;
    const wxString charts = sectionCharts(s.category, report.metrics);
    if (!charts.empty())
        deeper += "<h4>" + esc(_("Charts")) + "</h4>" + charts;
    if (!s.results.empty())
    {
        deeper += "<h4>" + esc(wxString::Format(_("All values (%d)"), int(s.results.size())))
            + "</h4>" + valuesTable(s.results);
        deeper += "<h4>" + esc(_("What is tested")) + "</h4>" + explanations(s.results);
    }
    if (!deeper.empty())
    {
        html += "<details class=\"deeper\"><summary>" + esc(_("More details"))
            + "<span class=\"chev\"></span></summary>" + deeper + "</details>";
    }
    return html + "</div>";
}

wxString systemBlock(const BenchmarkReport& report)
{
    auto grid = [](const std::vector<BenchmarkInfoGroup>& groups)
    {
        wxString html = "<div class=\"sys-grid\">";
        for (const auto& group : groups)
        {
            html += "<div class=\"sys-card\"><h3>" + esc(group.title) + "</h3>"
                + facts(group.rows) + "</div>";
        }
        return html + "</div>";
    };
    const wxString cards = grid(getBenchmarkSystemOverview(report));
    const wxString details = grid(getBenchmarkSystemDetails(report));
    return "<details class=\"block\" id=\"system\"><summary>" + esc(_("System details"))
        + " <span class=\"hint\">" + esc(_("hardware, software, network and the test database"))
        + "</span><span class=\"chev\"></span></summary><div class=\"block-body\">" + cards
        + "<details class=\"deeper sys-more\"><summary>" + esc(_("All system information"))
        + "<span class=\"chev\"></span></summary>" + details + "</details></div></details>\n";
}

wxString statisticsTable(const std::vector<Section>& sections)
{
    wxString html = tableStart({ { _("Test"), false }, { _("Result"), true },
        { _("Amount"), true }, { _("Time"), true }, { _("Average"), true },
        { _("Rate"), true } });
    for (const Section& s : sections)
    {
        wxString rows;
        for (const BenchmarkResult* r : s.results)
        {
            // configuration values are in the system details
            if (r->amount.empty() && r->duration.empty() && r->rate.empty()
                && s.category == BenchmarkCategory::Environment)
            {
                continue;
            }
            rows += "<tr><td>" + esc(r->test) + tip(r->hint)
                + (r->details.empty() ? wxString() : "<small>" + esc(r->details) + "</small>")
                + "</td>" + cellLabel(_("Result"), "n") + value(r->value) + "</td>"
                + cellLabel(_("Amount"), "n", r->amount) + esc(r->amount) + "</td>"
                + cellLabel(_("Time"), "n", r->duration) + esc(r->duration) + "</td>"
                + cellLabel(_("Average"), "n", r->average) + esc(r->average) + "</td>"
                + cellLabel(_("Rate"), "n", r->rate) + esc(r->rate) + "</td></tr>";
        }
        if (!rows.empty())
        {
            html += "<tr class=\"group\"><td colspan=\"6\">"
                + esc(getBenchmarkReportSectionName(s.category)) + "</td></tr>" + rows;
        }
    }
    return html + "</tbody></table>";
}

wxString glossary(const BenchmarkReport& report)
{
    wxString html = "<h4>" + esc(_("Terms and abbreviations")) + "</h4><dl class=\"explain\">";
    for (const auto& entry : getBenchmarkGlossary())
        html += "<dt>" + esc(entry.term) + "</dt><dd>" + esc(entry.explanation) + "</dd>";
    html += "</dl><h4>" + esc(_("The tests")) + "</h4>";
    std::vector<const BenchmarkResult*> tests;
    for (const auto& r : report.results)
    {
        // the configuration values and the selected database are no tests
        if (r.category != BenchmarkCategory::Environment)
            tests.push_back(&r);
    }
    return html + explanations(tests);
}

wxString runDetails(const BenchmarkReport& report)
{
    const BenchmarkMetrics& m = report.metrics;
    std::vector<std::pair<wxString, wxString>> rows = {
        { _("Target"), report.target },
        { _("Runs on"), getBenchmarkRunLocationName(m.runLocation) + " ("
            + getBenchmarkConnectionKindName(m.connectionKind) + ")" },
        { _("Intensity"), report.intensityName },
        { _("Started"), report.startedAt },
        { _("Duration"), wxString::Format(_("%.0f s"), report.durationSeconds) },
        { _("Temporary database"), report.benchmarkDatabasePath + " ("
            + (!report.benchmarkDatabaseCreated ? _("not created")
                : report.benchmarkDatabaseDropped ? _("dropped") : _("not dropped")) + ")" } };
    for (const auto& path : report.undroppedDatabases)
        rows.push_back({ _("Not dropped"), path });
    for (const auto& skipped : m.skippedTests)
        rows.push_back({ _("Not measured"), skipped.first + ": " + skipped.second });
    return facts(rows);
}

// ---------------------------------------------------------------- one run

wxString introBox()
{
    return "<section class=\"box\" id=\"intro\"><div class=\"box-body\"><p>"
        + esc(_("These are the measured values of this run. Whether a value is fast "
            "or slow depends on the environment, so a single run is not judged."))
        + "</p><p>" + esc(_("Compare the report with another one, for example from "
            "before a change or from another server: select it under \"Compare with "
            "an earlier report\" before the next run, or use \"Compare two "
            "reports...\". The comparison shows what got faster or slower and what "
            "could be the reason.")) + "</p></div></section>\n";
}

// the key values of every part at a glance, with the work behind them
wxString keyValuesBox(const BenchmarkReport& report, const std::vector<Section>& sections)
{
    wxString rows;
    for (const Section& s : sections)
    {
        if (!s.part)
            continue;
        rows += "<tr class=\"group\"><td colspan=\"3\">"
            + esc(getBenchmarkReportSectionName(s.category))
            + tip(getBenchmarkSectionDescription(s.category, report.metrics)) + "</td></tr>";
        for (const auto& v : s.part->values)
        {
            wxString hint;
            for (const BenchmarkResult* r : s.results)
            {
                if (r->valueKey == v.key)
                    hint = r->hint;
            }
            const wxString work = formatBenchmarkWork(v.amountText, v.durationMs);
            rows += "<tr><td>" + esc(v.title) + tip(hint) + "</td>" + cellLabel(_("Result"), "n")
                + "<b>" + value(v.valueText) + "</b></td>" + cellLabel(_("Work"), wxEmptyString,
                    work) + esc(work) + "</td></tr>";
        }
    }
    if (rows.empty())
        return wxEmptyString;
    return "<section class=\"box\" id=\"key-values\"><div class=\"box-head\">"
        + esc(_("Key values")) + tip(_("The most telling value of every test, as work per "
            "second or as the time of one operation, with the work behind it."))
        + "</div>" + tableStart({ { _("Test"), false }, { _("Result"), true },
            { _("Work"), false } }) + rows + "</tbody></table></section>\n";
}

wxString partRow(const Section& s, const BenchmarkReport& report)
{
    return "<details class=\"part\"><summary class=\"row plain\"><span class=\"part-name\">"
        + esc(getBenchmarkReportSectionName(s.category)) + "</span><span class=\"part-text\">"
        + esc(getBenchmarkSectionDescription(s.category, report.metrics))
        + "</span><span class=\"chev\"></span></summary><div class=\"part-body\">"
        + partValues(s, report) + "</div></details>";
}

wxString partsBox(const BenchmarkReport& report, const std::vector<Section>& sections)
{
    wxString html = "<section class=\"box\" id=\"parts\"><div class=\"row plain parts-head\">"
        "<span>" + esc(_("Part")) + "</span><span class=\"part-text\">"
        + esc(_("What it shows")) + "</span><span></span></div>";
    for (const Section& s : sections)
    {
        // the programming patterns are of interest to developers only
        if (s.category != BenchmarkCategory::Application && !s.empty())
            html += partRow(s, report);
    }
    return html + "<div class=\"box-foot\">" + esc(_("Click a part for all its values. "))
        + "<label class=\"link\" for=\"v2\">" + esc(_("The detailed report has charts, "
        "every measured value and what each test does.")) + "</label></div></section>\n";
}

// ---------------------------------------------------------------- two runs

// green when faster, red when slower, grey within the normal variation
wxString statusOf(double ratio)
{
    if (getBenchmarkChangeLevel(ratio) == 0)
        return "info";
    return ratio > 1.0 ? "ok" : "bad";
}

wxString changeClass(double ratio)
{
    const int level = getBenchmarkChangeLevel(ratio);
    if (level == 0)
        return "same";
    return wxString(ratio > 1.0 ? "up" : "down") + wxString::Format("%d", level);
}

wxString changeChip(double ratio)
{
    return "<span class=\"chg " + changeClass(ratio) + "\">" + esc(formatBenchmarkChange(ratio))
        + "</span>";
}

wxString newerLabel(const BenchmarkComparison& c)
{
    return c.newerIsCurrentRun ? _("This run") : _("Later run");
}

wxString pointsOf(double ratio)
{
    return formatBenchmarkPoints(BenchmarkComparison::earlierPoints * ratio);
}

// the first sentence of an advice, for one line in a list
wxString shortTip(const wxString& advice)
{
    const size_t end = advice.find(". ");
    return end == wxString::npos ? advice : advice.Left(end + 1);
}

wxString comparisonPointsBox(const BenchmarkComparison& c)
{
    wxString html = "<section class=\"box\" id=\"comparison\"><div class=\"box-head\">"
        + esc(_("Points")) + tip(getBenchmarkUnitHint("points")) + "</div>";
    if (c.ratio <= 0.0)
    {
        return html + "<div class=\"box-body quiet\">" + esc(describeBenchmarkComparison(c))
            + "</div></section>\n";
    }
    const double earlier = BenchmarkComparison::earlierPoints;
    const double newer = earlier * c.ratio;
    html += "<div class=\"box-body points-grid\"><div><span class=\"muted\">"
        + esc(newerLabel(c)) + "</span><br><span class=\"points-num\">"
        + esc(formatBenchmarkPoints(newer)) + "</span><span class=\"points-unit\">"
        + esc(_("points")) + "</span><br><span class=\"rating " + statusOf(c.ratio)
        + "-text\">" + esc(describeBenchmarkComparison(c)) + "</span></div><div>"
        + horizontalBars({ { _("Earlier run"), earlier, formatBenchmarkPoints(earlier), 3 },
            { newerLabel(c), newer, formatBenchmarkPoints(newer), 1 } })
        + "</div></div><div class=\"box-foot\">";
    if (!c.sameKind)
    {
        html += "<span class=\"warn-text\">" + esc(_("The two runs were made in different ways "
            "(with or without a network, or with another intensity), so they are only roughly "
            "comparable.")) + "</span> ";
    }
    return html + esc(_("The earlier run has 1000 points. Differences of less than 15 % are "
        "normal between two runs.")) + "</div></section>\n";
}

// one row of a comparison table: the value with its work in both runs
wxString comparedRow(const BenchmarkComparedValue& v, const wxString& older,
    const wxString& newer)
{
    return "<tr><td>" + esc(v.title) + "</td>" + valueCell(older, v.olderText, v.olderWork)
        + valueCell(newer, v.newerText, v.newerWork) + cellLabel(_("Change"), "n")
        + changeChip(v.ratio) + "</td></tr>";
}

// what could make the slower run of a part slower
wxString reasons(const BenchmarkComparedPart& p, const BenchmarkComparison& c)
{
    wxString items;
    for (const auto& d : p.differences)
    {
        items += "<li><b>" + esc(d.label) + "</b>: " + esc(d.olderText + " "
            + wxUniChar(0x2192) + " " + d.newerText) + "</li>";
    }
    // two hints can give the same advice, e.g. the exclusions of a virus
    // scanner; it is shown once
    std::vector<wxString> shown;
    for (const auto& h : p.hints)
    {
        const wxString advice = shortTip(h.advice);
        const bool repeated = std::find(shown.begin(), shown.end(), advice) != shown.end();
        shown.push_back(advice);
        items += "<li><b>" + esc(h.title) + "</b>" + tip(h.detail)
            + (advice.empty() || repeated ? wxString() : " " + wxString(wxUniChar(0x2013))
                + " " + esc(advice)) + "</li>";
    }
    for (const auto& a : p.advice)
        items += "<li><b>" + esc(a.first) + "</b>: " + esc(a.second) + "</li>";
    if (items.empty())
        return wxEmptyString;
    return "<div class=\"reasons\"><h4>" + esc(getBenchmarkReasonsTitle(c, p)) + "</h4><ul>"
        + items + "</ul></div>";
}

wxString comparisonParts(const BenchmarkComparison& c)
{
    const wxString older = _("Earlier run");
    const wxString newer = newerLabel(c);
    wxString html = "<section class=\"box\" id=\"parts\"><div class=\"row parts-head\"><span>"
        + esc(_("Part")) + "</span><span class=\"n\">" + esc(older) + "</span><span class=\"n\">"
        + esc(newer) + "</span><span class=\"part-text\">" + esc(_("Change"))
        + "</span><span></span></div>";
    for (const auto& p : c.parts)
    {
        wxString rows;
        for (const auto& v : c.values)
        {
            if (v.category == p.category)
                rows += comparedRow(v, older, newer);
        }
        wxString further;
        for (const auto& v : c.stats)
        {
            if (v.category == p.category)
                further += comparedRow(v, older, newer);
        }
        if (!further.empty())
        {
            rows += (rows.empty() ? wxString() : "<tr class=\"group\"><td colspan=\"4\">"
                + esc(_("Further measurements")) + "</td></tr>") + further;
        }
        const size_t reasonCount = p.differences.size() + p.hints.size() + p.advice.size();
        wxString text = p.ratio > 0.0 ? changeChip(p.ratio) : wxString();
        if (reasonCount > 0)
        {
            text += " <span class=\"muted\">" + esc(wxString::Format(wxPLURAL(
                "%d possible reason", "%d possible reasons", int(reasonCount)),
                int(reasonCount))) + "</span>";
        }
        html += "<details class=\"part\"><summary class=\"row\"><span class=\"part-name\">"
            "<i class=\"dot " + statusOf(p.ratio) + "\"></i>"
            + esc(getBenchmarkReportSectionName(p.category)) + "</span><span class=\"n\">"
            + (p.ratio > 0.0 ? esc(pointsOf(1.0)) : wxString()) + "</span><span class=\"n\">"
            + (p.ratio > 0.0 ? esc(pointsOf(p.ratio)) : wxString())
            + "</span><span class=\"part-text\">" + text + "</span><span class=\"chev\"></span>"
            "</summary><div class=\"part-body\">";
        if (!rows.empty())
        {
            html += tableStart({ { _("Test"), false }, { older, true }, { newer, true },
                { _("Change"), true } }) + rows + "</tbody></table>";
        }
        html += reasons(p, c) + "</div></details>";
    }
    return html + "<div class=\"box-foot\">" + esc(_("Click a part for all its values and, "
        "where it differs clearly, what could be the reason.")) + "</div></section>\n";
}

// hardware, versions and settings that differ between the runs
wxString differencesBox(const BenchmarkComparison& c)
{
    const wxString older = _("Earlier run");
    const wxString newer = newerLabel(c);
    wxString html = "<section class=\"box\" id=\"differences\"><div class=\"box-head\">"
        + esc(_("What is different")) + "</div>";
    if (c.differences.empty())
    {
        return html + "<div class=\"box-body quiet\">" + esc(_("Hardware, versions and "
            "settings are the same.")) + "</div></section>\n";
    }
    html += tableStart({ { wxString(), false }, { older, false }, { newer, false } });
    for (const auto& d : c.differences)
    {
        html += "<tr><td>" + esc(d.label) + "</td>" + cellLabel(older) + esc(d.olderText)
            + "</td>" + cellLabel(newer) + esc(d.newerText) + "</td></tr>";
    }
    return html + "</tbody></table></section>\n";
}

wxString comparisonView(const BenchmarkComparison& c)
{
    return comparisonPointsBox(c) + comparisonParts(c) + differencesBox(c);
}

// the start of a page: head, styles and the opened <main>
wxString pageStart(const wxString& theme, const wxString& extraStyles)
{
    wxString html = "<!doctype html>\n<html lang=\"en\"";
    if (theme == "dark" || theme == "light")
        html += " data-theme=\"" + theme + "\"";
    return html + "><head><meta charset=\"utf-8\"><meta name=\"viewport\" "
        "content=\"width=device-width, initial-scale=1\"><title>"
        + esc(_("Performance Benchmark & Diagnosis")) + "</title><style>"
        + wxString::FromUTF8(styleSheet) + extraStyles + "</style></head><body><main>\n";
}

} // namespace

wxString BenchmarkComparison::toHtml(const wxString& theme) const
{
    const wxString dot = wxString(" ") + wxUniChar(0x00B7) + " ";
    wxString html = pageStart(theme, wxEmptyString);
    html += "<header><div class=\"brand\">" + esc("FlameRobin" + dot
        + _("Performance Benchmark & Diagnosis")) + "</div><h1>"
        + esc(_("Comparison of two benchmark runs")) + "</h1></header>\n";
    auto runBox = [](const BenchmarkSnapshot& run, const wxString& label)
    {
        return "<div class=\"box\"><div class=\"box-head\">" + esc(label) + "</div>"
            "<div class=\"box-body\"><b>" + esc(run.title) + "</b><br><span class=\"muted\">"
            + esc(run.date + (run.host.empty() ? wxString() : ", " + run.host)) + "<br>"
            + esc(getBenchmarkKindName(run.kind)) + "</span></div></div>";
    };
    html += "<div class=\"runs\">" + runBox(older, _("Earlier run"))
        + runBox(newer, newerLabel(*this)) + "</div>\n" + comparisonView(*this);
    html += "<footer>" + esc(_("Created by FlameRobin from two saved benchmark reports."))
        + "</footer>\n</main></body></html>\n";
    return html;
}

wxString BenchmarkReport::toHtml(const wxString& theme) const
{
    const std::vector<Section> sections = collectSections(*this);
    const BenchmarkSnapshot snapshot = makeBenchmarkSnapshot(*this);

    // a click on a tile shows its panel
    wxString tileStyles;
    for (const Section& s : sections)
    {
        tileStyles += wxString::Format("#c%d:checked~.tiles .p%d{display:block}"
            "#c%d:checked~.tiles .t%d{border-color:var(--link)}"
            "#c%d:checked~.tiles .t%d .open{display:none}"
            "#c%d:checked~.tiles .t%d .shut{display:inline}",
            s.index, s.index, s.index, s.index, s.index, s.index, s.index, s.index);
    }
    wxString html = pageStart(theme, tileStyles);
    // the two views: the overview is shown first
    html += "<input type=\"radio\" name=\"view\" class=\"view-toggle\" id=\"v1\" checked>"
        "<input type=\"radio\" name=\"view\" class=\"view-toggle\" id=\"v2\">\n";

    // header
    const wxString dot = wxString(" ") + wxUniChar(0x00B7) + " ";
    html += "<header><div class=\"brand\">" + esc("FlameRobin" + dot
        + _("Performance Benchmark & Diagnosis")) + "</div><h1>"
        + esc(getBenchmarkSystemTitle(*this)) + "</h1><p class=\"specs\">"
        + esc(getBenchmarkSystemSpecs(*this)) + "</p><p class=\"meta\">"
        + esc(metrics.localHostName + dot + intensityName + dot + startedAt + dot
            + wxString::Format(_("%.0f s"), durationSeconds)) + "</p>";
    if (anonymous)
    {
        html += "<p class=\"notice info\">" + esc(_("Computer and host names, IP addresses, "
            "user names, paths and service names have been removed from this report."))
            + "</p>";
    }
    if (cancelled)
    {
        html += "<p class=\"notice warn\">" + esc(_("The run was cancelled; the results are "
            "incomplete.")) + "</p>";
    }
    else if (!errorMessage.empty())
    {
        html += "<p class=\"notice bad\">" + esc(_("The run failed; the results are "
            "incomplete.") + " " + errorMessage) + "</p>";
    }
    if (benchmarkDatabaseCreated && !benchmarkDatabaseDropped)
    {
        html += "<p class=\"notice bad\">" + esc(wxString::Format(_("The temporary database "
            "%s could not be dropped. FlameRobin offers to drop it the next time the "
            "dialog opens."), benchmarkDatabasePath)) + "</p>";
    }
    // the navigation of the property pages: Overview | Detailed report
    html += "</header>\n<nav class=\"pages\"><label for=\"v1\">" + esc(_("Overview"))
        + "</label> | <label for=\"v2\">" + esc(_("Detailed report")) + "</label></nav>\n";

    // the overview: the values of every part, or the comparison with an
    // earlier run
    html += "<div class=\"overview\">";
    if (earlier)
    {
        BenchmarkComparison c = compareBenchmarkSnapshots(snapshot, *earlier);
        c.newerIsCurrentRun = true;
        html += "<p class=\"meta\">" + esc(wxString::Format(_("Compared with the earlier run "
            "%s of %s."), earlier->title, earlier->date)) + "</p>" + comparisonView(c);
    }
    else
        html += introBox() + keyValuesBox(*this, sections) + partsBox(*this, sections);
    html += "</div>\n";

    html += "<div class=\"details\">" + systemBlock(*this);
    // the tiles; check boxes before them open the panels without a script
    html += "<h2>" + esc(_("Parts of the system")) + "</h2><section class=\"cats\" id=\"categories\">";
    for (const Section& s : sections)
        html += wxString::Format("<input type=\"checkbox\" class=\"toggle\" id=\"c%d\">", s.index);
    // every panel follows its tile in the grid: on a phone it opens right
    // below the tile, on wider screens below the row of the tile
    html += "<div class=\"tiles\">";
    for (const Section& s : sections)
    {
        if (!s.empty())
            html += tile(s, *this) + panel(s, *this);
    }
    html += "</div></section>\n";

    auto block = [](const wxString& id, const wxString& title, const wxString& hint,
        const wxString& body)
    {
        return "<details class=\"block\" id=\"" + id + "\"><summary>" + esc(title)
            + (hint.empty() ? wxString() : " <span class=\"hint\">" + esc(hint) + "</span>")
            + "<span class=\"chev\"></span></summary><div class=\"block-body\">" + body
            + "</div></details>\n";
    };
    html += "<h2>" + esc(_("More")) + "</h2>";
    html += block("values", _("All measured values"), _("every test with amount, time, "
        "average and rate"), statisticsTable(sections));
    html += block("glossary", _("Glossary"), _("abbreviations, terms and what every test "
        "does"), glossary(*this));
    html += block("run", _("Run details"), wxEmptyString, runDetails(*this));

    html += "</div>\n<footer>" + esc(_("Created by FlameRobin. All tests ran in a temporary "
        "database that was created for the benchmark and dropped afterwards; no other "
        "database was changed.")) + "</footer>\n</main>\n"
        // the results for later comparisons, see parseBenchmarkSnapshot()
        + formatBenchmarkSnapshot(snapshot) + "</body></html>\n";
    return html;
}

} // namespace fr
