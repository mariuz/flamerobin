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

#include <wx/arrstr.h>
#include <wx/intl.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

#include "engine/Benchmark.h"

// The HTML report: one self-contained page without scripts or external
// resources, shown in the benchmark dialog and saved as a file. The
// overview is for everybody, also for administrators who know neither
// Firebird nor databases: a few key figures and what was noticed in the
// run, or, compared with another run, how many key figures got faster or
// slower, the parts that differ most with the likely reason and what to
// try. A single run is not judged, because whether a value is fast or slow
// depends on the environment. Everything else is one click away: the
// detailed comparison with every value of both runs, and the detailed
// report for developers with one line per part that opens its values and
// charts, every measured value and the explanations. The views and the
// parts open without a script (radio buttons and <details>); Internet
// Explorer in an older mode shows all of them. Charts are inline SVG or CSS
// bars drawn to scale; all colors are theme tokens.

namespace fr
{

namespace
{

// The style sheet is split into several literals, because MSVC limits a
// single string literal to about 16 KB.
const char* const styleSheet = R"CSS(
:root {
  --bg: #ffffff; --ink: #1d2430; --ink-2: #4a5466; --muted: #667183; --line: #e4e8ee; --line-2: #c9d0da;
  --head: #f3f5f8; --head-ink: #1d2430; --alt: #f7f8fa; --link: #1f62c0; --link-hover: #174d99;
  --ok: #2f855a; --ok-bg: #e6f4ea; --warn: #b35c00; --warn-bg: #fdf3dc;
  --bad: #c53030; --bad-bg: #fde8e8; --info-bg: #edf3fc;
  --up: #2e9e5b; --up-ink: #1e7a43; --down: #b3261e; --down-ink: #b3261e;
  --a-cpu: #6a4fd0; --a-disk: #00897b; --a-mem: #b06f00; --a-net: #c0397b; --a-users: #1d7fc0; --a-app: #667183;
  --c1: #1f6fd1; --c2: #d0590b; --c3: #a9b2bf;
  --no-gap: 0;
  color-scheme: light;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --bg: #1c1e22; --ink: #e8eaed; --ink-2: #b9c0cb; --muted: #959eab; --line: #32363d; --line-2: #4a505a;
    --head: #24272c; --head-ink: #ffffff; --alt: #212428; --link: #6aa6f5; --link-hover: #8fbcf8;
    --ok: #68d391; --ok-bg: #1f3a2a; --warn: #f0a04b; --warn-bg: #3d3216;
    --bad: #fc8181; --bad-bg: #3f2222; --info-bg: #1f2b3d;
    --up: #58c785; --up-ink: #58c785; --down: #e5534b; --down-ink: #f06a62;
    --a-cpu: #8672e6; --a-disk: #16a08f; --a-mem: #c4861a; --a-net: #dc5a95; --a-users: #3896dc; --a-app: #959eab;
    --c1: #5aa2f0; --c2: #f0913f; --c3: #5c6573;
    color-scheme: dark;
  }
}
:root[data-theme="dark"] {
  --bg: #1c1e22; --ink: #e8eaed; --ink-2: #b9c0cb; --muted: #959eab; --line: #32363d; --line-2: #4a505a;
  --head: #24272c; --head-ink: #ffffff; --alt: #212428; --link: #6aa6f5; --link-hover: #8fbcf8;
  --ok: #68d391; --ok-bg: #1f3a2a; --warn: #f0a04b; --warn-bg: #3d3216;
  --bad: #fc8181; --bad-bg: #3f2222; --info-bg: #1f2b3d;
  --up: #58c785; --up-ink: #58c785; --down: #e5534b; --down-ink: #f06a62;
  --a-cpu: #8672e6; --a-disk: #16a08f; --a-mem: #c4861a; --a-net: #dc5a95; --a-users: #3896dc; --a-app: #959eab;
  --c1: #5aa2f0; --c2: #f0913f; --c3: #5c6573;
  color-scheme: dark;
}

/* one family in two weights (400, 600), an 8 px grid, one corner radius
   (8 px), hairlines instead of boxes. The fonts are written out and not
   taken from var(), because Internet Explorer takes "font: 13px var(--x)"
   for the name of a font and falls back to a serif font */
* { box-sizing: border-box; }
html, body { margin: 0; background: var(--bg); color: var(--ink); font-size: 14px; line-height: 1.5; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; }
main { display: block; max-width: 1100px; padding: 24px 24px 32px; }
a, .link { color: var(--link); text-decoration: none; cursor: pointer; }
a:hover, .link:hover { color: var(--link-hover); text-decoration: underline; }
h1 { font-size: 24px; font-weight: 600; line-height: 1.25; margin: 0; overflow-wrap: anywhere; }
h2 { font-size: 16px; font-weight: 600; margin: 32px 0 8px; }
h3, h4 { font-size: 14px; font-weight: 600; margin: 16px 0 8px; }
b, strong, th { font-weight: 600; }
.num, .n { font-variant-numeric: tabular-nums; }
.quiet, small, .muted { color: var(--muted); }
p { max-width: 100ch; }
code { font-family: SFMono-Regular, Consolas, "Liberation Mono", Menlo, Courier, monospace; font-size: 12px; overflow-wrap: anywhere; }

/* pictograms: one <svg><symbol> block per page, every use draws in the
   color of its text; the mode of Internet Explorer 8 shows none of them,
   the short word next to them says the same */
.sprite { position: absolute; width: 0; height: 0; overflow: hidden; }
.ico { width: 24px; height: 24px; fill: none; stroke: currentColor; stroke-width: 1.75; stroke-linecap: round; stroke-linejoin: round; vertical-align: middle; flex: none; }
.ico.s { width: 16px; height: 16px; stroke-width: 2; }
.cpu { color: var(--a-cpu); } .disk { color: var(--a-disk); } .mem { color: var(--a-mem); }
.net { color: var(--a-net); } .users { color: var(--a-users); } .app { color: var(--a-app); }

/* head: brand, the system as title, its specification, the run as chips */
.brand { font-size: 12px; color: var(--muted); margin-bottom: 8px; }
.specs { margin: 8px 0 0; max-width: none; color: var(--ink-2); overflow-wrap: anywhere; }
.chips { display: flex; flex-wrap: wrap; align-items: center; gap: 8px; margin-top: 16px; }
.chips > * { margin: 0 8px 8px 0; margin: var(--no-gap); }
.chip { display: inline-flex; align-items: center; gap: 8px; min-height: 24px; padding: 0 8px; border: 1px solid var(--line); border-radius: 8px; font-size: 12px; color: var(--ink-2); white-space: nowrap; }
.chip .ico { width: 14px; height: 14px; stroke-width: 2; color: var(--muted); }
.chip > * + * { margin-left: 8px; margin-left: var(--no-gap); }
.chip.ask { position: relative; color: var(--link); cursor: help; }
.chip.ask .ico { color: var(--link); }
.lead { font-size: 12px; color: var(--muted); margin: 8px 0 0; overflow-wrap: anywhere; }
.notice { border-radius: 8px; padding: 8px 16px; margin-top: 16px; overflow-wrap: anywhere; }
.notice.bad { background: var(--bad-bg); } .notice.warn { background: var(--warn-bg); } .notice.info { background: var(--info-bg); }

/* Overview | Detailed comparison | Detailed report: tabs over a hairline.
   What hides something depends on :checked, which the Internet Explorer
   control knows only in the mode of Internet Explorer 9 and later: in an
   older mode every view and every part are shown, and the links that
   would not work are not */
.view-toggle, .filter-toggle { position: fixed; top: 0; left: 0; width: 1px; height: 1px; opacity: 0; pointer-events: none; clip: rect(0 0 0 0); }
.pages { display: none; margin: 24px 0 0; border-bottom: 1px solid var(--line); }
.view-toggle:checked ~ .pages { display: block; }
.pages label { display: inline-block; padding: 8px 0; margin-right: 24px; margin-bottom: -1px; border-bottom: 2px solid transparent; color: var(--ink-2); cursor: pointer; }
.pages label:hover { color: var(--ink); }
#v1:checked ~ .pages label[for="v1"], #v2:checked ~ .pages label[for="v2"], #v3:checked ~ .pages label[for="v3"] { color: var(--ink); font-weight: 600; border-bottom-color: var(--ink); cursor: default; }
#v1:checked ~ .details, #v1:checked ~ .cmp-details, #v2:checked ~ .overview, #v2:checked ~ .cmp-details, #v3:checked ~ .overview, #v3:checked ~ .details { display: none; }

/* the key figures: one per line, the large number on the left, the
   pictogram and a line in the color of the part; no rating */
.fig { display: flex; align-items: center; gap: 16px; min-width: 0; border-top: 1px solid var(--line); border-left: 2px solid var(--line); padding: 16px 0 16px 16px; }
.figs .fig:last-child { border-bottom: 1px solid var(--line); }
.fig > * { margin-right: 16px; margin-right: var(--no-gap); }
.fig.cpu { border-left-color: var(--a-cpu); } .fig.disk { border-left-color: var(--a-disk); } .fig.mem { border-left-color: var(--a-mem); }
.fig.net { border-left-color: var(--a-net); } .fig.users { border-left-color: var(--a-users); } .fig.app { border-left-color: var(--a-app); }
.tag { display: inline-flex; align-items: center; gap: 8px; width: 88px; flex: none; vertical-align: middle; }
.tag .ico { width: 20px; height: 20px; }
.tag > * + * { margin-left: 8px; margin-left: var(--no-gap); }
.tag-word { font-size: 12px; font-weight: 600; line-height: 1; letter-spacing: .08em; color: var(--ink-2); white-space: nowrap; }
.fig-val { flex: none; white-space: nowrap; }
.fig-num { display: inline-block; width: 144px; color: var(--ink); font-size: 40px; font-weight: 600; line-height: 1.1; text-align: right; font-variant-numeric: tabular-nums; letter-spacing: -0.5px; white-space: nowrap; flex: none; }
.fig-unit { display: inline-block; width: 56px; margin-left: 4px; font-size: 14px; font-weight: 400; letter-spacing: 0; color: var(--ink-2); text-align: left; }
.fig-text { flex: 1 1 auto; min-width: 0; }
.fig-name { display: block; color: var(--ink); }
.fig-work { display: block; color: var(--muted); font-size: 12px; }
)CSS"
R"CSS(
/* the overview: figures on the left, notes and the rows on the right */
.split { display: flex; align-items: flex-start; gap: 48px; }
.split > * { flex: 1 1 0%; min-width: 0; margin-right: 48px; margin-right: var(--no-gap); }
.split > :first-child { flex: 0 1 576px; }

/* section labels, notes, the rows that open */
.label { display: flex; flex-wrap: wrap; align-items: baseline; gap: 0 8px; font-size: 12px; font-weight: 600; color: var(--ink-2); letter-spacing: .4px; text-transform: uppercase; margin: 0 0 8px; }
.label.first { margin-top: 32px; margin-bottom: 16px; }
.label .quiet { margin-left: 8px; margin-left: var(--no-gap); font-weight: 400; text-transform: none; letter-spacing: 0; }
.label > * + * { margin-left: 8px; margin-left: var(--no-gap); }
.notes { margin: 0 0 32px; border-top: 1px solid var(--line); }
.notes-more { margin: -24px 0 32px; font-size: 12px; }
details > summary { list-style: none; cursor: pointer; }
details > summary::-webkit-details-marker { display: none; }
.note { border-bottom: 1px solid var(--line); }
.note > summary { display: flex; align-items: center; gap: 16px; padding: 12px 0; }
.note > summary > * { margin-right: 16px; margin-right: var(--no-gap); }
.note > summary:hover .note-title { color: var(--link); }
.note .ico.s { color: var(--link); }
.note-title { flex: 1 1 auto; min-width: 0; }
.note-where { margin-left: 16px; margin-left: var(--no-gap); font-size: 12px; color: var(--muted); white-space: nowrap; }
.note-body { padding: 0 32px 16px; color: var(--ink-2); }
.note-body p { margin: 0 0 8px; }
.note-body p:last-child { margin-bottom: 0; }
.note-body b { color: var(--ink); }
.chev { display: inline-block; width: 8px; height: 8px; border-right: 2px solid var(--muted); border-bottom: 2px solid var(--muted); transform: rotate(-45deg); margin: 0 4px 0 auto; flex: none; }
details[open] > summary .chev { transform: rotate(45deg); }
.more { border-bottom: 1px solid var(--line); }
.rows { border-top: 1px solid var(--line); }
.more > summary, label.more { display: flex; align-items: center; gap: 16px; padding: 16px 0; cursor: pointer; }
.more > summary > *, label.more > * { margin-right: 16px; margin-right: var(--no-gap); }
.more-title { font-weight: 600; white-space: nowrap; }
.side .more > summary, .side label.more { flex-wrap: wrap; row-gap: 0; }
.side .more-hint { order: 2; flex-basis: 100%; margin-left: 0; }
.more-hint { margin-left: 16px; margin-left: var(--no-gap); color: var(--muted); font-size: 12px; }
.more > summary:hover .more-title, label.more:hover .more-title { color: var(--link); }
.more-body { padding: 0 0 24px; }
.more-body .label { margin-top: 16px; }
label.more .go { margin-left: auto; color: var(--link); font-weight: 600; }
label.more { display: none; }
.view-toggle:checked ~ .overview label.more { display: flex; }
summary:focus-visible, label:focus-visible, .tip:focus, .chip.ask:focus { outline: 2px solid var(--link); outline-offset: 2px; }

/* tables: hairlines, a quiet head, numbers on the right */
table { width: 100%; border-collapse: collapse; margin: 8px 0; }
th { color: var(--muted); font-size: 12px; text-align: left; padding: 8px; border-bottom: 1px solid var(--line-2); white-space: nowrap; }
td { padding: 8px; border-bottom: 1px solid var(--line); vertical-align: top; overflow-wrap: anywhere; }
th:first-child, td:first-child { padding-left: 0; }
th:last-child, td:last-child { padding-right: 0; }
td.n, th.n { text-align: right; white-space: nowrap; font-variant-numeric: tabular-nums; }
td small { display: block; font-size: 12px; white-space: normal; }
tr.group td { padding-top: 24px; font-weight: 600; border-bottom-color: var(--line-2); }
tr.group .tag { width: auto; margin-right: 8px; }
tr.group .tag .ico { width: 16px; height: 16px; }

/* a comparison: the verdict first, one line per key figure */
.runs { display: flex; flex-wrap: wrap; align-items: center; gap: 8px; margin-top: 16px; }
.runs > * { margin: 0 8px 8px 0; margin: var(--no-gap); }
.runs .to { color: var(--muted); }
.verdict { margin: 40px 0 8px; font-size: 40px; font-weight: 600; line-height: 1.15; letter-spacing: -0.5px; }
.verdict .v-faster, .verdict .v-slower, .verdict .v-same { white-space: nowrap; }
.verdict .sep { color: var(--muted); font-weight: 400; }
.verdict .v-faster { color: var(--up-ink); } .verdict .v-slower { color: var(--down-ink); } .verdict .v-same { color: var(--muted); font-weight: 400; }
.verdict-sub { margin: 0 0 32px; color: var(--ink-2); }
.verdict-sub b { font-variant-numeric: tabular-nums; }
.verdict-sub .sep { color: var(--muted); }
.verdict-note { margin: -24px 0 32px; color: var(--ink-2); }
.verdict-note .ico { color: var(--warn); margin-right: 8px; }
.cmp { margin: 0 0 40px; }
.cmp td { padding-top: 16px; padding-bottom: 16px; vertical-align: middle; }
.cmp td.tagcol { width: 104px; }
.cmp .old { color: var(--muted); }
.cmp .arrow { color: var(--muted); padding-left: 0; padding-right: 0; }
.cmp .new { font-weight: 600; }
.cmp .unit { color: var(--muted); font-size: 12px; font-weight: 400; margin-left: 4px; }
.cmp .pts { font-size: 16px; font-weight: 600; }
.cmp .delta { display: block; font-size: 12px; }
.up-text { color: var(--up-ink); } .down-text { color: var(--down-ink); } .same-text { color: var(--muted); }
.cmp th.bar-col, .cmp td.bar-col { width: 144px; }
)CSS"
R"CSS(
/* the ratio as a bar from the middle: faster to the right (green), slower
   to the left (red); the triangle and the words say the same, also for
   readers who cannot tell red from green */
.dbar { position: relative; display: block; width: 128px; height: 8px; margin: 0 auto; background: var(--alt); border-radius: 8px; }
.dbar:after { content: ""; position: absolute; left: 50%; margin-left: -1px; top: -4px; width: 2px; height: 16px; background: var(--line-2); }
.dbar .fill { position: absolute; top: 0; height: 8px; }
.dbar .fill.faster { left: 50%; background: var(--up); border-radius: 0 8px 8px 0; }
.dbar .fill.slower { right: 50%; background: var(--down); border-radius: 8px 0 0 8px; }
.dir { font-weight: 600; white-space: nowrap; }
.dir.faster { color: var(--up-ink); } .dir.slower { color: var(--down-ink); } .dir.same { color: var(--muted); font-weight: 400; }
.dir:before { content: ""; display: inline-block; margin-right: 8px; vertical-align: 1px; }
.dir.faster:before { border-left: 5px solid transparent; border-right: 5px solid transparent; border-bottom: 8px solid; }
.dir.slower:before { border-left: 5px solid transparent; border-right: 5px solid transparent; border-top: 8px solid; }
.dir.same:before { width: 10px; height: 6px; border-top: 2px solid; border-bottom: 2px solid; vertical-align: 2px; }
.dir.l1 { font-weight: 400; }

/* what stands out: the parts that differ clearly, the largest change
   first, each with the likely reason and what to try; more on a click */
.stand { margin: 0 0 32px; border-top: 1px solid var(--line); }
.so { border-bottom: 1px solid var(--line); }
.so > summary, .so > .so-head { display: flex; align-items: flex-start; gap: 16px; padding: 16px 0; }
.so > summary > *, .so > .so-head > * { margin-right: 16px; margin-right: var(--no-gap); }
.so > summary:hover .so-part { color: var(--link); }
.so-dir { flex: none; width: 144px; margin-left: 8px; margin-left: var(--no-gap); }
.so-text { flex: 1 1 auto; min-width: 0; }
.so-part { display: block; font-weight: 600; margin-bottom: 4px; }
.so-line { display: block; color: var(--ink-2); }
/* only where :root is known: the mode of Internet Explorer 7 and 8 makes
   an element with an unknown display value inline, also after a known one */
:root .so-line { display: flex; align-items: baseline; gap: 8px; }
.so-line > * + * { flex: 1 1 0%; min-width: 0; }
.so-key { display: inline-block; flex: none; width: 104px; color: var(--muted); font-size: 12px; font-weight: 600; letter-spacing: .06em; text-transform: uppercase; }
.so-more { flex: none; margin-left: 8px; margin-left: var(--no-gap); color: var(--muted); font-size: 12px; white-space: nowrap; }
.so-body { padding: 0 0 16px 264px; }
.reason-head { margin: 0 0 8px; color: var(--muted); font-size: 12px; font-weight: 600; letter-spacing: .06em; text-transform: uppercase; }
.reason-list { margin: 0; padding-left: 16px; color: var(--ink-2); }
.reason-list li { margin: 0 0 8px; }
.reason-list b { color: var(--ink); }
.why-more { margin-top: 8px; }
.why-more > summary { display: inline-flex; align-items: center; gap: 8px; color: var(--link); font-size: 12px; }
.why-more > summary > * { margin-left: 8px; margin-left: var(--no-gap); }
.why-more .reason-head { margin-top: 8px; }
.why-block { margin: 0 0 16px; padding: 0 0 16px; border-bottom: 1px solid var(--line); }

/* the detailed comparison: every value of both runs, a filter without a
   script; without :checked (Internet Explorer 8) every row is shown */
.filter { display: none; margin: 0 0 16px; }
.filter-toggle:checked ~ .filter { display: block; }
.filter label { display: inline-block; padding: 4px 16px; border: 1px solid var(--line-2); color: var(--ink-2); font-size: 12px; cursor: pointer; }
.filter label + label { margin-left: -1px; }
.filter label:first-child { border-radius: 8px 0 0 8px; } .filter label:last-child { border-radius: 0 8px 8px 0; }
#f1:checked ~ .filter label[for="f1"], #f2:checked ~ .filter label[for="f2"] { background: var(--head); color: var(--ink); font-weight: 600; }
#f2:checked ~ .areas tr.same, #f2:checked ~ .areas .unchanged { display: none; }
.badge { display: inline-block; padding: 0 8px; border: 1px solid var(--line-2); border-radius: 8px; font-size: 12px; font-weight: 400; line-height: 20px; color: var(--ink-2); white-space: nowrap; }
.badge.nc { border-style: dashed; }
table.dt .work { font-size: 12px; }
.legend-note { font-size: 12px; margin-top: 16px; }

/* tooltips: a small question mark or a chip, the text on hover or focus */
.tip { position: relative; display: inline-block; width: 16px; height: 16px; margin-left: 4px; border-radius: 8px; border: 1px solid var(--line-2); color: var(--muted); font-style: normal; font-weight: 600; font-size: 10px; line-height: 14px; text-align: center; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; cursor: help; vertical-align: 1px; }
.tip-box { display: none; position: absolute; z-index: 20; top: 100%; left: -8px; margin-top: 8px; width: 280px; padding: 8px 16px; border-radius: 8px; border: 1px solid var(--line-2); background: var(--bg); color: var(--ink); box-shadow: 0 4px 16px rgba(0,0,0,.12); font-style: normal; font-weight: 400; font-size: 12px; line-height: 1.5; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; text-align: left; white-space: normal; text-transform: none; letter-spacing: 0; }
td.n .tip-box, th.n .tip-box, .chip.ask .tip-box { left: auto; right: -8px; }
.tip:hover .tip-box, .tip:focus .tip-box, .chip.ask:hover .tip-box, .chip.ask:focus .tip-box { display: block; }
)CSS"
R"CSS(
/* the detailed report: one line per part like the key figures, a click
   opens its values and charts; Internet Explorer shows them all open */
.areas { margin-top: 16px; border-bottom: 1px solid var(--line); }
.area > summary:hover .fig-name { color: var(--link); }
.area > summary .chev { margin-right: 8px; }
.area-body { color: var(--ink); display: flex; flex-wrap: wrap; align-items: flex-start; gap: 32px; padding: 0 0 32px 56px; border-left: 2px solid var(--line); }
.area-body > * { flex: 1 1 320px; min-width: 0; margin-right: 32px; margin-right: var(--no-gap); }
.area.cpu .area-body { border-left-color: var(--a-cpu); } .area.disk .area-body { border-left-color: var(--a-disk); } .area.mem .area-body { border-left-color: var(--a-mem); }
.area.net .area-body { border-left-color: var(--a-net); } .area.users .area-body { border-left-color: var(--a-users); } .area.app .area-body { border-left-color: var(--a-app); }
.where { color: var(--muted); margin: 0 0 8px; font-size: 12px; overflow-wrap: anywhere; }

/* charts */
.charts { display: grid; grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); gap: 16px; }
.charts > * { margin-bottom: 16px; margin-bottom: var(--no-gap); }
.chart { min-width: 0; overflow: hidden; }
.chart h5 { font-size: 14px; font-weight: 600; margin: 0; }
.chart-sub { color: var(--muted); font-size: 12px; margin: 0 0 8px; overflow-wrap: anywhere; }
.hbars > * { margin-bottom: 8px; }
.hb { display: flex; align-items: center; gap: 8px; font-size: 12px; }
.hb > * { margin-right: 8px; margin-right: var(--no-gap); }
.hb-label { width: 40%; color: var(--ink-2); overflow-wrap: anywhere; }
.hb-track { flex: 1 1 auto; height: 8px; background: var(--alt); border-radius: 8px; overflow: hidden; }
.hb-fill { display: block; height: 100%; border-radius: 8px; min-width: 2px; }
.hb-val { font-variant-numeric: tabular-nums; white-space: nowrap; width: 88px; text-align: right; }
.chart svg { width: 100%; height: auto; display: block; }
.chart svg text { font-family: SFMono-Regular, Consolas, "Liberation Mono", Menlo, Courier, monospace; }
.axis { fill: var(--muted); font-size: 10px; }
.gridline { stroke: var(--line); stroke-width: 1; }
.c1 { fill: var(--c1); background: var(--c1); } .c2 { fill: var(--c2); background: var(--c2); } .c3 { fill: var(--c3); background: var(--c3); }
.ideal { stroke: var(--muted); stroke-width: 1.5; stroke-dasharray: 4 3; fill: none; }
.ideal-dot { fill: var(--bg); stroke: var(--muted); stroke-width: 1.5; }
.best { stroke: var(--ok); stroke-width: 1; stroke-dasharray: 3 3; }
.vlabel { fill: var(--ink); font-size: 10px; paint-order: stroke; stroke: var(--bg); stroke-width: 3px; }
.stack { display: flex; height: 16px; border-radius: 8px; overflow: hidden; background: var(--alt); }
.stack span { display: block; height: 100%; min-width: 2px; }
.legend { display: flex; flex-wrap: wrap; gap: 4px 16px; font-size: 12px; color: var(--muted); margin-top: 8px; }
.legend > * + * { margin-left: 16px; margin-left: var(--no-gap); }
.legend i { display: inline-block; width: 8px; height: 8px; border-radius: 8px; margin-right: 8px; }
.legend i.dash { width: 12px; height: 0; border-top: 2px dashed var(--muted); border-radius: 0; vertical-align: 3px; }

/* system details, facts, explanations */
.sys-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 16px; }
.sys-grid > * { margin-bottom: 16px; margin-bottom: var(--no-gap); }
.sys-card { min-width: 0; }
.sys-card h3 { margin: 0 0 8px; }
.facts { margin: 0; display: grid; grid-template-columns: max-content minmax(0, 1fr); gap: 4px 16px; }
.facts dt { color: var(--muted); }
.facts dd { margin: 0; overflow-wrap: anywhere; }
.explain dt { font-weight: 600; margin-top: 8px; }
.explain dd { margin: 0; color: var(--ink-2); max-width: 90ch; }
.explain dd b { color: var(--ink); }
.deeper { margin-top: 16px; }
.deeper > summary { display: inline-flex; align-items: center; gap: 8px; color: var(--link); }
.deeper > summary > * { margin-left: 8px; margin-left: var(--no-gap); }

.details, .cmp-details { padding-top: 8px; }
footer { margin-top: 40px; color: var(--muted); font-size: 12px; max-width: 100ch; }

@media (max-width: 900px) {
  .split { display: block; }
  .side .label.first { margin-top: 40px; }
}
@media (max-width: 640px) {
  main { padding: 16px 16px 24px; }
  h1 { font-size: 20px; }
  /* the figures and the parts: the pictogram, the number, then the name */
  .fig { display: block; padding: 16px 0 16px 16px; }
  .fig > .tag { display: flex; width: auto; margin: 0 0 8px; }
  .fig-val { display: block; }
  .fig-num { width: auto; text-align: left; font-size: 32px; }
  .fig-unit { width: auto; }
  .fig-name { margin-top: 4px; }
  .area > summary .chev { display: none; }
  .area-body { display: block; padding: 0 0 24px 16px; }
  .verdict { font-size: 28px; margin-top: 32px; }
  .note-where { display: none; }
  .so > summary, .so > .so-head { display: block; position: relative; }
  .so > summary .tag, .so > .so-head .tag { display: flex; width: auto; margin: 0 0 8px; }
  .so-dir { display: block; width: auto; margin: 0 0 4px; }
  .so-more { position: absolute; right: 24px; top: 16px; }
  .so > summary .chev { position: absolute; right: 4px; top: 22px; }
  :root .so-line { display: block; }
  .so-key { display: block; width: auto; }
  .so-body { padding: 0 0 16px; }
  .note-body { padding: 0 0 16px 32px; }
  .more-hint { display: none; }
  .facts { grid-template-columns: minmax(0, 2fr) minmax(0, 3fr); gap: 4px 8px; }
  /* the comparison: one block per key figure */
  .cmp thead { display: none; }
  .cmp, .cmp tbody, .cmp tr { display: block; width: 100%; }
  .cmp tr { position: relative; padding: 16px 0; border-bottom: 1px solid var(--line); }
  .cmp td { display: inline-block; padding: 0; border: 0; }
  .cmp td.area { display: block; font-weight: 600; }
  .cmp td.tagcol { display: block; width: auto; margin-bottom: 8px; }
  .cmp td.arrow { padding: 0 4px; }
  .cmp td.bar-col { display: block; width: auto; margin: 8px 0 0; }
  .cmp .dbar { margin: 0; width: 100%; }
  .cmp td.pts { position: absolute; right: 0; top: 16px; }
  .cmp td.dir-col { display: block; margin-top: 4px; }
  .cmp .delta { display: none; }
  /* tables become cards: one row per block, every cell with its label */
  table.cards thead { display: none; }
  table.cards, table.cards tbody, table.cards tr, table.cards td { display: block; width: 100%; }
  table.cards tr { border-bottom: 1px solid var(--line); padding: 8px 0; }
  table.cards td { border: 0; padding: 0; text-align: left; white-space: normal; }
  table.cards tr.group td { padding-top: 16px; }
  table.cards td:first-child { font-weight: 600; }
  table.cards td[data-label]:not(:first-child):before { content: attr(data-label) ": "; color: var(--muted); }
  table.cards td:empty { display: none; }
  .tip-box { width: 240px; }
}
@media print {
  details > div { display: block !important; }
  .view-toggle, .filter-toggle, .pages, .filter, label.more, .chip.ask { display: none !important; }
  details::details-content { content-visibility: visible; }
  .chev, .go { display: none !important; }
  main { max-width: none; padding: 0; } .split { display: block; }
  .fig-num { font-size: 28px; } .note, .more > summary, .cmp tr, .fig { break-inside: avoid; }
}
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
wxString tableStart(std::initializer_list<Column> columns, const wxString& cls = "cards")
{
    wxString html = "<table class=\"" + cls + "\"><thead><tr>";
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

wxString dash()
{
    return wxString(wxUniChar(0x2014));
}

wxString arrow()
{
    return wxString(" ") + wxUniChar(0x2192) + " ";
}

wxString separator()
{
    return wxString("<span class=\"sep\"> ") + wxUniChar(0x00B7) + " </span>";
}

// ------------------------------------------------------------ pictograms

// 24 x 24, lines in the color of the text; Internet Explorer 11 draws a
// <use> only with xlink:href
const char* const symbols[][2] = {
    { "i-cpu", "<rect x=\"6\" y=\"6\" width=\"12\" height=\"12\" rx=\"2\"/><rect x=\"9.5\" "
        "y=\"9.5\" width=\"5\" height=\"5\" rx=\"1\"/><path d=\"M9 2.5V6M15 2.5V6M9 18v3.5M15 "
        "18v3.5M2.5 9H6M2.5 15H6M18 9h3.5M18 15h3.5\"/>" },
    { "i-commit", "<path d=\"M12 3v8.5M8.5 8 12 11.5 15.5 8\"/><rect x=\"3\" y=\"14.5\" "
        "width=\"18\" height=\"6\" rx=\"2\"/><path d=\"M17 17.5h.01\"/>" },
    { "i-write", "<path d=\"M4 6h16M4 11h16M4 16h7M17 13.5v7M13.5 17h7\"/>" },
    { "i-mem", "<rect x=\"2.5\" y=\"6.5\" width=\"19\" height=\"10\" rx=\"1.5\"/><path "
        "d=\"M7 10v3M12 10v3M17 10v3M6 16.5V19M10 16.5V19M14 16.5V19M18 16.5V19\"/>" },
    { "i-net", "<path d=\"M4 8h14M14.5 4.5 18 8l-3.5 3.5M20 16H6M9.5 12.5 6 16l3.5 3.5\"/>" },
    { "i-users", "<circle cx=\"9\" cy=\"8\" r=\"3.5\"/><path d=\"M2.5 20c0-3.6 2.9-6 6.5-6s6.5 "
        "2.4 6.5 6M15.5 4.8a3.5 3.5 0 0 1 0 6.4M18 14.4c2.1.7 3.5 2.8 3.5 5.6\"/>" },
    { "i-app", "<rect x=\"3\" y=\"4\" width=\"18\" height=\"12\" rx=\"2\"/><path d=\"M8 20h8M12 "
        "16v4\"/>" },
    { "i-code", "<path d=\"M8 7l-5 5 5 5M16 7l5 5-5 5M13.5 5l-3 14\"/>" },
    { "i-conf", "<path d=\"M4 6h9M17 6h3M4 12h3M11 12h9M4 18h11M19 18h1\"/><circle cx=\"15\" "
        "cy=\"6\" r=\"2\"/><circle cx=\"9\" cy=\"12\" r=\"2\"/><circle cx=\"17\" cy=\"18\" "
        "r=\"2\"/>" },
    { "i-info", "<circle cx=\"12\" cy=\"12\" r=\"9\"/><path d=\"M12 11v5.5M12 7.5h.01\"/>" },
    { "i-warn", "<path d=\"M10.3 4.2 2.6 17.6a2 2 0 0 0 1.7 3h15.4a2 2 0 0 0 1.7-3L13.7 4.2a2 2 "
        "0 0 0-3.4 0z\"/><path d=\"M12 9.5v4M12 17h.01\"/>" },
    { "i-date", "<rect x=\"3.5\" y=\"5\" width=\"17\" height=\"15.5\" rx=\"2\"/><path d=\"M3.5 "
        "10h17M8 3v4M16 3v4\"/>" },
    { "i-time", "<circle cx=\"12\" cy=\"12\" r=\"9\"/><path d=\"M12 7v5l3 2\"/>" },
    { "i-load", "<path d=\"M4 17a8 8 0 1 1 16 0\"/><path d=\"M12 17l4-5\"/>" },
    { "i-link", "<path d=\"M9 7H7a5 5 0 0 0 0 10h2M15 7h2a5 5 0 0 1 0 10h-2M8 12h8\"/>" },
    { "i-help", "<circle cx=\"12\" cy=\"12\" r=\"9\"/><path d=\"M9.5 9.5a2.5 2.5 0 1 1 3.5 "
        "2.3c-.6.3-1 .9-1 1.6v.4M12 17h.01\"/>" },
    { "i-go", "<path d=\"M5 12h14M13 6l6 6-6 6\"/>" }
};

// the symbols, once per page right after <body> (see finishPage())
wxString sprite()
{
    wxString html = "<svg class=\"sprite\" aria-hidden=\"true\" focusable=\"false\">";
    for (const auto& s : symbols)
    {
        html += wxString("<symbol id=\"") + s[0] + "\" viewBox=\"0 0 24 24\">" + s[1]
            + "</symbol>";
    }
    return html + "</svg>\n";
}

// a pictogram; the size as an attribute as well, in case the style sheet is
// not applied, and no tab stop in Internet Explorer
wxString icon(const char* symbol, const char* cls = "", int size = 24)
{
    return wxString::Format("<svg class=\"ico%s%s\" width=\"%d\" height=\"%d\" "
        "aria-hidden=\"true\" focusable=\"false\"><use xlink:href=\"#%s\"/></svg>",
        *cls ? " " : "", cls, size, size, symbol);
}

// the color class of a section
const char* toneOf(BenchmarkCategory section)
{
    switch (getBenchmarkReportSection(section))
    {
        case BenchmarkCategory::ServerCpu:
            return "cpu";
        case BenchmarkCategory::ServerStorage:
            return "disk";
        case BenchmarkCategory::ServerCache:
            return "mem";
        case BenchmarkCategory::Network:
            return "net";
        case BenchmarkCategory::MultiUser:
            return "users";
        default:
            return "app";
    }
}

const char* symbolOf(BenchmarkCategory section)
{
    switch (getBenchmarkReportSection(section))
    {
        case BenchmarkCategory::ServerCpu:
            return "i-cpu";
        case BenchmarkCategory::ServerStorage:
            return "i-commit";
        case BenchmarkCategory::ServerCache:
            return "i-mem";
        case BenchmarkCategory::Network:
            return "i-net";
        case BenchmarkCategory::MultiUser:
            return "i-users";
        case BenchmarkCategory::LocalMachine:
            return "i-app";
        case BenchmarkCategory::Application:
            return "i-code";
        default:
            return "i-conf";
    }
}

// writing differs from saving a change within the same section
const char* symbolOf(const wxString& id, BenchmarkCategory section)
{
    return id == "storage.insert" ? "i-write" : symbolOf(section);
}

// the pictogram and the short word of a section
wxString sectionTag(BenchmarkCategory section, const char* symbol)
{
    return "<span class=\"tag\">" + icon(symbol, "", 20) + "<span class=\"tag-word\">"
        + esc(getBenchmarkReportSectionWord(section)) + "</span></span>";
}

wxString sectionTag(BenchmarkCategory section)
{
    return sectionTag(section, symbolOf(section));
}

// a large number with its unit, a name and a small line, after the
// pictogram of its part: a key figure, and the head of a part
wxString figureRow(const wxString& element, BenchmarkCategory section,
    const wxString& tag, const wxString& number, const wxString& unit,
    const wxString& name, const wxString& lineHtml, const wxString& extra)
{
    return "<" + element + " class=\"fig " + toneOf(section) + "\">" + tag
        + "<span class=\"fig-val\"><span class=\"fig-num\">" + esc(number)
        + "</span><span class=\"fig-unit\">" + esc(unit) + "</span></span><span "
        "class=\"fig-text\"><span class=\"fig-name\">" + esc(name) + "</span><span "
        "class=\"fig-work\">" + lineHtml + "</span></span>" + extra + "</" + element + ">";
}

wxString chip(const char* symbol, const wxString& html)
{
    return "<span class=\"chip\">" + icon(symbol, "", 14) + "<span>" + html + "</span></span>";
}

// a chip that explains something on hover or focus
wxString askChip(const wxString& text, const wxString& explanation)
{
    return "<span class=\"chip ask\" tabindex=\"0\">" + icon("i-help", "", 14) + "<span>"
        + esc(text) + "</span><span class=\"tip-box\" role=\"tooltip\">" + esc(explanation)
        + "</span></span>";
}

// a small heading in capitals with a quiet explanation
wxString sectionLabel(const wxString& title, const wxString& hint)
{
    return "<div class=\"label first\">" + esc(title) + (hint.empty() ? wxString()
        : " <span class=\"quiet\">" + esc(hint) + "</span>") + "</div>";
}

// a row that opens on a click
wxString moreRow(const wxString& id, const wxString& title, const wxString& hint,
    const wxString& body)
{
    return "<details class=\"more\" id=\"" + id + "\"><summary><span class=\"more-title\">"
        + esc(title) + "</span><span class=\"more-hint\">" + esc(hint) + "</span><span "
        "class=\"chev\"></span></summary><div class=\"more-body\">" + body
        + "</div></details>\n";
}

// a row that shows another view; hidden where the views cannot switch
wxString viewRow(const wxString& view, const wxString& title, const wxString& hint)
{
    return "<label class=\"more\" for=\"" + view + "\"><span class=\"more-title\">"
        + esc(title) + "</span><span class=\"more-hint\">" + esc(hint) + "</span><span "
        "class=\"go\">" + icon("i-go", "s", 16) + "</span></label>\n";
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

// the tooltip of a key value: the hint of the result it comes from
wxString keyValueHint(const BenchmarkKeyValue& v, const Section& s)
{
    for (const BenchmarkResult* r : s.results)
    {
        if (r->id == v.id && isBenchmarkKeyValueSource(*r))
            return r->hint;
    }
    return wxEmptyString;
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
            rows += "<tr><td>" + esc(v.title) + tip(keyValueHint(v, s)) + "</td>"
                + valueCell(result, v.valueText, formatBenchmarkWork(v.amountText,
                    v.durationMs)) + "</tr>";
        }
    }
    wxString further;
    for (const BenchmarkResult* r : s.results)
    {
        if (isBenchmarkKeyValueSource(*r) || (r->durationMs <= 0.0 && r->rate.empty()
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

// the head of a part: its first key value in large, or what it measures
wxString partHead(const Section& s, const BenchmarkReport& report)
{
    wxString number, unit, line;
    if (s.part && !s.part->values.empty())
    {
        const BenchmarkKeyValue& lead = s.part->values.front();
        const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(lead.id);
        if (def && lead.value > 0.0)
        {
            const auto figure = formatBenchmarkFigure(lead.value, *def);
            number = figure.first;
            unit = figure.second;
        }
        line = lead.title + " " + wxUniChar(0x00B7) + " " + wxString::Format(wxPLURAL(
            "%d measurement", "%d measurements", int(s.results.size())), int(s.results.size()));
    }
    else
        line = getBenchmarkSectionDescription(s.category, report.metrics);
    return figureRow("summary", s.category, sectionTag(s.category), number, unit,
        getBenchmarkReportSectionName(s.category), esc(line), "<span class=\"chev\"></span>");
}

// one line per part like the key figures; a click shows its values and
// charts, and on request every value and what each test does
wxString areaRow(const Section& s, const BenchmarkReport& report)
{
    wxString body = "<div><p class=\"where\">"
        + esc(wxString::Format(_("Measured on %s"), machineOf(s, report))) + "</p>"
        + partValues(s, report);
    if (!s.results.empty())
    {
        body += "<details class=\"deeper\"><summary>" + esc(_("More details"))
            + "<span class=\"chev\"></span></summary><h4>"
            + esc(wxString::Format(_("All values (%d)"), int(s.results.size()))) + "</h4>"
            + valuesTable(s.results) + "<h4>" + esc(_("What is tested")) + "</h4>"
            + explanations(s.results) + "</details>";
    }
    body += "</div>";
    const wxString charts = sectionCharts(s.category, report.metrics);
    if (!charts.empty())
        body += "<div>" + charts + "</div>";
    return wxString("<details class=\"area ") + toneOf(s.category) + "\">" + partHead(s, report)
        + "<div class=\"area-body\">" + body + "</div></details>\n";
}

wxString systemBody(const BenchmarkReport& report)
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
    return grid(getBenchmarkSystemOverview(report)) + "<details class=\"deeper\"><summary>"
        + esc(_("All system information")) + "<span class=\"chev\"></span></summary>"
        + grid(getBenchmarkSystemDetails(report)) + "</details>";
}

// what was noticed: the neutral title; a saved report only has the title
wxString noticedTitle(const BenchmarkHint& h)
{
    return h.noticed.empty() ? h.title : h.noticed;
}

// a note with what was measured and what can help; without the details
// when the report hides private information
wxString noteItem(const BenchmarkHint& h, bool anonymous)
{
    return "<li><b>" + esc(noticedTitle(h)) + "</b>" + (anonymous || h.detail.empty()
        ? wxString() : ". " + esc(h.detail)) + (h.advice.empty() ? wxString()
        : " <span class=\"quiet\">" + esc(_("What can help:")) + " " + esc(h.advice)
            + "</span>") + "</li>";
}

// every note of the run by part
wxString allNotes(const BenchmarkReport& report)
{
    wxString html;
    for (BenchmarkCategory section : getBenchmarkReportSections())
    {
        wxString items;
        for (const BenchmarkHint& h : report.hints)
        {
            if (getBenchmarkReportSection(h.category) == section)
                items += noteItem(h, report.anonymous);
        }
        if (!items.empty())
        {
            html += wxString("<div class=\"label\"><span class=\"") + toneOf(section) + "\">"
                + sectionTag(section) + "</span>" + esc(getBenchmarkReportSectionName(section))
                + "</div><ul class=\"reason-list\">" + items + "</ul>";
        }
    }
    return html;
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
        rows.push_back({ _("Not measured"), skipped.name + ": " + skipped.reason });
    return facts(rows);
}

// ---------------------------------------------------------------- one run

// the notes in the overview of a single run
const size_t maximumNotes = 3;

wxString join(const std::vector<wxString>& items, const wxString& with)
{
    wxString text;
    for (const wxString& item : items)
        text += (text.empty() ? wxString() : with) + item;
    return text;
}

// the short words of every part a kind of hint was given for
wxString hintParts(const BenchmarkHint& hint, const BenchmarkReport& report)
{
    std::vector<wxString> words;
    for (const BenchmarkHint& h : report.hints)
    {
        const wxString word = getBenchmarkReportSectionWord(h.category);
        if (h.id == hint.id && std::find(words.begin(), words.end(), word) == words.end())
            words.push_back(word);
    }
    return join(words, ", ");
}

// the key figures in large, with the work behind them; no rating
wxString figuresBox(const std::vector<BenchmarkHeadline>& figures)
{
    if (figures.empty())
        return wxEmptyString;
    wxString html = sectionLabel(_("Key figures"), _("measured values of this run, not rated"))
        + "<div class=\"figs\" id=\"figures\">";
    for (const BenchmarkHeadline& f : figures)
    {
        html += figureRow("div", f.section, sectionTag(f.section, symbolOf(f.id, f.section)),
            f.number, f.unit, f.name, esc(f.work), wxEmptyString);
    }
    return html + "</div>\n";
}

// at most three things noticed in the run, as states or ratios; a click
// shows what was measured and what can help
wxString notesBox(const BenchmarkReport& report)
{
    const std::vector<BenchmarkHint> notes = getBenchmarkTopHints(report, maximumNotes);
    if (notes.empty())
        return wxEmptyString;
    wxString html = sectionLabel(_("Noticed"), _("states and ratios within this run, not rated"))
        + "<div class=\"notes\" id=\"noticed\">";
    for (const BenchmarkHint& h : notes)
    {
        wxString body;
        if (!report.anonymous && !h.detail.empty())
            body += "<p>" + esc(h.detail) + "</p>";
        if (!h.advice.empty())
            body += "<p><b>" + esc(_("What can help:")) + "</b> " + esc(h.advice) + "</p>";
        html += "<details class=\"note\"><summary>" + icon("i-info", "s", 16)
            + "<span class=\"note-title\">" + esc(noticedTitle(h)) + "</span><span "
            "class=\"note-where\">" + esc(hintParts(h, report)) + "</span><span "
            "class=\"chev\"></span></summary><div class=\"note-body\">" + body
            + "</div></details>";
    }
    html += "</div>\n";
    const int more = int(getBenchmarkTopHints(report, report.hints.size()).size()
        - notes.size());
    if (more > 0)
    {
        html += "<p class=\"quiet notes-more\"><label class=\"link\" for=\"v2\">"
            + esc(wxString::Format(wxPLURAL("%d more note in the detailed report",
                "%d more notes in the detailed report", more), more)) + "</label></p>\n";
    }
    return html;
}

// the key values of every part with the work behind them
wxString keyValuesTable(const BenchmarkReport& report, const std::vector<Section>& sections)
{
    wxString rows;
    for (const Section& s : sections)
    {
        if (!s.part)
            continue;
        rows += wxString("<tr class=\"group\"><td colspan=\"2\"><span class=\"")
            + toneOf(s.category) + "\">" + sectionTag(s.category) + "</span>"
            + esc(getBenchmarkReportSectionName(s.category))
            + tip(getBenchmarkSectionDescription(s.category, report.metrics)) + "</td></tr>";
        for (const auto& v : s.part->values)
        {
            const wxString work = formatBenchmarkWork(v.amountText, v.durationMs);
            rows += "<tr><td>" + esc(v.title) + tip(keyValueHint(v, s))
                + (work.empty() ? wxString() : "<small>" + esc(work) + "</small>") + "</td>"
                + cellLabel(_("Result"), "n") + "<b>" + value(v.valueText) + "</b></td></tr>";
        }
    }
    if (rows.empty())
        return wxEmptyString;
    return tableStart({ { _("Test"), false }, { _("Result"), true } }) + rows
        + "</tbody></table>";
}

// a single run: the key figures on the left; what was noticed and the
// rows that open on the right
wxString singleOverview(const BenchmarkReport& report, const std::vector<Section>& sections,
    const BenchmarkSnapshot& snapshot)
{
    const wxString notes = notesBox(report);
    wxString side = notes.empty() ? sectionLabel(_("More"), wxEmptyString) : notes;
    side += "<div class=\"rows\">";
    const wxString values = keyValuesTable(report, sections);
    if (!values.empty())
    {
        side += moreRow("key-values", _("All key values"), _("the most telling value of "
            "every test, with the work behind it"), values);
    }
    side += moreRow("run-summary", _("Run details"), _("target, intensity, temporary "
        "database"), runDetails(report));
    side += viewRow("v2", _("Detailed report"), _("every part with charts, every measured "
        "value, glossary")) + "</div>";
    const wxString figures = figuresBox(getBenchmarkHeadlineValues(snapshot));
    if (figures.empty())
        return "<div class=\"overview\"><div class=\"side\">" + side + "</div></div>\n";
    return "<div class=\"overview\"><div class=\"split\"><div>" + figures
        + "</div><div class=\"side\">" + side + "</div></div></div>\n";
}

// the detailed report: one line per part, then everything else
wxString detailsView(const BenchmarkReport& report, const std::vector<Section>& sections)
{
    wxString html = "<div class=\"details\">" + sectionLabel(_("Parts of the system"),
        _("a click opens the values and charts of a part"))
        + "<div class=\"areas\" id=\"categories\">";
    for (const Section& s : sections)
    {
        if (!s.empty())
            html += areaRow(s, report);
    }
    html += "</div>\n" + sectionLabel(_("More"), wxEmptyString) + "<div class=\"rows\">";
    html += moreRow("system", _("System details"), _("hardware, software, network and the "
        "test database"), systemBody(report));
    html += moreRow("values", _("All measured values"), _("every test with amount, time, "
        "average and rate"), statisticsTable(sections));
    const size_t notes = getBenchmarkTopHints(report, report.hints.size()).size();
    if (notes > 0)
    {
        html += moreRow("notes", _("All notes"), wxString::Format(wxPLURAL("%d note, by part",
            "%d notes, by part", int(notes)), int(notes)), allNotes(report));
    }
    html += moreRow("glossary", _("Glossary"), _("abbreviations, terms and what every test "
        "does"), glossary(report));
    html += moreRow("run", _("Run details"), _("target, intensity, temporary database"),
        runDetails(report));
    return html + "</div></div>\n";
}

// ---------------------------------------------------------------- two runs

// the parts in "What stands out"
const size_t maximumStandOut = 5;

wxString newerLabel(const BenchmarkComparison& c)
{
    return c.newerIsCurrentRun ? _("This run") : _("Later run");
}

wxString pointsOf(double ratio)
{
    return formatBenchmarkPoints(BenchmarkComparison::earlierPoints * ratio);
}

// faster, slower or about the same: a triangle and words, green and red;
// a clear change in a lighter weight than a large one
wxString direction(double ratio)
{
    const int level = getBenchmarkChangeLevel(ratio);
    const char* kind = level == 0 ? "same" : ratio > 1.0 ? "faster" : "slower";
    return wxString("<span class=\"dir ") + kind + (level == 1 ? " l1" : "") + "\">"
        + esc(formatBenchmarkChange(ratio)) + "</span>";
}

const char* directionText(double ratio)
{
    if (getBenchmarkChangeLevel(ratio) == 0)
        return "same-text";
    return ratio > 1.0 ? "up-text" : "down-text";
}

// the ratio as a bar from the middle, logarithmic up to four times
wxString ratioBar(double ratio)
{
    if (getBenchmarkChangeLevel(ratio) == 0)
        return "<span class=\"dbar\"></span>";
    const double share = std::min(std::abs(std::log2(ratio)) / 2.0, 1.0) * 50.0;
    return wxString("<span class=\"dbar\"><span class=\"fill ") + (ratio > 1.0 ? "faster"
        : "slower") + "\" style=\"width:" + num(share) + "%\"></span></span>";
}

// new, no longer measured, not comparable
wxString badge(BenchmarkCompareState state)
{
    const bool changed = state == BenchmarkCompareState::TestChanged
        || state == BenchmarkCompareState::WorkChanged;
    return wxString("<span class=\"badge") + (changed ? " nc" : "") + "\">"
        + esc(getBenchmarkCompareStateName(state)) + "</span>";
}

// the main statement: how many key figures got faster or slower, the
// points of both runs, and what could not be compared
wxString verdictBox(const BenchmarkComparison& c)
{
    std::vector<wxString> counts;
    for (const auto& count : describeBenchmarkVerdict(getBenchmarkVerdict(c)))
    {
        const char* cls = count.first > 0 ? "v-faster"
            : count.first < 0 ? "v-slower" : "v-same";
        counts.push_back(wxString("<span class=\"") + cls + "\">" + esc(count.second)
            + "</span>");
    }
    wxString html = "<div id=\"comparison\">";
    if (!counts.empty())
        html += "<p class=\"verdict\">" + join(counts, separator()) + "</p>";

    std::vector<wxString> notes;
    if (c.ratio > 0.0)
    {
        wxString points = esc(wxString::Format(_("Overall %s points; the earlier run has %s"),
            "@points@", pointsOf(1.0)));
        points.Replace("@points@", "<b>" + esc(pointsOf(c.ratio)) + "</b>");
        notes.push_back(points + tip(getBenchmarkUnitHint("points") + " "
            + _("Differences of less than 15 % are normal between two runs.")));
    }
    else
        notes.push_back(esc(describeBenchmarkComparison(c)));
    // the parts that one run did not measure, with their short word, and
    // the key figures that were not compared
    for (const wxString& note : getBenchmarkVerdictNotes(c))
    {
        wxString text = esc(note);
        for (BenchmarkCategory section : getBenchmarkReportSections())
        {
            const wxString word = esc(getBenchmarkReportSectionWord(section));
            if (text.StartsWith(word + " "))
            {
                text = "<span class=\"tag-word\">" + word + "</span>" + text.Mid(word.length());
                break;
            }
        }
        notes.push_back(text);
    }
    html += "<p class=\"verdict-sub\">" + join(notes, separator()) + "</p>";
    if (!c.sameKind)
    {
        html += "<p class=\"verdict-note\">" + icon("i-warn", "s", 16)
            + esc(_("The two runs were made in different ways (with or without a network, "
                "or with another intensity), so they are only roughly comparable.")) + "</p>";
    }
    return html + "</div>\n";
}

// one line per key figure: both values, the change and the points
wxString keyFigureRows(const BenchmarkComparison& c)
{
    const std::vector<BenchmarkHeadlineChange> changes = getBenchmarkHeadlineChanges(c);
    if (changes.empty())
        return wxEmptyString;
    const wxString older = _("Earlier run");
    const wxString newer = newerLabel(c);
    wxString rows;
    for (const BenchmarkHeadlineChange& h : changes)
    {
        const bool compared = h.state == BenchmarkCompareState::Compared && h.ratio > 0.0;
        wxString points = dash();
        wxString change = badge(h.state);
        wxString bar;
        if (compared)
        {
            const long delta = std::lround(h.ratio * BenchmarkComparison::earlierPoints)
                - long(BenchmarkComparison::earlierPoints);
            wxString deltaText = wxString::Format("%+ld", delta);
            if (delta == 0)
                deltaText = wxString(wxUniChar(0x00B1)) + "0";
            points = esc(pointsOf(h.ratio)) + "<span class=\"delta " + directionText(h.ratio)
                + "\">" + esc(deltaText) + "</span>";
            change = direction(h.ratio);
            bar = ratioBar(h.ratio);
        }
        rows += wxString("<tr><td class=\"tagcol ") + toneOf(h.section) + "\">"
            + sectionTag(h.section, symbolOf(h.id, h.section)) + "</td><td class=\"area\">"
            + esc(h.name) + "</td>" + cellLabel(older, "n old")
            + esc(h.olderNumber.empty() ? dash() : h.olderNumber) + "</td><td class=\"arrow\">"
            + wxUniChar(0x2192) + "</td>" + cellLabel(newer, "n new")
            + esc(h.newerNumber.empty() ? dash() : h.newerNumber) + "<span class=\"unit\">"
            + esc(h.unit) + "</span></td><td class=\"bar-col\">" + bar + "</td>"
            + cellLabel(_("Change"), "dir-col") + change + "</td>"
            + cellLabel(_("Points"), "n pts") + points + "</td></tr>";
    }
    return "<table class=\"cmp\" id=\"areas\"><thead><tr><th colspan=\"2\">"
        + esc(_("Key figure")) + "</th><th class=\"n\">" + esc(older) + "</th><th></th><th "
        "class=\"n\">" + esc(newer) + "</th><th class=\"bar-col\"></th><th>"
        + esc(_("Change")) + "</th><th class=\"n\">" + esc(_("Points")) + "</th></tr></thead>"
        "<tbody>" + rows + "</tbody></table>\n";
}

size_t reasonCount(const BenchmarkComparedPart& p)
{
    return p.differences.size() + p.hints.size() + p.advice.size();
}

wxString reasonCountText(size_t count)
{
    return wxString::Format(wxPLURAL("%d possible reason", "%d possible reasons", int(count)),
        int(count));
}

// the likely reason and what to try, one line each
wxString reasonLines(const BenchmarkComparedPart& p)
{
    wxString html;
    const wxString likely = getBenchmarkLikelyReason(p);
    if (!likely.empty())
    {
        html += "<span class=\"so-line\"><span class=\"so-key\">" + esc(_("Likely"))
            + "</span><span>" + esc(likely) + "</span></span>";
    }
    const wxString advice = getBenchmarkTryAdvice(p);
    if (!advice.empty())
    {
        html += "<span class=\"so-line\"><span class=\"so-key\">" + esc(_("What to try"))
            + "</span><span>" + esc(advice) + "</span></span>";
    }
    return html;
}

// every possible reason of a part: the facts that differ, the hints of the
// slower run with what was measured and what can help, and the typical
// causes of its slower values
wxString reasonList(const BenchmarkComparedPart& p, const BenchmarkComparison& c)
{
    wxString items;
    for (const auto& d : p.differences)
    {
        items += "<li><b>" + esc(d.label) + "</b>: " + esc(d.olderText + arrow() + d.newerText)
            + "</li>";
    }
    // two hints can give the same advice, e.g. the exclusions of a virus
    // scanner; it is shown once
    std::vector<wxString> shown;
    for (const auto& h : p.hints)
    {
        const bool repeated = std::find(shown.begin(), shown.end(), h.advice) != shown.end();
        shown.push_back(h.advice);
        items += "<li><b>" + esc(h.title) + "</b>" + (h.detail.empty() ? wxString()
            : ". " + esc(h.detail)) + (h.advice.empty() || repeated ? wxString()
            : " <span class=\"quiet\">" + esc(h.advice) + "</span>") + "</li>";
    }
    for (const auto& a : p.advice)
        items += "<li><b>" + esc(a.first) + "</b>: " + esc(a.second) + "</li>";
    if (items.empty())
        return wxEmptyString;
    return "<p class=\"reason-head\">" + esc(getBenchmarkReasonsTitle(c, p))
        + "</p><ul class=\"reason-list\">" + items + "</ul>";
}

// the parts that differ clearly, the largest change first, each with the
// likely reason and what to try; a click shows every possible reason
wxString standOutBox(const BenchmarkComparison& c)
{
    const std::vector<const BenchmarkComparedPart*> parts = getBenchmarkStandOut(c,
        maximumStandOut);
    if (parts.empty())
        return wxEmptyString;
    wxString html = sectionLabel(_("What stands out"), _("the clearest changes, possible "
        "reasons and what to try")) + "<div class=\"stand\" id=\"changes\">";
    for (const BenchmarkComparedPart* p : parts)
    {
        const wxString head = wxString("<span class=\"") + toneOf(p->category) + "\">"
            + sectionTag(p->category) + "</span><span class=\"so-dir\">" + direction(p->ratio)
            + "</span><span class=\"so-text\"><span class=\"so-part\">"
            + esc(getBenchmarkReportSectionName(p->category)) + "</span>";
        const size_t count = reasonCount(*p);
        if (count == 0)
        {
            html += "<div class=\"so\"><div class=\"so-head\">" + head + "<span class=\"so-line\">"
                + esc(_("No difference was found in the facts and notes of the two runs."))
                + "</span></span></div></div>";
            continue;
        }
        html += "<details class=\"so\"><summary>" + head + reasonLines(*p)
            + "</span><span class=\"so-more\">" + esc(reasonCountText(count))
            + "</span><span class=\"chev\"></span></summary><div class=\"so-body\">"
            + reasonList(*p, c) + "</div></details>";
    }
    return html + "</div>\n";
}

// hardware, versions and settings that differ between the runs
wxString differencesRow(const BenchmarkComparison& c)
{
    if (c.differences.empty())
    {
        return moreRow("differences", _("What is different"), _("hardware, versions and "
            "settings are the same"), "<p class=\"quiet\">" + esc(_("Hardware, versions and "
            "settings are the same.")) + "</p>");
    }
    const wxString older = _("Earlier run");
    const wxString newer = newerLabel(c);
    wxString html = tableStart({ { wxString(), false }, { older, false }, { newer, false } });
    for (const auto& d : c.differences)
    {
        html += "<tr><td>" + esc(d.label) + "</td>" + cellLabel(older) + esc(d.olderText)
            + "</td>" + cellLabel(newer) + esc(d.newerText) + "</td></tr>";
    }
    const int count = int(c.differences.size());
    return moreRow("differences", _("What is different"), wxString::Format(wxPLURAL(
        "%d difference in hardware, versions or settings",
        "%d differences in hardware, versions or settings", count), count),
        html + "</tbody></table>");
}

wxString comparisonOverview(const BenchmarkComparison& c)
{
    return "<div class=\"overview\">" + verdictBox(c) + keyFigureRows(c) + standOutBox(c)
        + "<div class=\"rows\">" + viewRow("v3", _("Detailed comparison"), _("every "
            "measured value of both runs, by part, with the possible reasons"))
        + differencesRow(c) + "</div></div>\n";
}

// every value of both runs by part, with a filter for the changes
wxString comparisonDetails(const BenchmarkComparison& c)
{
    using S = BenchmarkCompareState;
    const wxString older = _("Earlier run");
    const wxString newer = newerLabel(c);
    const wxString dot = wxString(" ") + wxUniChar(0x00B7) + " ";
    wxString areas;
    for (BenchmarkCategory section : getBenchmarkReportSections())
    {
        int count = 0, faster = 0, slower = 0, same = 0, added = 0, removed = 0, other = 0;
        bool unchanged = true;
        wxString rows;
        for (const auto* list : { &c.values, &c.stats })
        {
            for (const BenchmarkComparedValue& v : *list)
            {
                if (v.category != section)
                    continue;
                ++count;
                wxString change;
                bool changed = true;
                if (v.state == S::Compared && v.ratio > 0.0)
                {
                    change = direction(v.ratio);
                    changed = getBenchmarkChangeLevel(v.ratio) > 0;
                    if (!changed)
                        ++same;
                    else if (v.ratio > 1.0)
                        ++faster;
                    else
                        ++slower;
                }
                else if (v.state == S::Compared)
                    changed = v.olderText != v.newerText;
                else
                {
                    change = badge(v.state);
                    if (v.state == S::OnlyNewer)
                        ++added;
                    else if (v.state == S::OnlyOlder)
                        ++removed;
                    else
                        ++other;
                }
                unchanged = unchanged && !changed;
                const wxString note = describeBenchmarkCompareState(c, v);
                const wxString work = formatBenchmarkComparedWork(c, v);
                rows += wxString("<tr class=\"") + (changed ? "chg" : "same") + "\"><td>"
                    + esc(v.title) + (note.empty() ? wxString() : "<small>" + esc(note)
                        + "</small>") + "</td>" + cellLabel(older, "n") + esc(v.olderText)
                    + "</td>" + cellLabel(newer, "n") + "<b>" + esc(v.newerText) + "</b></td>"
                    + cellLabel(_("Change"), wxEmptyString, change) + change + "</td>"
                    + cellLabel(_("Work"), "quiet work", work) + esc(work) + "</td></tr>";
            }
        }
        if (count == 0)
            continue;

        // the head: the points of the part, how it changed and its values
        std::vector<wxString> counts;
        auto add = [&counts](int n, const wxString& text)
        {
            if (n > 0)
                counts.push_back(text);
        };
        add(faster, wxString::Format(_("%d faster"), faster));
        add(slower, wxString::Format(_("%d slower"), slower));
        add(same, wxString::Format(_("%d about the same"), same));
        add(added, wxString::Format(_("%d new"), added));
        add(removed, wxString::Format(_("%d no longer measured"), removed));
        add(other, wxString::Format(_("%d not comparable"), other));
        wxString line = esc(wxString::Format(wxPLURAL("%d value", "%d values", count), count)
            + (counts.empty() ? wxString() : ": " + join(counts, ", ")));
        const BenchmarkComparedPart* part = c.findPart(section);
        wxString number = dash();
        wxString unit;
        if (part && part->ratio > 0.0)
        {
            number = pointsOf(part->ratio);
            unit = _("pts");
            line = direction(part->ratio) + esc(dot) + line;
        }
        else if (getBenchmarkPartState(c, section) == S::OnlyNewer)
        {
            line = "<span class=\"badge\">" + esc(c.newerIsCurrentRun ? _("only in this run")
                : _("only in the later run")) + "</span>" + esc(dot) + line;
        }
        else if (getBenchmarkPartState(c, section) == S::OnlyOlder)
        {
            line = "<span class=\"badge\">" + esc(_("only in the earlier run")) + "</span>"
                + esc(dot) + line;
        }
        wxString body;
        if (part && reasonCount(*part) > 0)
        {
            body += "<div class=\"why-block\">" + reasonLines(*part) + "<details "
                "class=\"why-more\"><summary>" + esc(reasonCountText(reasonCount(*part)))
                + "<span class=\"chev\"></span></summary>" + reasonList(*part, c)
                + "</details></div>";
        }
        body += tableStart({ { _("Test"), false }, { older, true }, { newer, true },
            { _("Change"), false }, { _("Work"), false } }, "cards dt") + rows
            + "</tbody></table>";
        areas += wxString("<details class=\"area ") + toneOf(section)
            + (unchanged ? " unchanged" : "") + "\">" + figureRow("summary", section,
                sectionTag(section), number, unit, getBenchmarkReportSectionName(section), line,
                "<span class=\"chev\"></span>") + "<div class=\"area-body\"><div>" + body
            + "</div></div></details>\n";
    }
    if (areas.empty())
        return wxEmptyString;
    // radio buttons before the filter and the parts, as for the views
    return "<div class=\"cmp-details\"><input type=\"radio\" name=\"filter\" "
        "class=\"filter-toggle\" id=\"f1\" checked><input type=\"radio\" name=\"filter\" "
        "class=\"filter-toggle\" id=\"f2\">" + sectionLabel(_("Every measured value"),
            _("a click opens a part; the earlier run is on the left"))
        + "<div class=\"filter\"><label for=\"f1\">" + esc(_("All values"))
        + "</label><label for=\"f2\">" + esc(_("Only changes")) + "</label></div>"
        "<div class=\"areas\" id=\"all-values\">" + areas + "</div><p class=\"quiet "
        "legend-note\">" + badge(S::OnlyNewer) + " " + esc(c.newerIsCurrentRun
            ? _("only in this run") : _("only in the later run")) + esc(dot)
        + badge(S::OnlyOlder) + " " + esc(_("only in the earlier run")) + esc(dot)
        + badge(S::TestChanged) + " " + esc(_("the test or its work changed")) + esc(dot)
        + esc(_("the same work in less time is faster.")) + "</p></div>\n";
}

// "a" or "a -> b"
wxString both(const wxString& older, const wxString& newer)
{
    return older == newer ? older : older + arrow() + newer;
}

wxString intensityOf(const BenchmarkSnapshot& run)
{
    if (run.intensity == "quick")
        return getBenchmarkIntensityName(BenchmarkIntensity::Quick);
    if (run.intensity == "deep")
        return getBenchmarkIntensityName(BenchmarkIntensity::Deep);
    if (run.intensity == "standard")
        return getBenchmarkIntensityName(BenchmarkIntensity::Standard);
    return run.intensity;
}

// the date of a run without the seconds, and its computer
wxString runChip(const BenchmarkSnapshot& run, const wxString& label)
{
    return chip("i-date", "<b>" + esc(label) + "</b> " + esc(run.date.Left(16)
        + (run.host.empty() ? wxString() : wxString(" ") + wxUniChar(0x00B7) + " " + run.host)));
}

std::map<wxString, wxString> readCssTokens(const wxString& css, const wxString& selector)
{
    std::map<wxString, wxString> tokens;
    size_t start = css.find(selector + " {");
    if (start == wxString::npos)
        return tokens;
    start += selector.length() + 2;
    const size_t end = css.find('}', start);
    for (const wxString& declaration : wxSplit(css.substr(start, end - start), ';', '\0'))
    {
        wxString name = declaration.BeforeFirst(':');
        name.Trim().Trim(false);
        wxString value = declaration.AfterFirst(':');
        value.Trim().Trim(false);
        if (name.StartsWith("--") && !value.empty())
            tokens[name] = value;
    }
    return tokens;
}

wxString cssProperty(const wxString& declaration)
{
    wxString property = declaration.BeforeFirst(':');
    property.Trim().Trim(false);
    return property;
}

// the declaration that Internet Explorer 11 understands instead of this
// one, or nothing
wxString legacyDeclaration(const wxString& declaration,
    const std::map<wxString, wxString>& tokens)
{
    const wxString property = cssProperty(declaration);
    wxString value = declaration.AfterFirst(':');
    value.Trim().Trim(false);
    if (property.empty() || property.StartsWith("--") || value.empty()
        || property.find_first_not_of("abcdefghijklmnopqrstuvwxyz-") != wxString::npos)
    {
        return wxString();
    }
    if (property == "display" && value == "grid")
        return "display: block";
    if (property == "display" && value == "inline-grid")
        return "display: inline-block";
    if (property == "overflow-wrap" && value == "anywhere")
        return "word-wrap: break-word";
    if (!value.Contains("var(--"))
        return wxString();
    for (size_t pos = value.find("var(--"); pos != wxString::npos; pos = value.find("var(--"))
    {
        const size_t close = value.find(')', pos);
        if (close == wxString::npos)
            return wxString();
        wxString name = value.substr(pos + 4, close - pos - 4).BeforeFirst(',');
        name.Trim();
        auto it = tokens.find(name);
        if (it == tokens.end())
            return wxString();
        value = value.substr(0, pos) + it->second + value.substr(close + 1);
    }
    return property + ": " + value;
}

// the start of a page: head, styles and the opened <main>
wxString pageStart(const wxString& theme)
{
    wxString html = "<!doctype html>\n<html lang=\"en\"";
    if (theme == "dark" || theme == "light")
        html += " data-theme=\"" + theme + "\"";
    // X-UA-Compatible has to come first: without it, the Internet Explorer
    // control and intranet pages use the mode of Internet Explorer 7
    return html + "><head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">"
        "<meta charset=\"utf-8\"><meta name=\"viewport\" "
        "content=\"width=device-width, initial-scale=1\"><title>"
        + esc(_("Performance Benchmark & Diagnosis")) + "</title><style>"
        + addBenchmarkCssFallbacks(wxString::FromUTF8(styleSheet), theme == "dark")
        + "</style></head><body><main>\n";
}

// replaces the private texts that anonymized() could not replace in a field
// of its own, then adds the pictograms, so that a name never changes them
wxString finishPage(const wxString& html,
    const std::vector<std::pair<wxString, wxString>>& hiddenTexts)
{
    wxString page = hideBenchmarkTexts(html, hiddenTexts, true);
    const size_t body = page.find("<body>");
    if (body != wxString::npos)
        page.insert(body + 6, sprite());
    return page;
}

// the radio buttons of the views: Overview (v1), Detailed report (v2),
// Detailed comparison (v3); the overview is shown first
wxString viewToggles(bool report, bool comparison)
{
    wxString html = "<input type=\"radio\" name=\"view\" class=\"view-toggle\" id=\"v1\" "
        "checked>";
    if (report)
        html += "<input type=\"radio\" name=\"view\" class=\"view-toggle\" id=\"v2\">";
    if (comparison)
        html += "<input type=\"radio\" name=\"view\" class=\"view-toggle\" id=\"v3\">";
    return html + "\n";
}

// the navigation of the views; no <nav>, the mode of Internet Explorer 8
// cannot style it
wxString viewTabs(bool report, bool comparison)
{
    wxString html = "<div class=\"pages\"><label for=\"v1\">" + esc(_("Overview")) + "</label>";
    if (comparison)
        html += "<label for=\"v3\">" + esc(_("Detailed comparison")) + "</label>";
    if (report)
        html += "<label for=\"v2\">" + esc(_("Detailed report")) + "</label>";
    return html + "</div>\n";
}

} // namespace

wxString addBenchmarkCssFallbacks(const wxString& css, bool dark)
{
    std::map<wxString, wxString> tokens = readCssTokens(css, ":root");
    if (dark)
    {
        for (const auto& token : readCssTokens(css, ":root[data-theme=\"dark\"]"))
            tokens[token.first] = token.second;
    }

    wxString result;
    size_t pos = 0;
    for (size_t open = css.find('{'); open != wxString::npos; open = css.find('{', pos))
    {
        const size_t next = css.find_first_of("{}", open + 1);
        if (next == wxString::npos)
            break;
        result += css.substr(pos, open + 1 - pos);
        pos = open + 1;
        // the rules of @media follow
        if (css[next] == '{')
            continue;
        // a block of declarations; a pair written for Internet Explorer
        // already has its own fallback
        const wxArrayString declarations = wxSplit(css.substr(pos, next - pos), ';', '\0');
        wxString previous;
        for (size_t i = 0; i < declarations.size(); ++i)
        {
            const wxString& declaration = declarations[i];
            const wxString property = cssProperty(declaration);
            const wxString fallback = property == previous ? wxString()
                : legacyDeclaration(declaration, tokens);
            if (fallback.empty())
                result += declaration;
            else
            {
                const size_t start = declaration.find_first_not_of(" \t\r\n");
                result += declaration.substr(0, start) + fallback + "; "
                    + declaration.substr(start);
            }
            if (i + 1 < declarations.size())
                result += ";";
            previous = property;
        }
        pos = next;
    }
    return result + css.substr(pos);
}

wxString BenchmarkComparison::toHtml(const wxString& theme) const
{
    const wxString dot = wxString(" ") + wxUniChar(0x00B7) + " ";
    wxString html = pageStart(theme) + viewToggles(false, true);
    html += "<header><div class=\"brand\">" + esc("FlameRobin" + dot
        + _("Performance Benchmark & Diagnosis")) + "</div><h1>"
        + esc(_("Comparison of two benchmark runs")) + "</h1><p class=\"specs\">"
        + esc(both(older.title, newer.title) + dot + both(getBenchmarkKindName(older.kind),
            getBenchmarkKindName(newer.kind)) + dot + both(intensityOf(older),
            intensityOf(newer))) + "</p><div class=\"runs\">"
        + runChip(older, _("Earlier run")) + "<span class=\"to\">" + wxUniChar(0x2192)
        + "</span>" + runChip(newer, newerLabel(*this)) + "</div></header>\n";
    html += viewTabs(false, true) + comparisonOverview(*this) + comparisonDetails(*this);
    html += "<footer>" + esc(_("Created by FlameRobin from two saved benchmark reports."))
        + "</footer>\n" + getBenchmarkComparisonMarker() + "</main></body></html>\n";
    return finishPage(html, hiddenTexts);
}

wxString BenchmarkReport::toHtml(const wxString& theme) const
{
    const std::vector<Section> sections = collectSections(*this);
    const BenchmarkSnapshot snapshot = makeBenchmarkSnapshot(*this);
    std::optional<BenchmarkComparison> comparison;
    if (earlier)
    {
        comparison = compareBenchmarkSnapshots(snapshot, *earlier);
        comparison->newerIsCurrentRun = true;
    }
    wxString html = pageStart(theme) + viewToggles(true, comparison.has_value());

    // header: the tested system, its specification and the run
    const wxString dot = wxString(" ") + wxUniChar(0x00B7) + " ";
    html += "<header><div class=\"brand\">" + esc("FlameRobin" + dot
        + _("Performance Benchmark & Diagnosis")) + "</div><h1>"
        + esc(getBenchmarkSystemTitle(*this)) + "</h1><p class=\"specs\">"
        + esc(getBenchmarkSystemSpecs(*this)) + "</p><div class=\"chips\">";
    if (!metrics.localHostName.empty())
        html += chip("i-app", esc(metrics.localHostName));
    html += chip("i-date", esc(startedAt.Left(16))) + chip("i-load", esc(intensityName))
        + chip("i-time", esc(wxString::Format(_("%.0f s"), durationSeconds)))
        + chip("i-link", esc(getBenchmarkKindName(getBenchmarkKindId(metrics))));
    if (!comparison)
    {
        html += askChip(_("Compare with another run"), _("Choose this result or "
            "report under \"Compare with\" before the next run, or use \"Compare...\". The "
            "comparison shows what got faster or slower and what could be the reason."));
    }
    html += "</div>";
    if (comparison)
    {
        html += "<p class=\"lead\">" + esc(wxString::Format(_("Compared with the earlier run "
            "%s of %s."), earlier->title, earlier->date)) + "</p>";
    }
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
    html += "</header>\n" + viewTabs(true, comparison.has_value());

    // the overview: the key figures and the notes of this run, or the
    // comparison with an earlier run and every value of both
    if (comparison)
        html += comparisonOverview(*comparison) + comparisonDetails(*comparison);
    else
        html += singleOverview(*this, sections, snapshot);
    html += detailsView(*this, sections);

    html += "<footer>" + esc(_("Created by FlameRobin. All tests ran in a temporary "
        "database that was created for the benchmark and dropped afterwards; no other "
        "database was changed.")) + "</footer>\n</main>\n"
        // the results for later comparisons, see parseBenchmarkSnapshot()
        + formatBenchmarkSnapshot(snapshot) + "</body></html>\n";
    return finishPage(html, hiddenTexts);
}

} // namespace fr
