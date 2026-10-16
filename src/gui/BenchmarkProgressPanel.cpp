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

#include <wx/bmpbndl.h>
#include <wx/dcbuffer.h>
#include <wx/settings.h>
#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif
#ifdef __WXGTK__
#include <wx/dynlib.h>
#endif

#include <algorithm>
#include <cmath>

#include "gui/BenchmarkProgressPanel.h"

#ifndef SPI_GETCLIENTAREAANIMATION
#define SPI_GETCLIENTAREAANIMATION 0x1042
#endif

using fr::BenchmarkStage;
using fr::BenchmarkStageState;

wxColour mixBenchmarkColours(const wxColour& first, const wxColour& second, double share)
{
    return wxColour(wxColour::AlphaBlend(first.Red(), second.Red(), share),
        wxColour::AlphaBlend(first.Green(), second.Green(), share),
        wxColour::AlphaBlend(first.Blue(), second.Blue(), share));
}

double getBenchmarkContrast(const wxColour& first, const wxColour& second)
{
    // the relative luminance of WCAG 2
    auto luminance = [](const wxColour& colour)
    {
        auto channel = [](unsigned char value)
        {
            const double share = value / 255.0;
            return share <= 0.03928 ? share / 12.92 : std::pow((share + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * channel(colour.Red()) + 0.7152 * channel(colour.Green())
            + 0.0722 * channel(colour.Blue());
    };
    const double a = luminance(first);
    const double b = luminance(second);
    return (std::max(a, b) + 0.05) / (std::min(a, b) + 0.05);
}

wxColour getReadableBenchmarkColour(const wxColour& colour, const wxColour& text,
    const wxColour& background, double contrast)
{
    // a theme may have colours that are too pale for its background
    for (int step = 0; step < 10; ++step)
    {
        const wxColour mixed = mixBenchmarkColours(text, colour, step / 10.0);
        if (getBenchmarkContrast(mixed, background) >= contrast)
            return mixed;
    }
    return text;
}

namespace
{

const double pi = 3.14159265358979323846;

// the FlameRobin logo, res/svg/flamerobin.svg (Stefano Pennuto, 2006): the
// paths unchanged, only their colours follow the progress
const char* const logoBody =
    "M 52.478 5.215 C 50.397 10.372 49.739 16.128 49.739 21.997 "
    "C 49.739 43.289 58.836 61.435 80.631 69.649 "
    "C 103.756 79.119 118.836 96.125 118.836 120.163 "
    "C 118.836 149.157 95.332 172.663 66.336 172.663 "
    "C 66.052 172.663 65.773 172.646 65.490 172.642 L 65.486 172.663 "
    "C -13.013 172.372 -16.513 23.872 52.478 5.215 Z";
const char* const logoInner =
    "M 46.728 108.688 C 48.261 109.821 49.488 111.384 50.542 113.098 "
    "C 54.367 119.315 53.923 126.891 50.080 132.561 "
    "C 44.761 138.337 43.680 147.153 47.998 154.173 "
    "C 53.205 162.639 64.287 165.280 72.749 160.073 "
    "C 72.833 160.022 72.911 159.967 72.993 159.914 L 72.998 159.921 "
    "C 95.869 145.735 70.217 101.745 46.728 108.688 Z";
const char* const logoBeak =
    "M 122.029 55.267 C 121.908 48.825 120.707 42.643 118.578 36.904 "
    "C 118.068 35.529 117.500 34.183 116.888 32.861 L 116.885 32.849 "
    "L 135.586 40.206 L 122.040 55.314 L 122.029 55.267 Z";
// the view box, with a margin of 3 units for the outline of the empty body
const double logoX = -3.0, logoY = -3.0, logoW = 141.586, logoH = 182.174;
// the body fills from its lowest to its highest point
const double bodyTop = 5.2, bodyBottom = 172.7;
const int logoHeightDip = 104;
// the fill levels; the pictures change only with the level
const int logoLevels = 40;
// the logo is rasterized this many times larger and reduced, which gives
// smoother edges than the rasterizer alone
const int logoOversampling = 4;
// the pictures of a level: the wave in eight positions of the edge (two
// seconds at 4 pictures a second), the first of them again with the eye
// closed, the breath in four heights of the filling (four seconds at 1
// picture a second), and the still picture at the exact level
const int wavePhases = 8;
const int blinkFrame = wavePhases;
const int breathFrames = 4;
const int firstBreathFrame = blinkFrame + 1;
const int stillFrame = firstBreathFrame + breathFrames;
const int logoFrames = stillFrame + 1;
// the height of the wave and how far the filling rises with the breath, in
// DIP; below a pixel the edge moves by its shading only
const double waveAmplitudeDip = 1.5;
const double breathRiseDip = 2.0;
// with the breath the inner flame glows up to this share (the pale one
// towards its yellow, the full one towards white), so that the breath shows
// also where the edge crosses only the tip of the body
const double breathGlow = 0.45;

// the scale of the pixels of a bitmap to the coordinates of the window:
// GTK 3 and macOS draw in logical pixels, MSW in physical ones
double getBitmapScale(const wxWindow* window)
{
#ifdef wxHAS_DPI_INDEPENDENT_PIXELS
    return window->GetDPIScaleFactor();
#else
    wxUnusedVar(window);
    return 1.0;
#endif
}

wxBitmap toBitmap(const wxImage& image, double scale)
{
    wxBitmap bitmap(image);
#ifdef wxHAS_DPI_INDEPENDENT_PIXELS
    bitmap.SetScaleFactor(scale);
#else
    wxUnusedVar(scale);
#endif
    return bitmap;
}

struct Palette
{
    bool dark = false;
    wxColour background, text, muted, line, ring;
    // the stages: running orange, done green (a traffic light), failed red
    wxColour running, done, failed, failedText;
    // the logo: red, black and yellow, the empty body pale
    wxColour red, yellow, head, white, beak, emptyBody, emptyLine, emptyInner;
};

Palette getPalette()
{
    Palette p;
    p.dark = wxSystemSettings::GetAppearance().IsUsingDarkBackground();
    p.background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    p.text = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    p.muted = getReadableBenchmarkColour(mixBenchmarkColours(p.text, p.background, 0.6),
        p.text, p.background);
    p.line = mixBenchmarkColours(p.text, p.background, 0.15);
    // the marks are graphics that must be seen: 3:1 (WCAG 1.4.11); the
    // colours of the report (Entwurf v5) where the theme lets them
    auto mark = [&p](unsigned char r, unsigned char g, unsigned char b)
    {
        return getReadableBenchmarkColour(wxColour(r, g, b), p.text, p.background, 3.0);
    };
    p.ring = getReadableBenchmarkColour(mixBenchmarkColours(p.text, p.background, 0.4),
        p.text, p.background, 3.0);
    p.running = p.dark ? mark(0xf0, 0xa0, 0x4b) : mark(0xe0, 0x70, 0x00);
    p.done = p.dark ? mark(0x68, 0xd3, 0x91) : mark(0x2f, 0x85, 0x5a);
    p.failed = p.dark ? mark(0xfc, 0x81, 0x81) : mark(0xc5, 0x30, 0x30);
    p.failedText = getReadableBenchmarkColour(p.failed, p.text, p.background);
    p.red = wxColour(0xff, 0x00, 0x00);
    p.yellow = wxColour(0xff, 0xff, 0x00);
    p.head = wxColour(0x00, 0x00, 0x00);
    p.white = wxColour(0xff, 0xff, 0xff);
    // a black beak disappears on a dark background
    p.beak = p.dark ? wxColour(0xe6, 0xe6, 0xe6) : p.head;
    p.emptyBody = mixBenchmarkColours(p.red, p.background, p.dark ? 0.22 : 0.12);
    p.emptyLine = mixBenchmarkColours(p.red, p.background, 0.45);
    p.emptyInner = mixBenchmarkColours(p.yellow, p.background, p.dark ? 0.25 : 0.3);
    return p;
}

// the words of the stages in the colours of the parts of the report (Entwurf
// v5), the first and the last in the colour of the text: grey would look
// disabled
wxColour getStageColour(BenchmarkStage stage, const Palette& p)
{
    wxColour colour;
    switch (stage)
    {
        case BenchmarkStage::Cpu:
            colour = p.dark ? wxColour(0x86, 0x72, 0xe6) : wxColour(0x6a, 0x4f, 0xd0);
            break;
        case BenchmarkStage::Disk:
        case BenchmarkStage::PageSizes:
            colour = p.dark ? wxColour(0x16, 0xa0, 0x8f) : wxColour(0x00, 0x89, 0x7b);
            break;
        case BenchmarkStage::Memory:
            colour = p.dark ? wxColour(0xc4, 0x86, 0x1a) : wxColour(0xb0, 0x6f, 0x00);
            break;
        case BenchmarkStage::Connect:
        case BenchmarkStage::Network:
            colour = p.dark ? wxColour(0xdc, 0x5a, 0x95) : wxColour(0xc0, 0x39, 0x7b);
            break;
        case BenchmarkStage::Users:
            colour = p.dark ? wxColour(0x38, 0x96, 0xdc) : wxColour(0x1d, 0x7f, 0xc0);
            break;
        default:
            return p.text;
    }
    return getReadableBenchmarkColour(colour, p.text, p.background);
}

wxString svgColour(const wxColour& colour)
{
    return colour.GetAsString(wxC2S_HTML_SYNTAX);
}

// an SVG text as an image of this size in pixels, with alpha
wxImage renderSvg(const wxString& svg, const wxSize& size)
{
    const wxScopedCharBuffer utf8 = svg.utf8_str();
    wxImage image = wxBitmapBundle::FromSVG(utf8.data(), size).GetBitmap(size)
        .ConvertToImage();
    if (!image.HasAlpha())
        image.InitAlpha();
    return image;
}

// the image reduced by an integer factor, every pixel the mean of its
// square weighted by alpha, so that the edges do not get dark
wxImage reduce(const wxImage& big, int factor)
{
    const int w = big.GetWidth() / factor, h = big.GetHeight() / factor;
    wxImage small(w, h);
    small.InitAlpha();
    const unsigned char* rgb = big.GetData();
    const unsigned char* alpha = big.GetAlpha();
    unsigned char* out = small.GetData();
    unsigned char* outAlpha = small.GetAlpha();
    const int bigWidth = big.GetWidth();
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            double sum[3] = { 0, 0, 0 };
            double weight = 0;
            for (int dy = 0; dy < factor; ++dy)
            {
                for (int dx = 0; dx < factor; ++dx)
                {
                    const int i = (y * factor + dy) * bigWidth + x * factor + dx;
                    const double a = alpha[i];
                    weight += a;
                    for (int k = 0; k < 3; ++k)
                        sum[k] += rgb[3 * i + k] * a;
                }
            }
            const int o = y * w + x;
            outAlpha[o] = static_cast<unsigned char>(std::lround(weight / (factor * factor)));
            for (int k = 0; k < 3; ++k)
            {
                out[3 * o + k] = weight > 0
                    ? static_cast<unsigned char>(std::lround(sum[k] / weight)) : 0;
            }
        }
    }
    return small;
}

wxString logoSvg(const Palette& p, bool full, bool blink, const wxSize& size)
{
    const wxString body = svgColour(full ? p.red : p.emptyBody);
    const wxString line = svgColour(full ? p.red : p.emptyLine);
    const wxString inner = svgColour(full ? p.yellow : p.emptyInner);
    wxString svg = wxString::Format("<svg xmlns='http://www.w3.org/2000/svg' width='%d' "
        "height='%d' viewBox='%s %s %s %s'>", size.x, size.y,
        wxString::FromCDouble(logoX, 3), wxString::FromCDouble(logoY, 3),
        wxString::FromCDouble(logoW, 3), wxString::FromCDouble(logoH, 3));
    svg << "<circle cx='66.336' cy='56.332' r='52.5' fill='" << svgColour(p.head) << "'/>"
        << "<circle cx='66.336' cy='56.332' r='52.5' fill='none' stroke='"
        << svgColour(p.white) << "' stroke-width='7'/>"
        << "<path fill='" << body << "' stroke='" << line << "' stroke-width='3' "
        << "stroke-linejoin='round' d='" << logoBody << "'/>";
    if (blink)
        svg << "<ellipse cx='90.908' cy='37.2' rx='6.2' ry='1.5' fill='" << svgColour(p.white)
            << "'/>";
    else
        svg << "<circle cx='90.908' cy='36.478' r='5.666' fill='" << svgColour(p.white) << "'/>";
    svg << "<path fill='" << inner << "' d='" << logoInner << "'/>"
        << "<path fill='" << svgColour(p.beak) << "' d='" << logoBeak << "'/></svg>";
    return svg;
}

wxImage renderLogo(const Palette& p, bool full, bool blink, const wxSize& size)
{
    const wxSize big(size.x * logoOversampling, size.y * logoOversampling);
    return reduce(renderSvg(logoSvg(p, full, blink, big), big), logoOversampling);
}

// where the inner flame is, as the alpha of the image
wxImage renderInnerFlame(const wxSize& size)
{
    const wxSize big(size.x * logoOversampling, size.y * logoOversampling);
    wxString svg = wxString::Format("<svg xmlns='http://www.w3.org/2000/svg' width='%d' "
        "height='%d' viewBox='%s %s %s %s'>", big.x, big.y,
        wxString::FromCDouble(logoX, 3), wxString::FromCDouble(logoY, 3),
        wxString::FromCDouble(logoW, 3), wxString::FromCDouble(logoH, 3));
    svg << "<path fill='#ffffff' d='" << logoInner << "'/></svg>";
    return reduce(renderSvg(svg, big), logoOversampling);
}

} // namespace

///
/// The logo, which fills from the bottom: a picture of the current level is
/// made when it is first shown, after that it is only drawn (one bitmap,
/// without alpha, on the background), and only when the picture changes.
///
class BenchmarkLogo : public wxWindow
{
public:
    explicit BenchmarkLogo(wxWindow* parent)
        : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &BenchmarkLogo::OnPaint, this);
        createImages();
    }

    // for the DPI of the window, at the start and when it changes
    void createImages()
    {
        paletteM = getPalette();
        scaleM = getBitmapScale(this);
        const int height = FromDIP(logoHeightDip);
        logicalSizeM = wxSize(int(std::lround(height * logoW / logoH)), height);
        pixelSizeM = wxSize(int(std::lround(logicalSizeM.x * scaleM)),
            int(std::lround(logicalSizeM.y * scaleM)));
        fullM = renderLogo(paletteM, true, false, pixelSizeM);
        fullBlinkM = renderLogo(paletteM, true, true, pixelSizeM);
        emptyM = renderLogo(paletteM, false, false, pixelSizeM);
        emptyBlinkM = renderLogo(paletteM, false, true, pixelSizeM);
        innerM = renderInnerFlame(pixelSizeM);
        SetMinSize(logicalSizeM);
        const int level = levelM;
        levelM = -1;
        setLevel(std::max(level, 0), mutedM);
    }

    // the pictures of a fill level (0 to logoLevels); grey after a cancel or
    // an error
    void setLevel(int level, bool muted)
    {
        if (level == levelM && muted == mutedM)
            return;
        levelM = level;
        mutedM = muted;
        for (wxBitmap& frame : framesM)
            frame = wxBitmap();
        Refresh(false);
    }

    // a picture of the level, see stillFrame; drawn only when it changes
    void showFrame(int frame)
    {
        if (frame == frameM)
            return;
        frameM = frame;
        Refresh(false);
    }
    int getFrame() const { return frameM; }

    // a picture of the level, made when it is first needed
    wxBitmap getFrameBitmap(int frame)
    {
        if (!framesM[frame].IsOk())
            framesM[frame] = makeFrame(frame);
        return framesM[frame];
    }

protected:
    wxSize DoGetBestClientSize() const override
    {
        return logicalSizeM;
    }

private:
    Palette paletteM;
    double scaleM = 1.0;
    wxSize logicalSizeM, pixelSizeM;
    wxImage fullM, fullBlinkM, emptyM, emptyBlinkM, innerM;
    wxBitmap framesM[logoFrames];
    int levelM = -1;
    bool mutedM = false;
    int frameM = stillFrame;

    // below the edge the logo in its colours, above it the empty one; the
    // edge is smooth (a pixel on it is shaded by its share below the edge)
    // and has a small wave
    wxBitmap makeFrame(int frame) const
    {
        const int w = pixelSizeM.x, h = pixelSizeM.y;
        // the pixels of the bitmap per DIP
        const double dip = double(h) / logoHeightDip;
        const double unit = h / logoH;
        const double top = (bodyTop - logoY) * unit, bottom = (bodyBottom - logoY) * unit;
        const double share = double(levelM) / logoLevels;
        double edge = bottom - share * (bottom - top);
        const bool partly = levelM > 0 && levelM < logoLevels;
        const double amplitude = (frame != stillFrame || partly) ? waveAmplitudeDip * dip : 0.0;
        double phase = 0.0;
        if (frame < wavePhases)
            phase = frame * 2 * pi / wavePhases;
        if (frame != stillFrame)
        {
            // the movement also shows when the logo is empty or full
            const double margin = (waveAmplitudeDip + breathRiseDip) * dip;
            edge = std::clamp(edge, top + margin, bottom - margin);
        }
        double glow = 0.0;
        if (frame >= firstBreathFrame && frame < stillFrame)
        {
            // the filling rises and sinks again, the inner flame glows
            const int step = frame - firstBreathFrame;
            const double breath = (1 - std::cos(2 * pi * step / breathFrames)) / 2;
            edge -= breathRiseDip * dip * breath;
            glow = breathGlow * breath;
        }
        const unsigned char* innerAlpha = innerM.GetAlpha();
        const double glowColour[3] = { double(paletteM.yellow.Red()),
            double(paletteM.yellow.Green()), double(paletteM.yellow.Blue()) };
        const wxColour bg = paletteM.background;
        const wxImage& full = frame == blinkFrame ? fullBlinkM : fullM;
        const wxImage& empty = frame == blinkFrame ? emptyBlinkM : emptyM;
        wxImage out(w, h);
        const unsigned char* fullRgb = full.GetData();
        const unsigned char* fullAlpha = full.GetAlpha();
        const unsigned char* emptyRgb = empty.GetData();
        const unsigned char* emptyAlpha = empty.GetAlpha();
        unsigned char* rgb = out.GetData();
        for (int x = 0; x < w; ++x)
        {
            // the wave runs from left to right
            const double line = edge + amplitude * std::sin(2 * pi * x / (w * 0.7) - phase);
            for (int y = 0; y < h; ++y)
            {
                // the share of the pixel below the edge
                const double below = std::clamp(y + 1 - line, 0.0, 1.0);
                const int i = y * w + x;
                const double a1 = emptyAlpha[i] / 255.0 * (1 - below);
                const double a2 = fullAlpha[i] / 255.0 * below;
                const double a = a1 + a2;
                for (int k = 0; k < 3; ++k)
                {
                    // the glow: the pale inner flame towards its yellow, the
                    // full one towards white
                    const double light = glow * innerAlpha[i] / 255.0;
                    const double pale = emptyRgb[3 * i + k]
                        + (glowColour[k] - emptyRgb[3 * i + k]) * light;
                    const double bright = fullRgb[3 * i + k] + (255 - fullRgb[3 * i + k]) * light;
                    const double colour = a > 0 ? (pale * a1 + bright * a2) / a : 0.0;
                    const unsigned char back = k == 0 ? bg.Red() : k == 1 ? bg.Green()
                        : bg.Blue();
                    const double value = colour * a + back * (1 - a);
                    rgb[3 * i + k] = static_cast<unsigned char>(std::lround(
                        std::clamp(value, 0.0, 255.0)));
                }
                if (mutedM)
                {
                    // grey, a little towards the background
                    unsigned char* px = rgb + 3 * i;
                    const double grey = 0.30 * px[0] + 0.59 * px[1] + 0.11 * px[2];
                    const double toBackground = paletteM.dark ? 0.2 : 0.35;
                    px[0] = static_cast<unsigned char>(grey * (1 - toBackground)
                        + bg.Red() * toBackground);
                    px[1] = static_cast<unsigned char>(grey * (1 - toBackground)
                        + bg.Green() * toBackground);
                    px[2] = static_cast<unsigned char>(grey * (1 - toBackground)
                        + bg.Blue() * toBackground);
                }
            }
        }
        return toBitmap(out, scaleM);
    }

    void OnPaint(wxPaintEvent& WXUNUSED(event))
    {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(paletteM.background));
        dc.Clear();
        if (levelM >= 0)
            dc.DrawBitmap(getFrameBitmap(frameM), 0, 0, false);
    }
};

namespace
{

struct StripItem
{
    wxString word;
    // the name of the running stage, or its state at the end
    wxString detail;
    wxColour colour;
    BenchmarkStageState state = BenchmarkStageState::Waiting;
    // the share of the stage that is done, for the line to the next one
    double fraction = 0.0;
};

} // namespace

///
/// The stages in a row: a mark of its own shape for every state and the
/// word below it. Drawn when a stage changes, not by the timer: the shapes
/// from SVG into a bitmap (smooth also with MSW GDI), the words with wxDC.
///
class BenchmarkStageStrip : public wxWindow
{
public:
    explicit BenchmarkStageStrip(wxWindow* parent)
        : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        paletteM = getPalette();
        Bind(wxEVT_PAINT, &BenchmarkStageStrip::OnPaint, this);
        Bind(wxEVT_SIZE, [this](wxSizeEvent& event)
        {
            shapesM = wxBitmap();
            Refresh(false);
            event.Skip();
        });
    }

    void setItems(const std::vector<StripItem>& items)
    {
        // most messages of the runner change no stage
        if (std::equal(items.begin(), items.end(), itemsM.begin(), itemsM.end(),
            [](const StripItem& a, const StripItem& b)
            {
                return a.word == b.word && a.detail == b.detail && a.colour == b.colour
                    && a.state == b.state && std::abs(a.fraction - b.fraction) < 0.005;
            }))
        {
            return;
        }
        const bool count = items.size() != itemsM.size();
        itemsM = items;
        shapesM = wxBitmap();
        if (count)
            InvalidateBestSize();
        Refresh(false);
    }

    void resetPictures()
    {
        paletteM = getPalette();
        shapesM = wxBitmap();
        InvalidateBestSize();
        Refresh(false);
    }

protected:
    wxSize DoGetBestClientSize() const override
    {
        const wxFont bold = GetFont().Bold();
        int wordWidth = 0, wordHeight = 0, detailHeight = 0;
        for (const StripItem& item : itemsM)
        {
            int w = 0, h = 0;
            GetTextExtent(item.word, &w, &h, nullptr, nullptr, &bold);
            wordWidth = std::max(wordWidth, w);
            wordHeight = std::max(wordHeight, h);
        }
        GetTextExtent("Ag", nullptr, &detailHeight);
        if (wordHeight == 0)
            GetTextExtent("Ag", nullptr, &wordHeight, nullptr, nullptr, &bold);
        const int n = std::max<int>(1, int(itemsM.size()));
        return wxSize(n * (wordWidth + FromDIP(8)), shapeHeight() + FromDIP(4) + wordHeight
            + FromDIP(2) + detailHeight);
    }

private:
    Palette paletteM;
    std::vector<StripItem> itemsM;
    std::vector<int> centersM;
    wxBitmap shapesM;

    int radius() const { return FromDIP(9); }
    int shapeHeight() const { return 2 * radius() + 2 * FromDIP(2); }

    // the marks and the lines between them as SVG in window coordinates
    wxString shapes(int width, const wxSize& pixels) const
    {
        const Palette& p = paletteM;
        const double r = radius(), cy = FromDIP(2) + r;
        const double k = r / 9.0;   // the marks are drawn for a radius of 9
        const double stroke = 2.0 * k;
        wxString svg = wxString::Format("<svg xmlns='http://www.w3.org/2000/svg' width='%d' "
            "height='%d' viewBox='0 0 %d %d'>", pixels.x, pixels.y, width, shapeHeight());
        auto line = [&](double from, double to, const wxColour& colour)
        {
            if (to > from)
            {
                svg << "<path d='M" << wxString::FromCDouble(from, 1) << " "
                    << wxString::FromCDouble(cy, 1) << "H" << wxString::FromCDouble(to, 1)
                    << "' stroke='" << svgColour(colour) << "' stroke-width='"
                    << wxString::FromCDouble(stroke, 1) << "' stroke-linecap='round'/>";
            }
        };
        // the line to the next stage, as far as the stage is done
        for (size_t i = 0; i + 1 < itemsM.size(); ++i)
        {
            const double from = centersM[i] + r + FromDIP(4);
            const double to = centersM[i + 1] - r - FromDIP(4);
            line(from, to, p.line);
            line(from, from + (to - from) * itemsM[i].fraction, p.muted);
        }
        for (size_t i = 0; i < itemsM.size(); ++i)
        {
            const double cx = centersM[i];
            auto at = [&](double dx, double dy)
            {
                return wxString::FromCDouble(cx + dx * k, 2) + " "
                    + wxString::FromCDouble(cy + dy * k, 2);
            };
            auto circle = [&](double size, const wxColour& fill, const wxColour& outline)
            {
                svg << "<circle cx='" << wxString::FromCDouble(cx, 2) << "' cy='"
                    << wxString::FromCDouble(cy, 2) << "' r='" << wxString::FromCDouble(size, 2)
                    << "' fill='" << (fill.IsOk() ? svgColour(fill) : wxString("none"))
                    << "' stroke='" << (outline.IsOk() ? svgColour(outline) : wxString("none"))
                    << "' stroke-width='" << wxString::FromCDouble(2.0 * k, 2) << "'/>";
            };
            const wxString inner = svgColour(p.background);
            const wxString width2 = wxString::FromCDouble(2.0 * k, 2);
            switch (itemsM[i].state)
            {
                case BenchmarkStageState::Running:
                    // a dot in a filled circle
                    circle(r, p.running, wxNullColour);
                    svg << "<circle cx='" << wxString::FromCDouble(cx, 2) << "' cy='"
                        << wxString::FromCDouble(cy, 2) << "' r='"
                        << wxString::FromCDouble(r * 0.36, 2) << "' fill='" << inner << "'/>";
                    break;
                case BenchmarkStageState::Done:
                    // a check
                    circle(r, p.done, wxNullColour);
                    svg << "<path d='M" << at(-4, 0.3) << "L" << at(-1.2, 3.1) << "L"
                        << at(4.2, -2.9) << "' fill='none' stroke='" << inner
                        << "' stroke-width='" << width2
                        << "' stroke-linecap='round' stroke-linejoin='round'/>";
                    break;
                case BenchmarkStageState::Failed:
                    // a cross
                    circle(r, p.failed, wxNullColour);
                    svg << "<path d='M" << at(-3.2, -3.2) << "L" << at(3.2, 3.2) << "M"
                        << at(3.2, -3.2) << "L" << at(-3.2, 3.2) << "' stroke='" << inner
                        << "' stroke-width='" << width2 << "' stroke-linecap='round'/>";
                    break;
                case BenchmarkStageState::Cancelled:
                    // a square: stopped
                    circle(r, p.muted, wxNullColour);
                    svg << "<rect x='" << wxString::FromCDouble(cx - 3 * k, 2) << "' y='"
                        << wxString::FromCDouble(cy - 3 * k, 2) << "' width='"
                        << wxString::FromCDouble(6 * k, 2) << "' height='"
                        << wxString::FromCDouble(6 * k, 2) << "' rx='"
                        << wxString::FromCDouble(0.8 * k, 2) << "' fill='" << inner << "'/>";
                    break;
                case BenchmarkStageState::Skipped:
                    // a ring with a dash
                    circle(r - k, wxNullColour, p.ring);
                    svg << "<path d='M" << at(-3.5, 0) << "L" << at(3.5, 0) << "' stroke='"
                        << svgColour(p.ring) << "' stroke-width='" << width2
                        << "' stroke-linecap='round'/>";
                    break;
                default:
                    // waiting or not run: an empty ring
                    circle(r - k, wxNullColour, p.ring);
                    break;
            }
        }
        return svg + "</svg>";
    }

    void OnPaint(wxPaintEvent& WXUNUSED(event))
    {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(paletteM.background));
        dc.Clear();
        const wxSize size = GetClientSize();
        if (size.x <= 0 || size.y <= 0 || itemsM.empty())
            return;
        const wxFont bold = GetFont().Bold();
        dc.SetFont(bold);
        int wordWidth = 0;
        for (const StripItem& item : itemsM)
            wordWidth = std::max(wordWidth, dc.GetTextExtent(item.word).x);
        // the marks evenly from the first to the last word
        const int n = int(itemsM.size());
        const double margin = std::max(wordWidth / 2.0, double(radius())) + FromDIP(2);
        const double distance = n > 1 ? (size.x - 2 * margin) / (n - 1) : 0;
        centersM.resize(itemsM.size());
        for (int i = 0; i < n; ++i)
            centersM[i] = int(std::lround(margin + i * distance));
        if (!shapesM.IsOk())
        {
            const double scale = getBitmapScale(this);
            const wxSize pixels(int(std::lround(size.x * scale)),
                int(std::lround(shapeHeight() * scale)));
            const wxString svg = shapes(size.x, pixels);
            shapesM = toBitmap(renderSvg(svg, pixels), scale);
        }
        dc.DrawBitmap(shapesM, 0, 0, true);
        const int wordY = shapeHeight() + FromDIP(4);
        for (int i = 0; i < n; ++i)
        {
            const StripItem& item = itemsM[i];
            dc.SetFont(bold);
            dc.SetTextForeground(item.colour);
            const wxSize extent = dc.GetTextExtent(item.word);
            dc.DrawText(item.word, centersM[i] - extent.x / 2, wordY);
            if (item.detail.empty())
                continue;
            dc.SetFont(GetFont());
            dc.SetTextForeground(item.state == BenchmarkStageState::Failed
                ? paletteM.failedText : paletteM.muted);
            const wxSize detail = dc.GetTextExtent(item.detail);
            const int x = std::clamp(centersM[i] - detail.x / 2, 0,
                std::max(0, size.x - detail.x));
            dc.DrawText(item.detail, x, wordY + extent.y + FromDIP(2));
        }
    }
};

BenchmarkProgressPanel::BenchmarkProgressPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), timerM(this)
{
    // a page in the colours of a text field, like the report next to it
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    const Palette p = getPalette();

    logoM = new BenchmarkLogo(this);
    percentM = new wxStaticText(this, wxID_ANY, "0 %");
    wxFont big = GetFont();
    big.SetFractionalPointSize(GetFont().GetFractionalPointSize() * 2.2);
    big.SetWeight(wxFONTWEIGHT_BOLD);
    percentM->SetFont(big);
    // the time does not move the layout when it changes
    elapsedM = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    elapsedM->SetForegroundColour(p.muted);
    elapsedM->SetMinSize(wxSize(elapsedM->GetTextExtent(wxString::Format(_("Elapsed: %s"),
        "88:88")).x + FromDIP(4), -1));

    createMarks();
    markM = new wxStaticBitmap(this, wxID_ANY, marksM[0]);
    markM->Hide();
    wordM = new wxStaticText(this, wxID_ANY, wxEmptyString);
    wordM->SetFont(GetFont().Bold());
    stepM = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxST_ELLIPSIZE_END | wxST_NO_AUTORESIZE);
    // a long step is cut where the dialog is narrow
    stepM->SetMinSize(wxSize(FromDIP(40), -1));
    stripM = new BenchmarkStageStrip(this);

    // after an error: the message in full, readable and to be copied, and
    // the way to the log
    errorM = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_NO_VSCROLL | wxBORDER_NONE);
    errorM->SetBackgroundColour(GetBackgroundColour());
    errorM->SetForegroundColour(GetForegroundColour());
    logButtonM = new wxButton(this, wxID_ANY, _("Show the &log"));
    logButtonM->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        if (onShowLog)
            onShowLog();
    });
    // the error comes at the end of a run, when the settings are hidden and
    // the page has room: no room is kept for it during the run, so that the
    // dialog stays small; the rest of a long message scrolls
    errorM->SetMinSize(wxSize(FromDIP(40), 2 * errorM->GetCharHeight()));
    wxBoxSizer* sizerError = new wxBoxSizer(wxHORIZONTAL);
    sizerError->Add(errorM, 1, wxEXPAND | wxRIGHT, FromDIP(12));
    sizerError->Add(logButtonM, 0, wxALIGN_TOP);

    wxBoxSizer* sizerNumbers = new wxBoxSizer(wxHORIZONTAL);
    sizerNumbers->Add(percentM, 0, wxALIGN_BOTTOM);
    sizerNumbers->AddStretchSpacer(1);
    sizerNumbers->Add(elapsedM, 0, wxALIGN_BOTTOM | wxBOTTOM, FromDIP(4));
    // the step, with the word of its stage or a mark at the end
    wxBoxSizer* sizerStep = new wxBoxSizer(wxHORIZONTAL);
    sizerStep->Add(markM, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
    sizerStep->Add(wordM, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
    sizerStep->Add(stepM, 1, wxALIGN_CENTER_VERTICAL);
    sizerStep->SetMinSize(wxSize(-1, std::max(FromDIP(16), GetCharHeight())));
    wxBoxSizer* sizerRight = new wxBoxSizer(wxVERTICAL);
    sizerRight->Add(sizerNumbers, 0, wxEXPAND);
    sizerRight->Add(sizerStep, 0, wxEXPAND | wxTOP, FromDIP(2));
    sizerRight->Add(stripM, 0, wxEXPAND | wxTOP, FromDIP(8));
    // the logo is the eye-catcher, the numbers and the stages beside it
    wxBoxSizer* sizerTop = new wxBoxSizer(wxHORIZONTAL);
    sizerTop->Add(logoM, 0, wxALIGN_TOP | wxRIGHT, FromDIP(28));
    sizerTop->Add(sizerRight, 1, wxEXPAND);
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
    sizerMain->Add(sizerTop, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
    sizerMain->Add(sizerError, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
    sizerMain->AddSpacer(FromDIP(12));
    SetSizer(sizerMain);
    errorM->Hide();
    logButtonM->Hide();

    Bind(wxEVT_TIMER, &BenchmarkProgressPanel::OnTimer, this);
    Bind(wxEVT_DPI_CHANGED, &BenchmarkProgressPanel::OnDPIChanged, this);
}

BenchmarkProgressPanel::~BenchmarkProgressPanel()
{
    timerM.Stop();
}

void BenchmarkProgressPanel::createMarks()
{
    // the end of a run: a check, a square (stopped), a cross; drawn for the
    // DPI of the window by wxBitmapBundle
    const Palette p = getPalette();
    const wxString inner = svgColour(p.background);
    const wxString bodies[3] = {
        "<circle cx='8' cy='8' r='7.5' fill='" + svgColour(p.done) + "'/>"
            "<path d='M4.6 8.3l2.3 2.4L11.5 5.6' fill='none' stroke='" + inner
            + "' stroke-width='1.9' stroke-linecap='round' stroke-linejoin='round'/>",
        "<circle cx='8' cy='8' r='7.5' fill='" + svgColour(p.muted) + "'/>"
            "<rect x='5.4' y='5.4' width='5.2' height='5.2' rx='0.6' fill='" + inner + "'/>",
        "<circle cx='8' cy='8' r='7.5' fill='" + svgColour(p.failed) + "'/>"
            "<path d='M5.4 5.4L10.6 10.6M10.6 5.4L5.4 10.6' stroke='" + inner
            + "' stroke-width='1.9' stroke-linecap='round'/>" };
    for (int i = 0; i < 3; ++i)
    {
        const wxString svg = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'>"
            + bodies[i] + "</svg>";
        const wxScopedCharBuffer utf8 = svg.utf8_str();
        marksM[i] = wxBitmapBundle::FromSVG(utf8.data(), wxSize(16, 16));
    }
}

void BenchmarkProgressPanel::setStepLine(int mark, const wxString& word,
    const wxColour& wordColour, const wxString& text, const wxColour& textColour)
{
    // most messages change only the text, which has a fixed place: the
    // layout only when the mark or the word change
    bool layout = false;
    if (mark != markIndexM)
    {
        markIndexM = mark;
        markM->Show(mark >= 0);
        if (mark >= 0)
            markM->SetBitmap(marksM[mark]);
        layout = true;
    }
    if (wordM->GetLabel() != word || wordM->IsShown() == word.empty())
    {
        wordM->Show(!word.empty());
        wordM->SetLabel(word);
        layout = true;
    }
    if (wordM->GetForegroundColour() != wordColour)
    {
        wordM->SetForegroundColour(wordColour);
        wordM->Refresh();
    }
    if (stepM->GetForegroundColour() != textColour)
    {
        stepM->SetForegroundColour(textColour);
        stepM->Refresh();
    }
    if (stepM->GetLabel() != text)
    {
        stepM->SetLabel(text);
        stepM->SetToolTip(text);
    }
    if (layout)
        Layout();
}

void BenchmarkProgressPanel::setFraction(double fraction)
{
    const int percent = int(std::floor(fraction * 100));
    const wxString label = wxString::Format("%d %%", percent);
    if (percentM->GetLabel() != label)
        percentM->SetLabel(label);
    const int level = std::clamp(int(std::lround(fraction * logoLevels)), 0, logoLevels);
    logoM->setLevel(level, mutedM);
    // for screen readers
    const wxString name = wxString::Format(_("Progress %d percent"), percent);
    if (logoM->GetName() != name)
    {
        logoM->SetName(name);
        logoM->SetLabel(name);
    }
}

void BenchmarkProgressPanel::updateStages()
{
    const Palette p = getPalette();
    const std::vector<fr::BenchmarkStagePlan>& plan = trackerM.getPlan();
    std::vector<StripItem> items;
    wxString text = _("Stages:");
    for (size_t i = 0; i < plan.size(); ++i)
    {
        StripItem item;
        item.word = fr::getBenchmarkStageWord(plan[i].stage);
        item.colour = getStageColour(plan[i].stage, p);
        item.state = trackerM.getState(i);
        item.fraction = trackerM.getStageFraction(i);
        // the name of the running stage where it says more than the word,
        // and the state where it ended
        const wxString name = fr::getBenchmarkStageName(plan[i].stage);
        if (item.state == BenchmarkStageState::Running && runningM
            && name.CmpNoCase(item.word) != 0)
        {
            item.detail = name;
        }
        else if (item.state == BenchmarkStageState::Failed
            || item.state == BenchmarkStageState::Cancelled
            || item.state == BenchmarkStageState::Skipped)
        {
            item.detail = fr::getBenchmarkStageStateName(item.state);
        }
        text += wxString::Format(" %s %s%s", item.word,
            fr::getBenchmarkStageStateName(item.state), i + 1 < plan.size() ? "," : ".");
        items.push_back(item);
    }
    stripM->setItems(items);
    // the state as text for screen readers
    if (stripM->GetName() != text)
    {
        stripM->SetName(text);
        stripM->SetLabel(text);
    }
}

void BenchmarkProgressPanel::showReady(const std::vector<fr::BenchmarkStagePlan>& plan,
    const wxString& hint)
{
    const Palette p = getPalette();
    trackerM = fr::BenchmarkStageTracker(plan);
    readyM = true;
    runningM = false;
    mutedM = false;
    updateTimer();
    percentM->SetForegroundColour(p.muted);
    elapsedM->SetLabel(wxEmptyString);
    setStepLine(-1, wxEmptyString, p.text, hint, p.text);
    errorM->Hide();
    logButtonM->Hide();
    setFraction(0.0);
    updateStages();
    Layout();
}

void BenchmarkProgressPanel::start(const std::vector<fr::BenchmarkStagePlan>& plan)
{
    const Palette p = getPalette();
    trackerM = fr::BenchmarkStageTracker(plan);
    readyM = false;
    runningM = true;
    mutedM = false;
    startM = std::chrono::steady_clock::now();
    elapsedSecondsM = -1;
    percentM->SetForegroundColour(p.text);
    setStepLine(-1, wxEmptyString, p.text, _("Starting..."), p.text);
    errorM->Hide();
    logButtonM->Hide();
    setFraction(0.0);
    updateStages();
    animationM.quietStep = false;
    readEnvironment();
    updateTimer();
    updateElapsed();
    Layout();
}

void BenchmarkProgressPanel::update(const fr::BenchmarkProgress& progress)
{
    if (!runningM)
        return;
    const int stage = trackerM.getCurrent();
    trackerM.update(progress);
    const Palette p = getPalette();
    setStepLine(-1, fr::getBenchmarkStageWord(progress.stage),
        getStageColour(progress.stage, p), progress.cleaningUp ? progress.message
        : wxString::Format(_("Step %d of %d: %s"), progress.step, progress.stepCount,
            progress.message), p.text);
    setFraction(trackerM.getFraction());
    if (trackerM.getCurrent() != stage)
    {
        // the system settings once per stage, so that a remote session that
        // starts during the run is noticed
        readEnvironment();
    }
    updateStages();
    animationM.quietStep = progress.quiet;
    updateTimer();
}

void BenchmarkProgressPanel::cancelRequested()
{
    if (!runningM)
        return;
    const Palette p = getPalette();
    setStepLine(-1, wordM->GetLabel(), wordM->GetForegroundColour(),
        _("Cancelling, the temporary database is being dropped..."), p.text);
}

void BenchmarkProgressPanel::finish(Outcome outcome, bool cleanupFailed,
    const wxString& message, const wxString& error)
{
    const Palette p = getPalette();
    // the time up to the end
    updateElapsed();
    runningM = false;
    readyM = false;
    updateTimer();
    trackerM.finish(outcome, cleanupFailed);
    // after a cancel and an error the logo is grey where the run stopped
    mutedM = outcome != Outcome::Finished;
    setFraction(trackerM.getFraction());
    percentM->SetForegroundColour(mutedM ? p.muted : p.text);
    switch (outcome)
    {
        case Outcome::Finished:
            setStepLine(0, wxEmptyString, p.text, message, p.text);
            break;
        case Outcome::Cancelled:
            setStepLine(1, wxEmptyString, p.text, message, p.text);
            break;
        case Outcome::Failed:
        default:
        {
            const int end = trackerM.getEndStage();
            const std::vector<fr::BenchmarkStagePlan>& plan = trackerM.getPlan();
            const wxString word = end >= 0 && end < int(plan.size())
                ? fr::getBenchmarkStageWord(plan[end].stage) : wxString();
            setStepLine(2, wxEmptyString, p.text, word.empty() ? message
                : wxString::Format(_("Failed in the %s stage."), word), p.failedText);
            break;
        }
    }
    const bool failed = outcome == Outcome::Failed && !error.empty();
    errorM->ChangeValue(failed ? error : wxString());
    errorM->Show(failed);
    logButtonM->Show(failed);
    updateStages();
    Layout();
}

void BenchmarkProgressPanel::readEnvironment()
{
#ifdef __WXMSW__
    animationM.remoteSession = ::GetSystemMetrics(SM_REMOTESESSION) != 0;
    BOOL animations = TRUE;
    if (::SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0))
        animationM.systemAnimations = animations != FALSE;
#else
    // a session over SSH (X forwarding) is the best guess of a remote one
    animationM.remoteSession = wxGetEnv("SSH_CONNECTION", nullptr);
#endif
#ifdef __WXGTK__
    // gtk-enable-animations of the GTK that wxWidgets uses, without its
    // headers
    wxLogNull noLog;
    wxDynamicLibrary program;
    program.Attach(wxDynamicLibrary::GetProgramHandle());
    if (program.HasSymbol("gtk_settings_get_default") && program.HasSymbol("g_object_get"))
    {
        typedef void* (*GetSettings)();
        typedef void (*GetProperty)(void*, const char*, ...);
        GetSettings getSettings = reinterpret_cast<GetSettings>(
            program.GetSymbol("gtk_settings_get_default"));
        GetProperty getProperty = reinterpret_cast<GetProperty>(
            program.GetSymbol("g_object_get"));
        if (void* settings = getSettings())
        {
            int enabled = 1;
            getProperty(settings, "gtk-enable-animations", &enabled, nullptr);
            animationM.systemAnimations = enabled != 0;
        }
    }
    program.Detach();
#endif
    animationM.userDisabled = wxGetEnv("FLAMEROBIN_NO_ANIMATION", nullptr);
}

void BenchmarkProgressPanel::updateTimer()
{
    animationM.running = runningM;
    wxTopLevelWindow* top = wxDynamicCast(wxGetTopLevelParent(this), wxTopLevelWindow);
    animationM.windowVisible = IsShownOnScreen() && !(top && top->IsIconized());
    const fr::BenchmarkAnimation mode = fr::getBenchmarkAnimation(animationM);
    if (mode != modeM)
    {
        // the first picture of the movement at once
        modeM = mode;
        ticksM = 0;
        logoM->showFrame(mode == fr::BenchmarkAnimation::Wave ? 0
            : mode == fr::BenchmarkAnimation::Breathe ? firstBreathFrame : stillFrame);
    }
    const int interval = fr::getBenchmarkTimerInterval(animationM);
    if (interval == 0)
    {
        timerM.Stop();
        intervalM = 0;
        return;
    }
    if (interval != intervalM || !timerM.IsRunning())
    {
        intervalM = interval;
        timerM.Start(interval);
    }
}

void BenchmarkProgressPanel::updateElapsed()
{
    const long seconds = long(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startM).count());
    if (seconds != elapsedSecondsM)
    {
        elapsedSecondsM = seconds;
        elapsedM->SetLabel(wxString::Format(_("Elapsed: %s"), wxString::Format("%02ld:%02ld",
            seconds / 60, seconds % 60)));
    }
}

void BenchmarkProgressPanel::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    updateElapsed();
    if (!runningM)
        return;
    // hidden or shown again: the animation stops or goes on
    wxTopLevelWindow* top = wxDynamicCast(wxGetTopLevelParent(this), wxTopLevelWindow);
    const bool visible = IsShownOnScreen() && !(top && top->IsIconized());
    if (visible != animationM.windowVisible)
        updateTimer();
    // one picture per tick, drawn only because it changes
    switch (modeM)
    {
        case fr::BenchmarkAnimation::Wave:
        {
            // the edge moves on; now and then the robin blinks, on the first
            // position of the edge, which the blink picture has
            ++ticksM;
            const int blink = fr::getBenchmarkBlinkInterval(animationM) / intervalM;
            logoM->showFrame(blink > 0 && ticksM % blink == 0 ? blinkFrame
                : ticksM % wavePhases);
            break;
        }
        case fr::BenchmarkAnimation::Breathe:
            ++ticksM;
            logoM->showFrame(firstBreathFrame + ticksM % breathFrames);
            break;
        default:
            break;
    }
}

void BenchmarkProgressPanel::OnDPIChanged(wxDPIChangedEvent& event)
{
    // the pictures are drawn for the DPI of the window, not scaled
    logoM->createImages();
    createMarks();
    if (markIndexM >= 0)
        markM->SetBitmap(marksM[markIndexM]);
    stripM->resetPictures();
    event.Skip();
}
