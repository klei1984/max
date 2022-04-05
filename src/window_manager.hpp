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

#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include "enums.hpp"
#include "gnw.h"

enum {
    WINDOW_MAIN_WINDOW,
    WINDOW_UNKNOWN_01,
    WINDOW_MESSAGE_BOX,
    WINDOW_ENDTURN_BUTTON,
    WINDOW_FILES_BUTTON,
    WINDOW_PREFS_BUTTON,
    WINDOW_CORNER_FLIC,
    WINDOW_PLAY_BUTTON,
    WINDOW_PAUSE_BUTTON,
    WINDOW_STAT_WINDOW,
    WINDOW_STAT_ROW_1,
    WINDOW_STAT_ROW_2,
    WINDOW_STAT_ROW_3,
    WINDOW_STAT_ROW_4,
    WINDOW_CENTER_BUTTON,
    WINDOW_LOCK_BUTTON,
    WINDOW_PRE_BUTTON,
    WINDOW_DONE_BUTTON,
    WINDOW_NXT_BUTTON,
    WINDOW_HELP_BUTTON,
    WINDOW_REPORTS_BUTTON,
    WINDOW_CHAT_BUTTON,
    WINDOW_ZOOM_PLUS_BUTTON,
    WINDOW_ZOOM_SLIDER_BUTTON,
    WINDOW_ZOOM_MINUS_BUTTON,
    WINDOW_ZOOM_SLIDER_WINDOW,
    WINDOW_SURVEY_BUTTON,
    WINDOW_STATUS_BUTTON,
    WINDOW_COLORS_BUTTON,
    WINDOW_HITS_BUTTON,
    WINDOW_AMMO_BUTTON,
    WINDOW_RANGE_BUTTON,
    WINDOW_SCAN_BUTTON,
    WINDOW_GRID_BUTTON,
    WINDOW_NAME_BUTTON,
    WINDOW_MINIMAP,
    WINDOW_2X_MINIMAP,
    WINDOW_TNT_MINIMAP,
    WINDOW_MAIN_MAP,
    WINDOW_SCROLL_UP_WINDOW,
    WINDOW_SCROLL_UP_RIGHT_WINDOW,
    WINDOW_SCROLL_RIGHT_UP_WINDOW,
    WINDOW_SCROLL_RIGHT_WINDOW,
    WINDOW_SCROLL_RIGHT_DOWN_WINDOW,
    WINDOW_SCROLL_DOWN_RIGHT_WINDOW,
    WINDOW_SCROLL_DOWN_WINDOW,
    WINDOW_SCROLL_DOWN_LEFT_WINDOW,
    WINDOW_SCROLL_LEFT_DOWN_WINDOW,
    WINDOW_SCROLL_LEFT_WINDOW,
    WINDOW_SCROLL_LEFT_UP_WINDOW,
    WINDOW_SCROLL_UP_LEFT_WINDOW,
    WINDOW_COORDINATES_DISPLAY,
    WINDOW_UNIT_DESCRIPTION_DISPLAY,
    WINDOW_TURN_COUNTER_DISPLAY,
    WINDOW_TURN_TIMER_DISPLAY,
    WINDOW_TOP_INSTRUMENTS_WINDOW,
    WINDOW_BOTTOM_INSTRUMENTS_WINDOW,
    WINDOW_INTERFACE_PANEL_TOP,
    WINDOW_INTERFACE_PANEL_BOTTOM,
    WINDOW_COUNT
};

extern Color *WindowManager_SystemPalette;
extern Color *WindowManager_ColorPalette;

int WindowManager_Init();
WindowInfo *WindowManager_GetWindow(unsigned char id);
void WindowManager_ClearWindow();
void WindowManager_FadeOut(int steps);
void WindowManager_FadeIn(int steps);
void WindowManager_LoadPalette(ResourceID id);
void WindowManager_DecodeImage(struct ImageBigHeader *image, unsigned char *buffer, int width, int height, int ulx);
int WindowManager_LoadImage(ResourceID id, WindowInfo *window, short offx, int palette_from_image, int draw_to_screen,
                            int width_from_image = -1, int height_from_image = -1);
void WindowManager_DecodeImage2(struct ImageSimpleHeader *image, int ulx, int uly, int has_transparency, WindowInfo *w);
void WindowManager_LoadImage2(ResourceID id, int ulx, int uly, int has_transparency, WindowInfo *w);

#endif /* WINDOW_MANAGER_HPP */