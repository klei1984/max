/* Copyright (c) 2022 M.A.X. Port Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "window_manager.hpp"

#include "game_manager.hpp"
#include "gfx.hpp"
#include "helpmenu.hpp"
#include "resource_manager.hpp"

#define WINDOW_RECT(ulx, uly, lrx, lry) \
    { ulx, uly, lrx, lry }

#define WINDOW_ITEM(rect, unknown, id, buffer, name) \
    { rect, unknown, id, buffer }

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

static void WindowManager_SwapSystemPalette(ImageBigHeader *image);
static void WindowManager_ScaleWindows();
static bool WindowManager_CustomSpriteScaler(ResourceID id, ImageBigHeader *image, WindowInfo *window, int16_t pitch,
                                             int32_t ulx, int32_t uly);
static void WindowManager_ResizeSimpleImage(ResourceID id, double scale);
static void WindowManager_ScaleWindow(int32_t wid, double scale);
static void WindowManager_ScaleButtonResource(ResourceID id, double scale);

Color *WindowManager_SystemPalette;
Color *WindowManager_ColorPalette;

int32_t WindowManager_WindowWidth;
int32_t WindowManager_WindowHeight;
int32_t WindowManager_MapWidth;
int32_t WindowManager_MapHeight;

static WindowInfo windows[WINDOW_COUNT] = {
    WINDOW_ITEM(WINDOW_RECT(0, 0, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1), WINDOW_WIDTH, 0, nullptr, WINDOW_MAIN_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(-1, -1, -1, -1), WINDOW_WIDTH, 0, nullptr, WINDOW_POPUP_BUTTONS),
    WINDOW_ITEM(WINDOW_RECT(-1, -1, -1, -1), WINDOW_WIDTH, 0, nullptr, WINDOW_MESSAGE_BOX),
    WINDOW_ITEM(WINDOW_RECT(390, 2, 461, 22), WINDOW_WIDTH, 0, nullptr, WINDOW_ENDTURN_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(17, 4, 83, 21), WINDOW_WIDTH, 0, nullptr, WINDOW_FILES_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(86, 4, 152, 21), WINDOW_WIDTH, 0, nullptr, WINDOW_PREFS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(10, 29, 137, 156), WINDOW_WIDTH, 0, nullptr, WINDOW_CORNER_FLIC),
    WINDOW_ITEM(WINDOW_RECT(146, 123, 164, 140), WINDOW_WIDTH, 0, nullptr, WINDOW_PLAY_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(146, 143, 164, 161), WINDOW_WIDTH, 0, nullptr, WINDOW_PAUSE_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(10, 173, 162, 220), WINDOW_WIDTH, 0, nullptr, WINDOW_STAT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(10, 173, 162, 184), WINDOW_WIDTH, 0, nullptr, WINDOW_STAT_ROW_1),
    WINDOW_ITEM(WINDOW_RECT(10, 185, 162, 196), WINDOW_WIDTH, 0, nullptr, WINDOW_STAT_ROW_2),
    WINDOW_ITEM(WINDOW_RECT(10, 197, 162, 208), WINDOW_WIDTH, 0, nullptr, WINDOW_STAT_ROW_3),
    WINDOW_ITEM(WINDOW_RECT(10, 209, 162, 220), WINDOW_WIDTH, 0, nullptr, WINDOW_STAT_ROW_4),
    WINDOW_ITEM(WINDOW_RECT(4, 227, 23, 249), WINDOW_WIDTH, 0, nullptr, WINDOW_CENTER_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(32, 227, 51, 248), WINDOW_WIDTH, 0, nullptr, WINDOW_LOCK_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(60, 228, 97, 249), WINDOW_WIDTH, 0, nullptr, WINDOW_PRE_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(99, 228, 124, 249), WINDOW_WIDTH, 0, nullptr, WINDOW_DONE_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(125, 228, 162, 249), WINDOW_WIDTH, 0, nullptr, WINDOW_NXT_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(21, 250, 45, 273), WINDOW_WIDTH, 0, nullptr, WINDOW_HELP_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(51, 252, 98, 270), WINDOW_WIDTH, 0, nullptr, WINDOW_REPORTS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(102, 252, 149, 270), WINDOW_WIDTH, 0, nullptr, WINDOW_CHAT_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(2, 275, 19, 288), WINDOW_WIDTH, 0, nullptr, WINDOW_ZOOM_PLUS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(20, 275, 149, 288), WINDOW_WIDTH, 0, nullptr, WINDOW_ZOOM_SLIDER_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(150, 275, 169, 288), WINDOW_WIDTH, 0, nullptr, WINDOW_ZOOM_MINUS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(20, 275, 149, 293), WINDOW_WIDTH, 0, nullptr, WINDOW_ZOOM_SLIDER_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(4, 298, 58, 313), WINDOW_WIDTH, 0, nullptr, WINDOW_SURVEY_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(4, 314, 58, 329), WINDOW_WIDTH, 0, nullptr, WINDOW_STATUS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(4, 330, 58, 347), WINDOW_WIDTH, 0, nullptr, WINDOW_COLORS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(59, 298, 113, 313), WINDOW_WIDTH, 0, nullptr, WINDOW_HITS_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(59, 314, 113, 329), WINDOW_WIDTH, 0, nullptr, WINDOW_AMMO_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(59, 330, 113, 347), WINDOW_WIDTH, 0, nullptr, WINDOW_RANGE_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(114, 298, 168, 313), WINDOW_WIDTH, 0, nullptr, WINDOW_SCAN_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(114, 314, 168, 329), WINDOW_WIDTH, 0, nullptr, WINDOW_GRID_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(114, 330, 168, 347), WINDOW_WIDTH, 0, nullptr, WINDOW_NAME_BUTTON),
    WINDOW_ITEM(WINDOW_RECT(15, 356, 126, 467), WINDOW_WIDTH, 0, nullptr, WINDOW_MINIMAP),
    WINDOW_ITEM(WINDOW_RECT(136, 387, 162, 412), WINDOW_WIDTH, 0, nullptr, WINDOW_2X_MINIMAP),
    WINDOW_ITEM(WINDOW_RECT(136, 413, 162, 439), WINDOW_WIDTH, 0, nullptr, WINDOW_TNT_MINIMAP),
    WINDOW_ITEM(WINDOW_RECT(180, 18, 7 * 64 + 180 - 1, 7 * 64 + 18 - 1), WINDOW_WIDTH, 0, nullptr, WINDOW_MAIN_MAP),
    WINDOW_ITEM(WINDOW_RECT(50, 0, 590, 8), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_UP_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(590, 0, 639, 8), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_UP_RIGHT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(632, 0, 639, 50), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_RIGHT_UP_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(632, 50, 639, 430), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_RIGHT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(632, 430, 639, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_RIGHT_DOWN_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(590, 472, 639, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_DOWN_RIGHT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(50, 472, 590, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_DOWN_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 472, 50, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_DOWN_LEFT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 430, 8, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_LEFT_DOWN_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 50, 8, 430), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_LEFT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 0, 8, 50), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_LEFT_UP_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 0, 50, 8), WINDOW_WIDTH, 0, nullptr, WINDOW_SCROLL_UP_LEFT_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(242, 456, 331, 476), WINDOW_WIDTH, 0, nullptr, WINDOW_COORDINATES_DISPLAY),
    WINDOW_ITEM(WINDOW_RECT(338, 456, 559, 476), WINDOW_WIDTH, 0, nullptr, WINDOW_UNIT_DESCRIPTION_DISPLAY),
    WINDOW_ITEM(WINDOW_RECT(468, 2, 528, 22), WINDOW_WIDTH, 0, nullptr, WINDOW_TURN_COUNTER_DISPLAY),
    WINDOW_ITEM(WINDOW_RECT(534, 2, 594, 22), WINDOW_WIDTH, 0, nullptr, WINDOW_TURN_TIMER_DISPLAY),
    WINDOW_ITEM(WINDOW_RECT(384, 0, 639, 24), WINDOW_WIDTH, 0, nullptr, WINDOW_TOP_INSTRUMENTS_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(240, 456, 585, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_BOTTOM_INSTRUMENTS_WINDOW),
    WINDOW_ITEM(WINDOW_RECT(0, 0, 178, 238), WINDOW_WIDTH, 0, nullptr, WINDOW_INTERFACE_PANEL_TOP),
    WINDOW_ITEM(WINDOW_RECT(0, 239, 178, 479), WINDOW_WIDTH, 0, nullptr, WINDOW_INTERFACE_PANEL_BOTTOM)};

void WindowManager_ResizeSimpleImage(ResourceID id, double scale) {
    uint8_t *resource = ResourceManager_LoadResource(id);
    ImageSimpleHeader *image = reinterpret_cast<ImageSimpleHeader *>(resource);
    int32_t data_size;
    Rect map_frame;

    image = WindowManager_RescaleSimpleImage(image, GFX_SCALE_DENOMINATOR / scale);
    data_size = image->width * image->height + sizeof(image->width) * 4;
    ResourceManager_Realloc(id, reinterpret_cast<uint8_t *>(image), data_size);
}

void WindowManager_ScaleWindow(int32_t wid, double scale) {
    WindowInfo *const screen = &windows[WINDOW_MAIN_WINDOW];
    WindowInfo *const window = &windows[wid];

    window->window.ulx *= scale;
    window->window.uly *= scale;
    window->window.lrx *= scale;
    window->window.lry *= scale;
    window->width = screen->width;
    window->buffer = &screen->buffer[window->window.uly * window->width + window->window.ulx];
}

void WindowManager_ScaleButtonResource(ResourceID id, double scale) {
    WindowManager_ResizeSimpleImage(id, scale);
    WindowManager_ResizeSimpleImage(static_cast<ResourceID>(id + 1), scale);
}

void WindowManager_ScaleResources() {
    double scale = WindowManager_GetScale();

    WindowManager_ResizeSimpleImage(PANELTOP, scale);
    WindowManager_ResizeSimpleImage(PANELBTM, scale);

    for (int32_t i = PNLSEQ_1; i <= PNLSEQ_5; ++i) {
        WindowManager_ResizeSimpleImage(static_cast<ResourceID>(i), scale);
    }

    for (int32_t i = BPNLSQ_1; i <= BPNLSQ_4; ++i) {
        WindowManager_ResizeSimpleImage(static_cast<ResourceID>(i), scale);
    }

    WindowManager_ResizeSimpleImage(XYPOS, scale);
    WindowManager_ResizeSimpleImage(UNITNAME, scale);
    WindowManager_ResizeSimpleImage(TURNS, scale);
    WindowManager_ResizeSimpleImage(TIMER, scale);
    WindowManager_ResizeSimpleImage(ENDTRN_U, scale);
    WindowManager_ResizeSimpleImage(R_ENDT_D, scale);
    WindowManager_ResizeSimpleImage(G_ENDT_D, scale);
    WindowManager_ResizeSimpleImage(B_ENDT_D, scale);
    WindowManager_ResizeSimpleImage(W_ENDT_D, scale);

    WindowManager_ScaleButtonResource(FILES_OF, scale);
    WindowManager_ScaleButtonResource(PREF_OFF, scale);
    WindowManager_ScaleButtonResource(PLAY_OF, scale);
    WindowManager_ScaleButtonResource(PAUSE_OF, scale);
    WindowManager_ScaleButtonResource(FIND_OFF, scale);
    WindowManager_ScaleButtonResource(PREV_OF, scale);
    WindowManager_ScaleButtonResource(UDONE_OF, scale);
    WindowManager_ScaleButtonResource(NEXT_OF, scale);
    WindowManager_ScaleButtonResource(HELP_OF, scale);
    WindowManager_ScaleButtonResource(REPT_OFF, scale);
    WindowManager_ScaleButtonResource(CHAT_OFF, scale);
    WindowManager_ScaleButtonResource(GOAL_OFF, scale);
    WindowManager_ScaleButtonResource(SURV_OFF, scale);
    WindowManager_ScaleButtonResource(STAT_OFF, scale);
    WindowManager_ScaleButtonResource(COLOR_OF, scale);
    WindowManager_ScaleButtonResource(HITS_OF, scale);
    WindowManager_ScaleButtonResource(AMMO_OF, scale);
    WindowManager_ScaleButtonResource(RANG_OFF, scale);
    WindowManager_ScaleButtonResource(VISN_OFF, scale);
    WindowManager_ScaleButtonResource(GRID_OFF, scale);
    WindowManager_ScaleButtonResource(NAMES_UP, scale);
    WindowManager_ScaleButtonResource(LOCK_OF, scale);
    WindowManager_ScaleButtonResource(MIN2X_OF, scale);
    WindowManager_ScaleButtonResource(MINFL_OF, scale);
    WindowManager_ScaleButtonResource(PNLHLP_U, scale);
    WindowManager_ScaleButtonResource(PNLCAN_U, scale);

    WindowManager_ResizeSimpleImage(ZOOMPNL1, scale);
    WindowManager_ResizeSimpleImage(ZOOMPNL2, scale);
    WindowManager_ResizeSimpleImage(ZOOMPNL3, scale);
    WindowManager_ResizeSimpleImage(ZOOMPNL4, scale);
    WindowManager_ResizeSimpleImage(ZOOMPNL5, scale);
    WindowManager_ResizeSimpleImage(ZOOMPNL6, scale);
    WindowManager_ResizeSimpleImage(ZOOMPNL7, scale);
    WindowManager_ResizeSimpleImage(ZOOMPTR, scale);
}

void WindowManager_ScaleWindows() {
    WindowInfo *const screen = &windows[WINDOW_MAIN_WINDOW];
    int32_t screen_width = WindowManager_GetWidth(screen);
    int32_t screen_height = WindowManager_GetHeight(screen);
    double scale = WindowManager_GetScale();

    {
        WindowInfo *const wmap = &windows[WINDOW_MAIN_MAP];

        const int32_t window_left = wmap->window.ulx;
        const int32_t window_tile_count_x = (WINDOW_WIDTH - window_left) / GFX_MAP_TILE_SIZE;
        const int32_t window_right = WINDOW_WIDTH - window_left - window_tile_count_x * GFX_MAP_TILE_SIZE;
        const int32_t window_top = wmap->window.uly;
        const int32_t window_tile_count_y = (WINDOW_HEIGHT - window_top) / GFX_MAP_TILE_SIZE;
        const int32_t window_bottom = WINDOW_HEIGHT - window_top - window_tile_count_y * GFX_MAP_TILE_SIZE;

        const int32_t map_left = window_left * scale;
        const int32_t map_right = window_right * scale;
        const int32_t map_top = window_top * scale;
        const int32_t map_bottom = window_bottom * scale;
        const int32_t map_width = screen_width - map_left - map_right;
        const int32_t map_height = screen_height - map_top - map_bottom;

        wmap->window.ulx = map_left;
        wmap->window.uly = map_top;
        wmap->window.lrx = map_left + map_width - 1;
        wmap->window.lry = map_top + map_height - 1;
        wmap->buffer = &screen->buffer[screen->width * wmap->window.uly + wmap->window.ulx];

        WindowManager_MapWidth = map_width;
        WindowManager_MapHeight = map_height;

        WindowInfo *const wpt = &windows[WINDOW_INTERFACE_PANEL_TOP];
        WindowInfo *const wpb = &windows[WINDOW_INTERFACE_PANEL_BOTTOM];

        const int32_t wpt_width = (wpt->window.lrx - wpt->window.ulx) * scale;
        const int32_t wpt_height = (wpt->window.lry - wpt->window.uly) * scale;

        wpt->window = {0, 0, wpt_width, wpt_height};
        wpb->window = {0, wpt_height + 1, wpt_width, screen->window.lry};

        wpt->buffer = &screen->buffer[wpt->window.uly * wpt->width + wpt->window.ulx];
        wpb->buffer = &screen->buffer[wpb->window.uly * wpb->width + wpb->window.ulx];
    }

    {
        WindowInfo *const wu = &windows[WINDOW_SCROLL_UP_WINDOW];
        WindowInfo *const wur = &windows[WINDOW_SCROLL_UP_RIGHT_WINDOW];
        WindowInfo *const wru = &windows[WINDOW_SCROLL_RIGHT_UP_WINDOW];
        WindowInfo *const wr = &windows[WINDOW_SCROLL_RIGHT_WINDOW];
        WindowInfo *const wrd = &windows[WINDOW_SCROLL_RIGHT_DOWN_WINDOW];
        WindowInfo *const wdr = &windows[WINDOW_SCROLL_DOWN_RIGHT_WINDOW];
        WindowInfo *const wd = &windows[WINDOW_SCROLL_DOWN_WINDOW];
        WindowInfo *const wdl = &windows[WINDOW_SCROLL_DOWN_LEFT_WINDOW];
        WindowInfo *const wld = &windows[WINDOW_SCROLL_LEFT_DOWN_WINDOW];
        WindowInfo *const wl = &windows[WINDOW_SCROLL_LEFT_WINDOW];
        WindowInfo *const wlu = &windows[WINDOW_SCROLL_LEFT_UP_WINDOW];
        WindowInfo *const wul = &windows[WINDOW_SCROLL_UP_LEFT_WINDOW];

        const int32_t corner_size = wu->window.ulx * scale;
        const int32_t corner_width = wu->window.lry * scale;

        const int32_t x0 = 0;
        const int32_t x1 = corner_size;
        const int32_t x2 = screen_width - corner_size;
        const int32_t x3 = screen_width - 1;

        const int32_t y0 = 0;
        const int32_t y1 = corner_size;
        const int32_t y2 = screen_height - corner_size;
        const int32_t y3 = screen_height - 1;

        wu->window = {x1, y0, x2, corner_width};
        wur->window = {x2, y0, x3, corner_width};
        wru->window = {x3 - corner_width + 1, y0, x3, y1};
        wr->window = {x3 - corner_width + 1, y1, x3, y2};
        wrd->window = {x3 - corner_width + 1, y2, x3, y3};
        wdr->window = {x2, y3 - corner_width + 1, x3, y3};
        wd->window = {x1, y3 - corner_width + 1, x2, y3};
        wdl->window = {x0, y3 - corner_width + 1, y1, y3};
        wld->window = {x0, y2, corner_width, y3};
        wl->window = {x0, y1, corner_width, y2};
        wlu->window = {x0, y0, corner_width, y1};
        wul->window = {x0, y0, y1, corner_width};

        wu->buffer = &screen->buffer[wu->window.uly * wu->width + wu->window.ulx];
        wur->buffer = &screen->buffer[wur->window.uly * wur->width + wur->window.ulx];
        wru->buffer = &screen->buffer[wru->window.uly * wru->width + wru->window.ulx];
        wr->buffer = &screen->buffer[wr->window.uly * wr->width + wr->window.ulx];
        wrd->buffer = &screen->buffer[wrd->window.uly * wrd->width + wrd->window.ulx];
        wdr->buffer = &screen->buffer[wdr->window.uly * wdr->width + wdr->window.ulx];
        wd->buffer = &screen->buffer[wd->window.uly * wd->width + wd->window.ulx];
        wdl->buffer = &screen->buffer[wdl->window.uly * wdl->width + wdl->window.ulx];
        wld->buffer = &screen->buffer[wld->window.uly * wld->width + wld->window.ulx];
        wl->buffer = &screen->buffer[wl->window.uly * wl->width + wl->window.ulx];
        wlu->buffer = &screen->buffer[wlu->window.uly * wlu->width + wlu->window.ulx];
        wul->buffer = &screen->buffer[wul->window.uly * wul->width + wul->window.ulx];
    }

    {
        WindowInfo *const wti = &windows[WINDOW_TOP_INSTRUMENTS_WINDOW];
        WindowInfo *const wbi = &windows[WINDOW_BOTTOM_INSTRUMENTS_WINDOW];
        WindowInfo *const wcd = &windows[WINDOW_COORDINATES_DISPLAY];
        WindowInfo *const wud = &windows[WINDOW_UNIT_DESCRIPTION_DISPLAY];
        WindowInfo *const wtc = &windows[WINDOW_TURN_COUNTER_DISPLAY];
        WindowInfo *const wtt = &windows[WINDOW_TURN_TIMER_DISPLAY];
        WindowInfo *const wet = &windows[WINDOW_ENDTURN_BUTTON];
        WindowInfo *const wmap = &windows[WINDOW_MAIN_MAP];

        const int32_t bottom_instrument_width = 327 * scale;

        const int32_t wti_width = (wti->window.lrx - wti->window.ulx + 1) * scale;
        const int32_t wti_height = (wti->window.lry - wti->window.uly + 1) * scale;

        const int32_t wbi_width = (wbi->window.lrx - wbi->window.ulx + 1) * scale;
        const int32_t wbi_height = (wbi->window.lry - wbi->window.uly + 1) * scale;

        const int32_t wcd_offset = (wcd->window.ulx - wbi->window.ulx) * scale;
        const int32_t wcd_width = (wcd->window.lrx - wcd->window.ulx + 1) * scale;
        const int32_t wcd_height = (wcd->window.lry - wcd->window.uly + 1) * scale;

        const int32_t wud_offset = (wud->window.ulx - wbi->window.ulx) * scale;
        const int32_t wud_width = (wud->window.lrx - wud->window.ulx + 1) * scale;
        const int32_t wud_height = (wud->window.lry - wud->window.uly + 1) * scale;

        const int32_t wet_offset = (wet->window.ulx - wti->window.ulx) * scale;
        const int32_t wet_width = (wet->window.lrx - wet->window.ulx + 1) * scale;
        const int32_t wet_height = (wet->window.lry - wet->window.uly + 1) * scale;

        const int32_t wtc_offset = (wtc->window.ulx - wti->window.ulx) * scale;
        const int32_t wtc_width = (wtc->window.lrx - wtc->window.ulx + 1) * scale;
        const int32_t wtc_height = (wtc->window.lry - wtc->window.uly + 1) * scale;

        const int32_t wtt_offset = (wtt->window.ulx - wti->window.ulx) * scale;
        const int32_t wtt_width = (wtt->window.lrx - wtt->window.ulx + 1) * scale;
        const int32_t wtt_height = (wtt->window.lry - wtt->window.uly + 1) * scale;

        const int32_t wmap_width = wmap->window.lrx - wmap->window.ulx + 1;

        wti->window.ulx = screen_width - wti_width;
        wti->window.uly = wti->window.uly * scale;
        wti->window.lrx = screen_width - 1;
        wti->window.lry = wti->window.lry * scale;

        wbi->window.ulx = (wmap_width - bottom_instrument_width) / 2 + wmap->window.ulx;
        wbi->window.lrx = wbi->window.ulx + wbi_width;
        wbi->window.uly = screen->window.lry - (wbi_height - 1);
        wbi->window.lry = screen->window.lry;

        wcd->window.ulx = wbi->window.ulx + wcd_offset;
        wcd->window.lrx = wcd->window.ulx + wcd_width;
        wcd->window.uly = wbi->window.uly;
        wcd->window.lry = wcd->window.uly + wcd_height;

        wud->window.ulx = wbi->window.ulx + wud_offset;
        wud->window.lrx = wud->window.ulx + wud_width;
        wud->window.uly = wbi->window.uly;
        wud->window.lry = wud->window.uly + wud_height;

        wet->window.ulx = wti->window.ulx + wet_offset;
        wet->window.lrx = wet->window.ulx + wet_width - 1;
        wet->window.uly = wet->window.uly * scale;
        wet->window.lry = wet->window.uly + wet_height - 1;

        wtc->window.ulx = wti->window.ulx + wtc_offset;
        wtc->window.lrx = wtc->window.ulx + wtc_width - 1;
        wtc->window.uly = wtc->window.uly * scale;
        wtc->window.lry = wtc->window.uly + wtc_height - 1;

        wtt->window.ulx = wti->window.ulx + wtt_offset;
        wtt->window.lrx = wtt->window.ulx + wtt_width - 1;
        wtt->window.uly = wtt->window.uly * scale;
        wtt->window.lry = wtt->window.uly + wtt_height - 1;

        wti->buffer = &screen->buffer[wti->window.uly * wti->width + wti->window.ulx];
        wbi->buffer = &screen->buffer[wbi->window.uly * wbi->width + wbi->window.ulx];
        wcd->buffer = &screen->buffer[wcd->window.uly * wcd->width + wcd->window.ulx];
        wud->buffer = &screen->buffer[wud->window.uly * wud->width + wud->window.ulx];
        wet->buffer = &screen->buffer[wet->window.uly * wet->width + wet->window.ulx];
        wtc->buffer = &screen->buffer[wtc->window.uly * wtc->width + wtc->window.ulx];
        wtt->buffer = &screen->buffer[wtt->window.uly * wtt->width + wtc->window.ulx];
    }

    {
        WindowInfo *const wmm = &windows[WINDOW_MINIMAP];

        const int32_t wmm_width = (wmm->window.lrx - wmm->window.ulx + 1) * scale;
        const int32_t wmm_height = (wmm->window.lry - wmm->window.uly + 1) * scale;

        wmm->window.ulx = wmm->window.ulx * scale;
        wmm->window.lrx = wmm->window.ulx + wmm_width - 1;
        wmm->window.uly = wmm->window.uly * scale;
        wmm->window.lry = wmm->window.uly + wmm_height - 1;

        wmm->width = screen->width;
        wmm->buffer = &screen->buffer[wmm->window.uly * wmm->width + wmm->window.ulx];
    }

    {
        WindowManager_ScaleWindow(WINDOW_FILES_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_PREFS_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_PLAY_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_PAUSE_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_CENTER_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_PRE_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_DONE_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_NXT_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_HELP_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_REPORTS_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_CHAT_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_SURVEY_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_STATUS_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_COLORS_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_HITS_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_AMMO_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_RANGE_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_SCAN_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_GRID_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_NAME_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_LOCK_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_2X_MINIMAP, scale);
        WindowManager_ScaleWindow(WINDOW_TNT_MINIMAP, scale);
    }

    {
        WindowInfo *const wcf = &windows[WINDOW_CORNER_FLIC];

        const int32_t wcf_width = (wcf->window.lrx - wcf->window.ulx + 1) * scale;
        const int32_t wcf_height = (wcf->window.lry - wcf->window.uly + 1) * scale;

        wcf->window.ulx = wcf->window.ulx * scale;
        wcf->window.lrx = wcf->window.ulx + wcf_width - 1;
        wcf->window.uly = wcf->window.uly * scale;
        wcf->window.lry = wcf->window.uly + wcf_height - 1;

        wcf->width = screen->width;
        wcf->buffer = &screen->buffer[wcf->window.uly * wcf->width + wcf->window.ulx];
    }

    {
        WindowManager_ScaleWindow(WINDOW_STAT_WINDOW, scale);
        WindowManager_ScaleWindow(WINDOW_STAT_ROW_1, scale);
        WindowManager_ScaleWindow(WINDOW_STAT_ROW_2, scale);
        WindowManager_ScaleWindow(WINDOW_STAT_ROW_3, scale);
        WindowManager_ScaleWindow(WINDOW_STAT_ROW_4, scale);
    }

    {
        WindowManager_ScaleWindow(WINDOW_ZOOM_PLUS_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_ZOOM_SLIDER_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_ZOOM_MINUS_BUTTON, scale);
        WindowManager_ScaleWindow(WINDOW_ZOOM_SLIDER_WINDOW, scale);
    }
}

bool WindowManager_CustomSpriteScaler(ResourceID id, ImageBigHeader *image, WindowInfo *window, int16_t pitch,
                                      int32_t ulx, int32_t uly) {
    bool result = false;

    /// \todo Support ULX, ULY offsets

    switch (id) {
        case FRAMEPIC: {
            const double scale = WindowManager_GetScale();
            WindowInfo *const screen = &windows[WINDOW_MAIN_WINDOW];

            const int32_t left = 180;
            const int32_t right = 18;
            const int32_t scaled_left = 180 * scale;
            const int32_t scaled_right = 18 * scale;

            const Rect old_l = {0, 0, left - 1, WINDOW_HEIGHT - 1};
            const Rect old_r = {WINDOW_WIDTH - 1 - right, old_l.uly, WINDOW_WIDTH - 1, old_l.lry};
            const Rect old_m = {old_l.lrx + 1, old_l.uly, old_r.ulx - 1, old_l.lry};

            const Rect new_l = {0, 0, scaled_left - 1, screen->window.lry};
            const Rect new_r = {window->window.lrx - scaled_right, new_l.uly, window->window.lrx, new_l.lry};
            const Rect new_m = {new_l.lrx + 1, new_l.uly, new_r.ulx - 1, new_l.lry};

            uint8_t *buffer = new (std::nothrow) uint8_t[image->width * image->height];

            WindowManager_DecodeBigImage(image, buffer, 0, 0, image->width);

            cscale(&buffer[image->width * old_l.uly + old_l.ulx], old_l.lrx + 1 - old_l.ulx, old_l.lry - old_l.uly + 1,
                   image->width, &window->buffer[window->width * new_l.uly + new_l.ulx], new_l.lrx + 2 - new_l.ulx,
                   new_l.lry - new_l.uly + 1, window->width);

            cscale(&buffer[image->width * old_m.uly + old_m.ulx], old_m.lrx + 1 - old_m.ulx, old_m.lry - old_m.uly + 1,
                   image->width, &window->buffer[window->width * new_m.uly + new_m.ulx], new_m.lrx + 2 - new_m.ulx,
                   new_m.lry - new_m.uly + 1, window->width);

            cscale(&buffer[image->width * old_r.uly + old_r.ulx], old_r.lrx + 1 - old_r.ulx, old_r.lry - old_r.uly + 1,
                   image->width, &window->buffer[window->width * new_r.uly + new_r.ulx], new_r.lrx + 2 - new_r.ulx,
                   new_r.lry - new_r.uly + 1, window->width);

            delete[] buffer;

            result = true;
        } break;
    }

    return result;
}

int32_t WindowManager_Init() {
    uint8_t result;
    WinID wid;

    if (win_init(Svga_Init, Svga_Deinit, 0)) {
        return EXIT_CODE_SCREEN_INIT_FAILED;
    }

    WindowManager_WindowWidth = Svga_GetScreenWidth();
    WindowManager_WindowHeight = Svga_GetScreenHeight();

    wid = win_add(0, 0, WindowManager_WindowWidth, WindowManager_WindowHeight, COLOR_BLACK, 2);

    if (wid == -1) {
        result = EXIT_CODE_INSUFFICIENT_MEMORY;

    } else {
        windows[WINDOW_MAIN_WINDOW].width = WindowManager_WindowWidth;
        windows[WINDOW_MAIN_WINDOW].window.lrx = WindowManager_WindowWidth - 1;
        windows[WINDOW_MAIN_WINDOW].window.lry = WindowManager_WindowHeight - 1;

        windows[WINDOW_MAIN_WINDOW].id = wid;
        windows[WINDOW_MAIN_WINDOW].buffer = win_get_buf(wid);

        if (windows[WINDOW_MAIN_WINDOW].buffer) {
            for (int32_t i = 0; i < WINDOW_COUNT; ++i) {
                windows[i].id = wid;
                windows[i].buffer =
                    &windows[WINDOW_MAIN_WINDOW]
                         .buffer[windows[WINDOW_MAIN_WINDOW].width * windows[i].window.uly + windows[i].window.ulx];
                windows[i].width = windows[WINDOW_MAIN_WINDOW].width;
            }

            WindowManager_ScaleWindows();

            WindowManager_SystemPalette = new (std::nothrow) uint8_t[PALETTE_STRIDE * PALETTE_SIZE];
            memset(WindowManager_SystemPalette, 0, PALETTE_STRIDE * PALETTE_SIZE);

            result = EXIT_CODE_NO_ERROR;

        } else {
            result = EXIT_CODE_INSUFFICIENT_MEMORY;
        }
    }

    return result;
}

WindowInfo *WindowManager_GetWindow(uint8_t id) {
    WindowInfo *window;

    if (id < WINDOW_COUNT) {
        window = &windows[id];
    } else {
        window = nullptr;
    }

    return window;
}

void WindowManager_ClearWindow() {
    WindowInfo *window;

    WindowManager_FadeOut(150);
    window = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
    Color_SetSystemPalette(WindowManager_SystemPalette);
    memset(window->buffer, 0, (window->window.lry + 1) * (window->window.lrx + 1));
    win_draw(window->id);
}

void WindowManager_FadeOut(int32_t time_limit) {
    Color_FadeSystemPalette(Color_GetColorPalette(), WindowManager_SystemPalette, time_limit);
}

void WindowManager_FadeIn(int32_t time_limit) {
    Color_FadeSystemPalette(WindowManager_SystemPalette, Color_GetColorPalette(), time_limit);
}

void WindowManager_SwapSystemPalette(ImageBigHeader *image) {
    uint8_t *palette;

    WindowManager_ClearWindow();
    palette = Color_GetSystemPalette();

    for (int32_t i = 0; i < (PALETTE_STRIDE * PALETTE_SIZE); ++i) {
        palette[i] = image->palette[i] / 4;
    }

    Color_SetSystemPalette(palette);

    WindowManager_ColorPalette = Color_GetColorPalette();
    memcpy(WindowManager_ColorPalette, palette, PALETTE_STRIDE * PALETTE_SIZE);
    Color_SetColorPalette(WindowManager_ColorPalette);
}

void WindowManager_LoadPalette(ResourceID id) {
    uint8_t *resource;
    ImageBigHeader *image;

    resource = ResourceManager_ReadResource(id);
    image = reinterpret_cast<ImageBigHeader *>(resource);

    if (image) {
        WindowManager_SwapSystemPalette(image);
    }

    delete[] resource;
}

void WindowManager_DecodeBigImage(struct ImageBigHeader *image, uint8_t *buffer, int32_t ulx, int32_t uly,
                                  int32_t pitch) {
    int32_t image_height;
    int32_t image_width;
    uint8_t *image_data;
    int32_t buffer_position;

    image_width = image->width;
    image_height = image->height;

    buffer_position = ulx + pitch * uly;
    image_data = &image->transparent_color;

    process_bk();

    for (int32_t line_count = 0; line_count < image_height; ++line_count) {
        int16_t opt_word;

        for (int32_t line_position = 0; line_position < image_width; line_position += opt_word) {
            opt_word = *reinterpret_cast<int16_t *>(image_data);
            image_data += sizeof(opt_word);

            if (opt_word > 0) {
                memcpy(&buffer[buffer_position + line_position], image_data, opt_word);

                image_data += opt_word;

            } else {
                opt_word = -opt_word;

                memset(&buffer[buffer_position + line_position], image_data[0], opt_word);

                image_data += sizeof(uint8_t);
            }
        }

        buffer_position += pitch;

        if ((line_count % 32) == 0) {
            process_bk();
        }
    }
}

int32_t WindowManager_LoadBigImage(ResourceID id, WindowInfo *window, int16_t pitch, bool palette_from_image,
                                   bool draw_to_screen, int32_t ulx, int32_t uly, bool center_align, bool rescale) {
    uint8_t *resource;
    ImageBigHeader *image;
    int32_t width = WindowManager_GetWidth(window);
    int32_t height = WindowManager_GetHeight(window);

    process_bk();

    resource = ResourceManager_ReadResource(id);
    image = reinterpret_cast<ImageBigHeader *>(resource);

    if (!image) {
        return 0;
    }

    if (palette_from_image) {
        WindowManager_SwapSystemPalette(image);
    }

    if (ulx == -1) {
        ulx = image->ulx;
    }

    if (uly == -1) {
        uly = image->uly;
    }

    if (rescale && (width != image->width || height != image->height)) {
        if (!WindowManager_CustomSpriteScaler(id, image, window, pitch, ulx, uly)) {
            WindowInfo w = *window;
            double scale;

            w.buffer = new (std::nothrow) uint8_t[image->width * image->height];
            w.width = image->width;

            WindowManager_DecodeBigImage(image, w.buffer, 0, 0, w.width);

            if (width >= height) {
                if (image->width + ulx >= image->height + uly) {
                    scale = static_cast<double>(height) / (image->height + uly);

                } else {
                    scale = static_cast<double>(height) / (image->width + ulx);
                }

            } else {
                if (image->width + ulx >= image->height + uly) {
                    scale = static_cast<double>(width) / (image->height + uly);

                } else {
                    scale = static_cast<double>(width) / (image->width + ulx);
                }
            }

            width = image->width * scale;
            height = image->height * scale;

            if (center_align) {
                ulx = (WindowManager_GetWidth(window) - width) / 2;
                uly = (WindowManager_GetHeight(window) - height) / 2;
            }

            cscale(w.buffer, image->width, image->height, w.width, &window->buffer[pitch * uly + ulx], width, height,
                   pitch);

            delete[] w.buffer;
        }

    } else {
        if (center_align) {
            ulx = (window->window.lrx - window->window.ulx - (image->ulx + image->width)) / 2;

            if (ulx < 0) {
                ulx = 0;
            }

            uly = (window->window.lry - window->window.uly - (image->uly + image->height)) / 2;

            if (uly < 0) {
                uly = 0;
            }
        }

        WindowManager_DecodeBigImage(image, window->buffer, ulx, uly, pitch);
    }

    if (draw_to_screen) {
        win_draw(window->id);
    }

    delete[] resource;

    return 1;
}

void WindowManager_DecodeSimpleImage(struct ImageSimpleHeader *image, int32_t ulx, int32_t uly, bool has_transparency,
                                     WindowInfo *w) {
    int32_t height;
    int32_t width;
    uint8_t *buffer;
    uint8_t *image_data;
    int32_t length;

    if (image) {
        const int32_t screen_width = WindowManager_GetWidth(&windows[WINDOW_MAIN_WINDOW]);
        const int32_t screen_height = WindowManager_GetHeight(&windows[WINDOW_MAIN_WINDOW]);

        if (w == nullptr) {
            w = WindowManager_GetWindow(WINDOW_MAIN_WINDOW);
        }

        width = w->window.lrx - w->window.ulx + 1;
        height = w->window.lry - w->window.uly + 1;

        if (width > screen_width) {
            width = screen_width;
        }

        if (height > screen_height) {
            height = screen_height;
        }

        length = image->width;

        ulx -= image->ulx;
        uly -= image->uly;

        buffer = &w->buffer[ulx + uly * w->width];
        image_data = &image->transparent_color;

        if (ulx < 0) {
            image_data -= ulx;
            buffer -= ulx;
            length += ulx;
        }

        if (image->width + ulx >= width) {
            length -= image->width + ulx - width;
        }

        for (int32_t i = 0; (i < image->height) && (uly < height); i++) {
            if (uly >= 0) {
                if (has_transparency) {
                    for (int32_t j = 0; j < length; j++) {
                        if (image_data[j] != image->transparent_color) {
                            buffer[j] = image_data[j];
                        }
                    }
                } else {
                    SDL_memcpy(buffer, image_data, length);
                }
            }

            image_data += image->width;
            buffer += w->width;
            uly++;
        }
    }
}

void WindowManager_LoadSimpleImage(ResourceID id, int32_t ulx, int32_t uly, bool has_transparency, WindowInfo *w) {
    WindowManager_DecodeSimpleImage(reinterpret_cast<struct ImageSimpleHeader *>(ResourceManager_LoadResource(id)), ulx,
                                    uly, has_transparency, w);
}

struct ImageSimpleHeader *WindowManager_RescaleSimpleImage(struct ImageSimpleHeader *image, int32_t scaling_factor) {
    int32_t width;
    int32_t height;
    int32_t scaled_width;
    int32_t scaled_height;
    int32_t scaling_factor_width;
    int32_t scaling_factor_height;
    struct ImageSimpleHeader *scaled_image;
    uint8_t *buffer;
    uint8_t *image_data;
    uint8_t *scaled_image_data;

    width = image->width;
    height = image->height;

    scaled_width = (width * GFX_SCALE_DENOMINATOR) / scaling_factor;
    scaled_height = (height * GFX_SCALE_DENOMINATOR) / scaling_factor;

    scaled_image = reinterpret_cast<struct ImageSimpleHeader *>(
        new (std::nothrow) uint8_t[scaled_width * scaled_height + sizeof(image->width) * 4]);

    scaled_image->width = scaled_width;
    scaled_image->height = scaled_height;
    scaled_image->ulx = (image->ulx * GFX_SCALE_DENOMINATOR) / scaling_factor;
    scaled_image->uly = (image->uly * GFX_SCALE_DENOMINATOR) / scaling_factor;

    scaling_factor_width = ((width - 1) * GFX_SCALE_DENOMINATOR) / (scaled_width - 1) + 8;
    scaling_factor_height = ((height - 1) * GFX_SCALE_DENOMINATOR) / (scaled_height - 1) + 8;

    image_data = &image->transparent_color;
    scaled_image_data = &scaled_image->transparent_color;

    for (int32_t i = 0; i < scaled_height; ++i) {
        buffer = &image_data[((i * scaling_factor_height) / GFX_SCALE_DENOMINATOR) * width];

        for (int32_t j = 0; j < scaled_width; ++j) {
            scaled_image_data[j + i * scaled_width] = buffer[(j * scaling_factor_width) / GFX_SCALE_DENOMINATOR];
        }
    }

    return scaled_image;
}

int32_t WindowManager_GetWidth(WindowInfo *w) { return win_width(w->id); }

int32_t WindowManager_GetHeight(WindowInfo *w) { return win_height(w->id); }

double WindowManager_GetScale() {
    WindowInfo *const screen = &windows[WINDOW_MAIN_WINDOW];
    int32_t screen_width = WindowManager_GetWidth(screen);
    int32_t screen_height = WindowManager_GetHeight(screen);
    double scale;

    if (screen_width >= screen_height) {
        scale = static_cast<double>(screen_height) / WINDOW_HEIGHT;

    } else {
        scale = static_cast<double>(screen_width) / WINDOW_WIDTH;
    }

    return scale;
}

int32_t WindowManager_ScaleUlx(WindowInfo *w, int32_t ulx) {
    return (WindowManager_GetWidth(w) - WINDOW_WIDTH) / 2 + ulx;
}

int32_t WindowManager_ScaleUly(WindowInfo *w, int32_t uly) {
    return (WindowManager_GetHeight(w) - WINDOW_HEIGHT) / 2 + uly;
}

int32_t WindowManager_ScaleLrx(WindowInfo *w, int32_t ulx, int32_t lrx) {
    int32_t width = lrx - ulx;

    return WindowManager_ScaleUlx(w, ulx) + width;
}

int32_t WindowManager_ScaleLry(WindowInfo *w, int32_t uly, int32_t lry) {
    int32_t height = lry - uly;

    return WindowManager_ScaleUly(w, uly) + height;
}

int32_t WindowManager_ScaleOffset(WindowInfo *w, int32_t ulx, int32_t uly) {
    return w->width * WindowManager_ScaleUly(w, uly) + WindowManager_ScaleUlx(w, ulx);
}

void WindowManager_ScaleCursor(int32_t help_id, int32_t window_id, int32_t &cursor_x, int32_t &cursor_y) {
    if (window_id == WINDOW_MAIN_WINDOW || (window_id == WINDOW_MAIN_MAP && (help_id == HELPMENU_REPORTS_SETUP))) {
        WindowInfo *const window = WindowManager_GetWindow(GameManager_GetDialogWindowCenterMode());

        cursor_x -= ((window->window.lrx + window->window.ulx + 1) - WINDOW_WIDTH) / 2;
        cursor_y -= ((window->window.lry + window->window.uly + 1) - WINDOW_HEIGHT) / 2;

    } else if (window_id == WINDOW_MAIN_MAP) {
        const double scale = WindowManager_GetScale();

        cursor_x /= scale;
        cursor_y /= scale;

    } else {
        SDL_assert(0);
    }
}
