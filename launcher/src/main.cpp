// Launcher for the God of War II native port. The settings are saved to GoW2Launcher.ini next to the executable and
// loaded at startup, but nothing applies them to the game yet and PLAY does not start it. The layout follows the
// approved mockup (bronze on near-black, Cinzel for display text and Inter for everything else). Every position and
// size below is in mockup pixels (a 960 x 600 design) and goes through px(), which scales the whole window by kScale.

#include "raylib.h"
#include "rlgl.h"
#include "imgui.h"
#include "rlImGui.h"

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace
{
constexpr float kScale = 1.125f; // 960 x 600 mockup -> 1080 x 675 window
constexpr float px(float v) { return v * kScale; }
ImVec2 px(float x, float y) { return ImVec2(x * kScale, y * kScale); }

constexpr float kDesignW = 960.0f, kDesignH = 600.0f;
constexpr int kWidth = static_cast<int>(kDesignW * kScale);
constexpr int kHeight = static_cast<int>(kDesignH * kScale);
constexpr float kTitleH = 40.0f;
constexpr float kLeftW = 340.0f;
constexpr float kTabsH = 42.0f;
constexpr float kFooterH = 64.0f;

// palette (0xRRGGBB)
constexpr unsigned kGround = 0x141110, kPanel = 0x110E0D, kTitleBar = 0x0E0B0B, kLine = 0x2A2321, kFrame = 0x2F2624;
constexpr unsigned kText = 0xECE4DC, kTextBright = 0xF3ECE4, kTextSoft = 0xDDD3CA, kMuted = 0xA8A09A;
constexpr unsigned kBronze = 0xC9A45C, kBronzeHi = 0xDDBB78, kEmber = 0x9E2A1F, kEmberHi = 0xB3321F, kEmberLo = 0x86231A;
constexpr unsigned kEmberEdge = 0xC0392B, kSideBg = 0x1D1817, kSideBgHi = 0x26201E, kSideEdge = 0x33292A, kSideEdgeHi = 0x4A3C36;
constexpr unsigned kField = 0x120F0E, kFieldHi = 0x1A1513, kFieldEdge = 0x3A302C, kOk = 0x7FBF7F;

ImU32 col(unsigned rgb, float alpha = 1.0f)
{
    return IM_COL32((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, static_cast<int>(alpha * 255.0f));
}
ImVec4 colv(unsigned rgb, float alpha = 1.0f)
{
    return ImVec4(((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f, (rgb & 0xFF) / 255.0f, alpha);
}

struct Fonts
{
    ImFont *display = nullptr;  // Cinzel Bold
    ImFont *regular = nullptr;  // Inter Regular
    ImFont *medium = nullptr;   // Inter Medium
    ImFont *semibold = nullptr; // Inter SemiBold
} g_font;

// ---- settings --------------------------------------------------------------------------------------------------------

struct Settings
{
    int resolution = 1;   // 0..3: 1x..4x
    int aspect = 0;       // 0: 16:9, 1: 4:3
    int windowMode = 1;   // 0: windowed, 1: borderless fullscreen
    int antiAliasing = 1; // 0: off, 1: FXAA
    int sharpening = 40;  // 0..100 %
    bool vsync = true;
    bool showFps = false;
    bool operator==(const Settings &o) const
    {
        return resolution == o.resolution && aspect == o.aspect && windowMode == o.windowMode &&
               antiAliasing == o.antiAliasing && sharpening == o.sharpening && vsync == o.vsync && showFps == o.showFps;
    }
};

const char *const kResolutionKey[] = {"1x", "2x", "3x", "4x"};
const char *const kAspectKey[] = {"16:9", "4:3"};
const char *const kWindowKey[] = {"windowed", "borderless"};
const char *const kAaKey[] = {"off", "fxaa"};

std::string settingsPath() { return std::string(GetApplicationDirectory()) + "GoW2Launcher.ini"; }

int indexOf(const char *const *keys, int count, const std::string &value, int fallback)
{
    for (int i = 0; i < count; ++i)
        if (value == keys[i])
            return i;
    return fallback;
}

// a missing file or a bad line keeps the default for that setting
Settings loadSettings()
{
    Settings s;
    std::ifstream in(settingsPath());
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        const size_t eq = line.find('=');
        if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '[' || eq == std::string::npos)
            continue;
        const std::string key = line.substr(0, eq), value = line.substr(eq + 1);
        if (key == "resolution")
            s.resolution = indexOf(kResolutionKey, 4, value, s.resolution);
        else if (key == "aspect_ratio")
            s.aspect = indexOf(kAspectKey, 2, value, s.aspect);
        else if (key == "window_mode")
            s.windowMode = indexOf(kWindowKey, 2, value, s.windowMode);
        else if (key == "anti_aliasing")
            s.antiAliasing = indexOf(kAaKey, 2, value, s.antiAliasing);
        else if (key == "sharpening")
        {
            const int v = std::atoi(value.c_str());
            if (v >= 0 && v <= 100 && !value.empty())
                s.sharpening = v;
        }
        else if (key == "vsync")
            s.vsync = value == "1" || value == "true";
        else if (key == "show_fps")
            s.showFps = value == "1" || value == "true";
    }
    return s;
}

bool saveSettings(const Settings &s)
{
    std::ofstream out(settingsPath(), std::ios::trunc);
    if (!out)
        return false;
    out << "; God of War II native port: launcher settings (written by GoW2Launcher)\n"
        << "[graphics]\n"
        << "resolution=" << kResolutionKey[s.resolution] << "\n"
        << "aspect_ratio=" << kAspectKey[s.aspect] << "\n"
        << "window_mode=" << kWindowKey[s.windowMode] << "\n"
        << "anti_aliasing=" << kAaKey[s.antiAliasing] << "\n"
        << "sharpening=" << s.sharpening << "\n"
        << "vsync=" << (s.vsync ? 1 : 0) << "\n"
        << "show_fps=" << (s.showFps ? 1 : 0) << "\n";
    return static_cast<bool>(out);
}

Settings g_saved;   // what the file holds
Settings g_edit;    // what the controls show
int g_tab = 0;
std::string g_flash; // short footer message after Save / Cancel / Restore defaults
double g_flashUntil = 0.0;
bool g_flashError = false;

void flash(const char *message, bool error = false)
{
    g_flash = message;
    g_flashError = error;
    g_flashUntil = GetTime() + 2.5;
}

bool g_quit = false;
bool g_dragging = false;
Vector2 g_dragFrom{};

// ---- drawing helpers (sizes in mockup pixels) -----------------------------------------------------------------------

ImVec2 textSize(ImFont *font, float size, const char *text, float spacing = 0.0f)
{
    ImVec2 s = font->CalcTextSizeA(px(size), FLT_MAX, 0.0f, text);
    if (spacing != 0.0f)
    {
        int glyphs = 0;
        for (const char *c = text; *c; ++c)
            glyphs += (static_cast<unsigned char>(*c) & 0xC0) != 0x80;
        s.x += px(spacing) * static_cast<float>(glyphs > 0 ? glyphs - 1 : 0);
    }
    return ImVec2(s.x / kScale, s.y / kScale);
}

// text with letter spacing (ImGui has none): one glyph at a time
void text(ImDrawList *dl, ImFont *font, float size, ImVec2 p, ImU32 c, const char *s, float spacing = 0.0f)
{
    ImVec2 at = px(p.x, p.y);
    if (spacing == 0.0f)
    {
        dl->AddText(font, px(size), at, c, s);
        return;
    }
    for (const char *ch = s; *ch;)
    {
        const char *next = ch + 1;
        while ((static_cast<unsigned char>(*next) & 0xC0) == 0x80)
            ++next;
        dl->AddText(font, px(size), at, c, ch, next);
        at.x += font->CalcTextSizeA(px(size), FLT_MAX, 0.0f, ch, next).x + px(spacing);
        ch = next;
    }
}

// paragraph wrapped at maxWidth with an explicit line height; returns the height used
float paragraph(ImDrawList *dl, ImFont *font, float size, ImVec2 p, ImU32 c, const char *s, float maxWidth, float lineH)
{
    std::string line;
    std::vector<std::string> lines;
    const std::string all(s);
    size_t i = 0;
    while (true)
    {
        const size_t end = all.find(' ', i);
        const std::string word = all.substr(i, end == std::string::npos ? std::string::npos : end - i);
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && textSize(font, size, candidate.c_str()).x > maxWidth)
        {
            lines.push_back(line);
            line = word;
        }
        else
            line = candidate;
        if (end == std::string::npos)
            break;
        i = end + 1;
    }
    if (!line.empty())
        lines.push_back(line);
    for (size_t n = 0; n < lines.size(); ++n)
        text(dl, font, size, ImVec2(p.x, p.y + lineH * static_cast<float>(n)), c, lines[n].c_str());
    return lineH * static_cast<float>(lines.size());
}

void rectFilled(ImDrawList *dl, float x0, float y0, float x1, float y1, ImU32 c, float r = 0.0f, ImDrawFlags f = 0)
{
    dl->AddRectFilled(px(x0, y0), px(x1, y1), c, px(r), f);
}
void rect(ImDrawList *dl, float x0, float y0, float x1, float y1, ImU32 c, float r = 0.0f, float w = 1.0f)
{
    dl->AddRect(px(x0, y0), px(x1, y1), c, px(r), 0, w);
}
void hline(ImDrawList *dl, float x0, float x1, float y, ImU32 c) { dl->AddLine(px(x0, y), px(x1, y), c, 1.0f); }
void vline(ImDrawList *dl, float x, float y0, float y1, ImU32 c) { dl->AddLine(px(x, y0), px(x, y1), c, 1.0f); }

// icons: stroke drawings on a 24-unit grid, like the mockup's inline SVGs
struct Icon
{
    ImDrawList *dl;
    ImVec2 o; // mockup pixels
    float s;  // mockup pixels per grid unit
    ImVec2 p(float x, float y) const { return px(o.x + x * s, o.y + y * s); }
    void line(float x0, float y0, float x1, float y1, ImU32 c, float w) const { dl->AddLine(p(x0, y0), p(x1, y1), c, px(w)); }
};
Icon icon(ImDrawList *dl, ImVec2 at, float size) { return Icon{dl, at, size / 24.0f}; }

void iconShield(ImDrawList *dl, ImVec2 at, float size, ImU32 c)
{
    const Icon i = icon(dl, at, size);
    dl->PathLineTo(i.p(12, 2));
    dl->PathLineTo(i.p(4, 6));
    dl->PathLineTo(i.p(4, 12));
    dl->PathBezierCubicCurveTo(i.p(4, 17), i.p(7.5f, 20.5f), i.p(12, 22));
    dl->PathBezierCubicCurveTo(i.p(16.5f, 20.5f), i.p(20, 17), i.p(20, 12));
    dl->PathLineTo(i.p(20, 6));
    dl->PathStroke(c, ImDrawFlags_Closed, px(1.6f));
    i.line(12, 7, 12, 17, c, 1.6f);
    i.line(8.5f, 10, 15.5f, 10, c, 1.6f);
}
void iconPlay(ImDrawList *dl, ImVec2 at, float size, ImU32 c)
{
    const Icon i = icon(dl, at, size);
    dl->AddTriangleFilled(i.p(7, 4.5f), i.p(20, 12), i.p(7, 19.5f), c);
}
void iconSliders(ImDrawList *dl, ImVec2 at, float size, ImU32 c)
{
    const Icon i = icon(dl, at, size);
    i.line(4, 7, 14, 7, c, 1.6f);
    i.line(18, 7, 20, 7, c, 1.6f);
    i.line(4, 17, 8, 17, c, 1.6f);
    i.line(12, 17, 20, 17, c, 1.6f);
    dl->AddCircle(i.p(16, 7), px(2.0f * i.s), c, 12, px(1.6f));
    dl->AddCircle(i.p(10, 17), px(2.0f * i.s), c, 12, px(1.6f));
}
void iconFolder(ImDrawList *dl, ImVec2 at, float size, ImU32 c)
{
    const Icon i = icon(dl, at, size);
    const ImVec2 pts[] = {i.p(3, 7), i.p(9, 7), i.p(11, 9), i.p(21, 9), i.p(21, 19), i.p(3, 19)};
    dl->AddPolyline(pts, 6, c, ImDrawFlags_Closed, px(1.6f));
}
void iconLog(ImDrawList *dl, ImVec2 at, float size, ImU32 c)
{
    const Icon i = icon(dl, at, size);
    const ImVec2 pts[] = {i.p(7, 3), i.p(15, 3), i.p(19, 7), i.p(19, 21), i.p(7, 21)};
    dl->AddPolyline(pts, 5, c, ImDrawFlags_Closed, px(1.6f));
    i.line(10, 12, 16, 12, c, 1.6f);
    i.line(10, 16, 16, 16, c, 1.6f);
}
void iconCheck(ImDrawList *dl, ImVec2 at, float size, ImU32 c, float w = 2.0f)
{
    const Icon i = icon(dl, at, size);
    const ImVec2 pts[] = {i.p(5, 12.5f), i.p(10, 17), i.p(19, 7)};
    dl->AddPolyline(pts, 3, c, ImDrawFlags_None, px(w));
}

// an invisible ImGui button over a rectangle (mockup pixels): hover / held state, a hand cursor, and the click
struct Hit
{
    bool hovered, held, clicked;
};
Hit hit(const char *id, ImVec2 pos, ImVec2 size)
{
    ImGui::SetCursorScreenPos(px(pos.x, pos.y));
    const bool clicked = ImGui::InvisibleButton(id, px(size.x, size.y));
    const Hit h{ImGui::IsItemHovered(), ImGui::IsItemActive(), clicked};
    if (h.hovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    return h;
}

// ---- pieces ---------------------------------------------------------------------------------------------------------

void titleBar(ImDrawList *dl)
{
    rectFilled(dl, 0, 0, kDesignW, kTitleH, col(kTitleBar), 10.0f, ImDrawFlags_RoundCornersTop);
    hline(dl, 0, kDesignW, kTitleH - 0.5f, col(kLine));
    iconShield(dl, ImVec2(14, 11), 18, col(kBronze));
    text(dl, g_font.regular, 12, ImVec2(42, 13), col(kMuted), "GoW II Native Port \xE2\x80\x94 Launcher", 0.5f);

    // window buttons: these two do work, the launcher could not be closed otherwise
    const ImVec2 bs(30, 30);
    const ImVec2 closeAt(kDesignW - 8 - bs.x, 5), minAt(closeAt.x - 2 - bs.x, 5);
    const Hit mn = hit("##minimize", minAt, bs);
    if (mn.hovered)
        rectFilled(dl, minAt.x, minAt.y, minAt.x + bs.x, minAt.y + bs.y, col(kLine), 6.0f);
    icon(dl, ImVec2(minAt.x + 8, minAt.y + 8), 14).line(5, 12, 19, 12, col(mn.hovered ? kText : kMuted), 1.6f);
    if (mn.clicked)
        MinimizeWindow();
    const Hit cl = hit("##close", closeAt, bs);
    if (cl.hovered)
        rectFilled(dl, closeAt.x, closeAt.y, closeAt.x + bs.x, closeAt.y + bs.y, col(kEmber), 6.0f);
    const Icon x = icon(dl, ImVec2(closeAt.x + 8, closeAt.y + 8), 14);
    x.line(6, 6, 18, 18, col(cl.hovered ? kTextBright : kMuted), 1.6f);
    x.line(18, 6, 6, 18, col(cl.hovered ? kTextBright : kMuted), 1.6f);
    if (cl.clicked)
        g_quit = true;

    // drag the window by the rest of the bar (window pixels here)
    const Vector2 m = GetMousePosition();
    const bool overBar = m.y >= 0 && m.y < px(kTitleH) && m.x < px(minAt.x);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && overBar)
    {
        g_dragging = true;
        g_dragFrom = m;
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        g_dragging = false;
    if (g_dragging)
    {
        const Vector2 w = GetWindowPosition();
        SetWindowPosition(static_cast<int>(w.x + m.x - g_dragFrom.x), static_cast<int>(w.y + m.y - g_dragFrom.y));
    }
}

bool sideButton(ImDrawList *dl, const char *id, ImVec2 pos, const char *label, void (*drawIcon)(ImDrawList *, ImVec2, float, ImU32))
{
    const ImVec2 size(kLeftW - 64, 44);
    const Hit h = hit(id, pos, size);
    rectFilled(dl, pos.x, pos.y, pos.x + size.x, pos.y + size.y, col(h.hovered ? kSideBgHi : kSideBg), 8.0f);
    rect(dl, pos.x, pos.y, pos.x + size.x, pos.y + size.y, col(h.hovered ? kSideEdgeHi : kSideEdge), 8.0f);
    drawIcon(dl, ImVec2(pos.x + 14, pos.y + 13), 18, col(kTextSoft));
    text(dl, g_font.medium, 14, ImVec2(pos.x + 44, pos.y + 13), col(h.hovered ? kTextBright : kTextSoft), label);
    return h.clicked;
}

void leftPanel(ImDrawList *dl)
{
    rectFilled(dl, 0, kTitleH, kLeftW, kDesignH, col(kPanel), 10.0f, ImDrawFlags_RoundCornersBottomLeft);
    vline(dl, kLeftW - 0.5f, kTitleH, kDesignH, col(kLine));

    const float x = 32.0f, w = kLeftW - 64.0f;
    float y = kTitleH + 36.0f;
    text(dl, g_font.semibold, 11, ImVec2(x, y), col(kBronze), "NATIVE PC PORT", 2.4f);
    y += 19.0f;
    text(dl, g_font.display, 38, ImVec2(x, y), col(kTextBright), "God of War II", 1.1f);
    y += 41.0f + 14.0f;
    rectFilled(dl, x, y, x + 64, y + 2, col(kEmber));
    y += 12.0f;
    y += paragraph(dl, g_font.regular, 13, ImVec2(x, y), col(kMuted),
                   "Static recompilation with PS2Recomp. Tune the graphics and performance before you launch the game.", w, 20.0f);

    // the button group sits in the middle of the free space, like the mockup's space-between column
    const float infoTop = kDesignH - 24.0f - 54.0f;
    const float groupH = 56.0f + 3.0f * (10.0f + 44.0f);
    float by = y + std::floor((infoTop - y - groupH) * 0.5f);

    const Hit play = hit("##play", ImVec2(x, by), ImVec2(w, 56));
    const unsigned playBg = play.held ? kEmberLo : play.hovered ? kEmberHi : kEmber;
    rectFilled(dl, x, by, x + w, by + 56, col(playBg), 8.0f);
    rect(dl, x, by, x + w, by + 56, col(kEmberEdge), 8.0f);
    const float labelW = textSize(g_font.display, 20, "PLAY", 2.8f).x, contentW = 18.0f + 12.0f + labelW;
    const float cx = x + std::floor((w - contentW) * 0.5f);
    iconPlay(dl, ImVec2(cx, by + 19), 18, col(0xFFF7F0));
    text(dl, g_font.display, 20, ImVec2(cx + 30, by + 16), col(0xFFF7F0), "PLAY", 2.8f);
    by += 56.0f + 10.0f;

    if (sideButton(dl, "##defaults", ImVec2(x, by), "Restore defaults", iconSliders))
    {
        g_edit = Settings{};
        flash("Defaults restored: Save settings to keep them");
    }
    by += 54.0f;
    sideButton(dl, "##folder", ImVec2(x, by), "Open game folder", iconFolder);
    by += 54.0f;
    sideButton(dl, "##logs", ImVec2(x, by), "View logs", iconLog);

    float iy = infoTop;
    iconCheck(dl, ImVec2(x, iy + 1), 14, col(kOk), 2.2f);
    text(dl, g_font.regular, 12, ImVec2(x + 22, iy), col(kMuted), "Reference ELF: SCUS_974.81 (v1.01)");
    iy += 18.0f;
    text(dl, g_font.regular, 12, ImVec2(x, iy), col(kMuted), "Version 0.1.0 \xC2\xB7 front-end preview");
    iy += 18.0f;
    text(dl, g_font.regular, 12, ImVec2(x, iy), col(kMuted), "Unofficial project, not affiliated with Sony.");
}

void tabs(ImDrawList *dl)
{
    static const char *const kNames[] = {"Graphics", "Performance", "Controls", "Audio"};
    float x = kLeftW + 32.0f;
    for (int t = 0; t < 4; ++t)
    {
        const ImVec2 sz = textSize(g_font.medium, 13, kNames[t], 0.26f);
        char id[16];
        std::snprintf(id, sizeof(id), "##tab%d", t);
        const Hit h = hit(id, ImVec2(x - 2, kTitleH), ImVec2(sz.x + 4, kTabsH));
        if (h.clicked)
            g_tab = t;
        const bool on = g_tab == t;
        text(dl, g_font.medium, 13, ImVec2(x, kTitleH + 13), col(on ? kTextBright : h.hovered ? kTextSoft : kMuted), kNames[t], 0.26f);
        if (on)
            rectFilled(dl, x - 2, kTitleH + kTabsH - 2, x + sz.x + 2, kTitleH + kTabsH, col(kBronze));
        x += sz.x + 28.0f;
    }
    hline(dl, kLeftW, kDesignW, kTitleH + kTabsH - 0.5f, col(kLine));
}

// a settings row: label and hint on the left, the control drawn by the caller on the right
float rowTop(ImDrawList *dl, float y, const char *label, const char *hint, bool last)
{
    const float x = kLeftW + 32.0f;
    text(dl, g_font.medium, 14, ImVec2(x, y + 12), col(kText), label);
    text(dl, g_font.regular, 12, ImVec2(x, y + 32), col(kMuted), hint);
    if (!last)
        hline(dl, x, kDesignW - 32.0f, y + 59.5f, col(kLine));
    return y + 30.0f; // vertical centre of the row
}

void combo(const char *id, float centreY, int *value, const char *const *items, int count)
{
    const float w = 170.0f;
    ImGui::SetCursorScreenPos(px(kDesignW - 32.0f - w, centreY - 18.0f));
    ImGui::SetNextItemWidth(px(w));
    ImGui::PushFont(g_font.medium, px(13.0f));
    if (ImGui::BeginCombo(id, items[*value]))
    {
        for (int i = 0; i < count; ++i)
            if (ImGui::Selectable(items[i], i == *value))
                *value = i;
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    ImGui::PopFont();
}

void slider(ImDrawList *dl, const char *id, float centreY, int *value)
{
    const float valueW = 36.0f, w = 130.0f;
    const float x0 = kDesignW - 32.0f - valueW - 12.0f - w;
    ImGui::SetCursorScreenPos(px(x0 - 8, centreY - 10));
    ImGui::InvisibleButton(id, px(w + 16, 20));
    const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    if (hovered || active)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    if (active)
    {
        const float t = (ImGui::GetIO().MousePos.x / kScale - x0) / w;
        *value = static_cast<int>(std::lround(std::fmin(std::fmax(t, 0.0f), 1.0f) * 100.0f));
    }
    const float kx = x0 + w * (static_cast<float>(*value) / 100.0f);
    rectFilled(dl, x0, centreY - 2, x0 + w, centreY + 2, col(kFieldEdge), 2.0f);
    rectFilled(dl, x0, centreY - 2, kx, centreY + 2, col(kBronze), 2.0f);
    dl->AddCircleFilled(px(kx, centreY), px(active ? 9.0f : 8.0f), col(hovered || active ? kBronzeHi : kBronze), 24);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d %%", *value);
    const float tw = textSize(g_font.medium, 13, buf).x;
    text(dl, g_font.medium, 13, ImVec2(kDesignW - 32.0f - tw, centreY - 8), col(kTextSoft), buf);
}

void checkbox(ImDrawList *dl, const char *id, float centreY, bool *value, const char *label)
{
    const float lw = textSize(g_font.medium, 13, label).x, w = 18.0f + 8.0f + lw;
    const ImVec2 at(kDesignW - 32.0f - w, centreY - 9);
    const Hit h = hit(id, ImVec2(at.x, at.y - 4), ImVec2(w, 26));
    if (h.clicked)
        *value = !*value;
    if (*value)
    {
        rectFilled(dl, at.x, at.y, at.x + 18, at.y + 18, col(h.hovered ? kBronzeHi : kBronze), 4.0f);
        iconCheck(dl, ImVec2(at.x + 1, at.y + 1), 16, col(0x1A1412), 2.4f);
    }
    else
    {
        rectFilled(dl, at.x, at.y, at.x + 18, at.y + 18, col(h.hovered ? kFieldHi : kField), 4.0f);
        rect(dl, at.x, at.y, at.x + 18, at.y + 18, col(h.hovered ? kSideEdgeHi : kFieldEdge), 4.0f, px(1.2f));
    }
    text(dl, g_font.medium, 13, ImVec2(at.x + 26, centreY - 8), col(kTextSoft), label);
}

void graphicsTab(ImDrawList *dl)
{
    static const char *const kResolution[] = {"1\xC3\x97 native (640\xC3\x97" "448)", "2\xC3\x97 (1280\xC3\x97" "896)",
                                              "3\xC3\x97 (1920\xC3\x97" "1344)", "4\xC3\x97 (2560\xC3\x97" "1792)"};
    static const char *const kAspect[] = {"16:9 widescreen", "4:3 original"};
    static const char *const kWindow[] = {"Windowed", "Borderless fullscreen"};
    static const char *const kAa[] = {"Off", "FXAA"};

    float y = kTitleH + kTabsH + 8.0f;
    float c = rowTop(dl, y, "Internal resolution", "Multiplies the PS2 resolution (640\xC3\x97" "448)", false);
    combo("##resolution", c, &g_edit.resolution, kResolution, 4);
    y += 60.0f;
    c = rowTop(dl, y, "Aspect ratio", "16:9 widens the view at the sides (Hor+)", false);
    combo("##aspect", c, &g_edit.aspect, kAspect, 2);
    y += 60.0f;
    c = rowTop(dl, y, "Window mode", "F12 also switches it in game", false);
    combo("##window", c, &g_edit.windowMode, kWindow, 2);
    y += 60.0f;
    c = rowTop(dl, y, "Anti-aliasing", "Smooths jagged edges", false);
    combo("##aa", c, &g_edit.antiAliasing, kAa, 2);
    y += 60.0f;
    c = rowTop(dl, y, "Sharpening (CAS)", "Brings back detail after upscaling", false);
    slider(dl, "##sharpening", c, &g_edit.sharpening);
    y += 60.0f;
    c = rowTop(dl, y, "Vertical sync", "Prevents screen tearing", false);
    checkbox(dl, "##vsync", c, &g_edit.vsync, "On");
    y += 60.0f;
    c = rowTop(dl, y, "FPS counter", "Show it at startup (F10 toggles it)", true);
    checkbox(dl, "##fps", c, &g_edit.showFps, "Show");
}

void placeholderTab(ImDrawList *dl, const char *name)
{
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%s settings will appear here.", name);
    const ImVec2 sz = textSize(g_font.regular, 13, buf);
    const float top = kTitleH + kTabsH, bottom = kDesignH - kFooterH;
    text(dl, g_font.regular, 13, ImVec2(kLeftW + (kDesignW - kLeftW - sz.x) * 0.5f, top + (bottom - top - sz.y) * 0.5f), col(kMuted), buf);
}

void footer(ImDrawList *dl)
{
    const float top = kDesignH - kFooterH;
    rectFilled(dl, kLeftW, top, kDesignW, kDesignH, col(kPanel), 10.0f, ImDrawFlags_RoundCornersBottomRight);
    hline(dl, kLeftW, kDesignW, top + 0.5f, col(kLine));

    const float saveW = textSize(g_font.semibold, 14, "Save settings").x + 40.0f;
    const ImVec2 saveAt(kDesignW - 32.0f - saveW, top + 12);
    const Hit save = hit("##save", saveAt, ImVec2(saveW, 40));
    rectFilled(dl, saveAt.x, saveAt.y, saveAt.x + saveW, saveAt.y + 40, col(save.hovered ? kBronzeHi : kBronze), 8.0f);
    text(dl, g_font.semibold, 14, ImVec2(saveAt.x + 20, saveAt.y + 11), col(0x1A1412), "Save settings");
    if (save.clicked)
    {
        if (saveSettings(g_edit))
        {
            g_saved = g_edit;
            flash("Settings saved");
        }
        else
            flash("Could not write GoW2Launcher.ini", true);
    }

    const float cancelW = textSize(g_font.medium, 14, "Cancel").x + 36.0f;
    const ImVec2 cancelAt(saveAt.x - 10.0f - cancelW, top + 12);
    const Hit cancel = hit("##cancel", cancelAt, ImVec2(cancelW, 40));
    if (cancel.hovered)
        rectFilled(dl, cancelAt.x, cancelAt.y, cancelAt.x + cancelW, cancelAt.y + 40, col(kSideBg), 8.0f);
    rect(dl, cancelAt.x, cancelAt.y, cancelAt.x + cancelW, cancelAt.y + 40, col(cancel.hovered ? kSideEdgeHi : kFieldEdge), 8.0f);
    text(dl, g_font.medium, 14, ImVec2(cancelAt.x + 18, cancelAt.y + 11), col(kTextSoft), "Cancel");
    if (cancel.clicked)
    {
        g_edit = g_saved;
        flash("Changes discarded");
    }

    // status on the left: the last action for a moment, otherwise whether there is something to save
    const char *status = nullptr;
    unsigned statusCol = kMuted;
    if (GetTime() < g_flashUntil)
    {
        status = g_flash.c_str();
        statusCol = g_flashError ? kEmberEdge : kOk;
    }
    else if (!(g_edit == g_saved))
    {
        status = "Unsaved changes";
        statusCol = kBronze;
    }
    if (status)
    {
        const float sx = kLeftW + 32.0f;
        dl->AddCircleFilled(px(sx + 4, top + 32), px(3.5f), col(statusCol), 12);
        text(dl, g_font.regular, 12, ImVec2(sx + 14, top + 24), col(kTextSoft), status);
    }
}

void drawLauncher()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(kWidth), static_cast<float>(kHeight)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("##launcher", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    ImDrawList *dl = ImGui::GetWindowDrawList();

    rectFilled(dl, 0, 0, kDesignW, kDesignH, col(kGround), 10.0f);
    titleBar(dl);
    leftPanel(dl);
    tabs(dl);
    switch (g_tab)
    {
    case 0: graphicsTab(dl); break;
    case 1: placeholderTab(dl, "Performance"); break;
    case 2: placeholderTab(dl, "Controls"); break;
    default: placeholderTab(dl, "Audio"); break;
    }
    footer(dl);
    dl->AddRect(ImVec2(0.5f, 0.5f), ImVec2(kWidth - 0.5f, kHeight - 0.5f), col(kFrame), px(10.0f));
    ImGui::End();
}

void loadFonts()
{
    const std::string dir = std::string(GetApplicationDirectory()) + "fonts/";
    ImGuiIO &io = ImGui::GetIO();
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    g_font.regular = io.Fonts->AddFontFromFileTTF((dir + "Inter-Regular.ttf").c_str(), px(13.0f), &cfg);
    g_font.medium = io.Fonts->AddFontFromFileTTF((dir + "Inter-Medium.ttf").c_str(), px(13.0f), &cfg);
    g_font.semibold = io.Fonts->AddFontFromFileTTF((dir + "Inter-SemiBold.ttf").c_str(), px(13.0f), &cfg);
    g_font.display = io.Fonts->AddFontFromFileTTF((dir + "Cinzel-Bold.ttf").c_str(), px(20.0f), &cfg);
    // a missing file falls back to ImGui's own font so the window still opens
    ImFont *fallback = nullptr;
    for (ImFont **f : {&g_font.regular, &g_font.medium, &g_font.semibold, &g_font.display})
        if (!*f)
        {
            if (!fallback)
                fallback = io.Fonts->AddFontDefault();
            *f = fallback;
        }
    io.FontDefault = g_font.medium;
}

void applyStyle()
{
    ImGuiStyle &s = ImGui::GetStyle();
    s.FrameRounding = px(6.0f);
    s.PopupRounding = px(6.0f);
    s.FrameBorderSize = 1.0f;
    s.PopupBorderSize = 1.0f;
    s.FramePadding = px(10.0f, 11.5f); // 13 px text + 2 x 11.5 = the mockup's 36 px fields
    s.ItemSpacing = px(8.0f, 4.0f);
    s.WindowPadding = px(6.0f, 6.0f);
    s.SelectableTextAlign = ImVec2(0.0f, 0.5f);
    ImVec4 *c = s.Colors;
    c[ImGuiCol_Text] = colv(kText);
    c[ImGuiCol_WindowBg] = colv(kGround, 0.0f);
    c[ImGuiCol_PopupBg] = colv(kSideBg);
    c[ImGuiCol_Border] = colv(kFieldEdge);
    c[ImGuiCol_FrameBg] = colv(kField);
    c[ImGuiCol_FrameBgHovered] = colv(kFieldHi);
    c[ImGuiCol_FrameBgActive] = colv(kFieldHi);
    c[ImGuiCol_Button] = colv(kField);
    c[ImGuiCol_ButtonHovered] = colv(kFieldHi);
    c[ImGuiCol_ButtonActive] = colv(kFieldHi);
    c[ImGuiCol_Header] = colv(kBronze, 0.22f);
    c[ImGuiCol_HeaderHovered] = colv(kBronze, 0.30f);
    c[ImGuiCol_HeaderActive] = colv(kBronze, 0.40f);
    c[ImGuiCol_NavCursor] = colv(kBronze);
}
} // namespace

int main()
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TRANSPARENT | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(kWidth, kHeight, "God of War II \xE2\x80\x94 Native Port Launcher");
    SetExitKey(KEY_NULL); // Esc does not close the launcher
    SetTargetFPS(60);

    g_saved = loadSettings();
    g_edit = g_saved;

    rlImGuiBeginInitImGui();
    ImGui::GetIO().IniFilename = nullptr; // no imgui.ini next to the launcher
    loadFonts();
    applyStyle();
    rlImGuiEndInitImGui();

    while (!WindowShouldClose() && !g_quit)
    {
        BeginDrawing();
        ClearBackground(BLANK); // transparent outside the rounded frame
        // The window is transparent (rounded corners). With the usual blend the framebuffer's alpha also drops at
        // every antialiased edge drawn over the opaque ground, and the desktop showed through the text (tinted
        // glyphs). Alpha accumulates instead (ONE, ONE_MINUS_SRC_ALPHA): opaque once the ground is drawn.
        rlSetBlendFactorsSeparate(0x0302, 0x0303, 1, 0x0303, 0x8006, 0x8006); // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, ..., GL_FUNC_ADD
        BeginBlendMode(BLEND_CUSTOM_SEPARATE);
        rlImGuiBegin();
        drawLauncher();
        rlImGuiEnd();
        EndBlendMode();
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
