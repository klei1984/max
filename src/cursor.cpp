/* Copyright (c) 2021 M.A.X. Port Team
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

#include "cursor.hpp"

#include <SDL3/SDL.h>

extern "C" {
#include "gnw.h"
#include "timer.h"
}

#include "enums.hpp"
#include "resource_manager.hpp"
#include "settings.hpp"
#include "units_manager.hpp"

#define CURSOR_CURSOR_COUNT 30

struct Cursor_Descriptor {
    int16_t width;
    int16_t height;
    int16_t ulx;
    int16_t uly;
    uint16_t frame_count;
    uint8_t* data;
};

struct HardwareCursor {
    SDL_Cursor* cursor;
    SDL_Cursor** frames;
    int32_t frame_count;
    int32_t current_frame;
    uint32_t frame_duration;
    bool is_animated;
};

static ResourceID Cursor_ResourceLut[CURSOR_CURSOR_COUNT] = {
    HIDDNPTR, HANDPTR,  ARROW_N,  ARROW_NE, ARROW_E,  ARROW_SE, ARROW_S,  ARROW_SW, ARROW_W,  ARROW_NW,
    MAP_PTR,  MINI_PTR, FRND_FIX, FRND_XFR, FRND_FUE, PTR_RLD,  FRND_LOD, FRND_PTR, ENMY_PTR, PTR_FTRG,
    UNIT_GO,  UNIT_NGO, WAY_PTR,  GROUPPTR, ACTVTPTR, MAP_PTR,  STEALPTR, DISBLPTR, PTR_PATH, PTR_HELP};

static Cursor_Descriptor Cursor_CursorDescriptorLut[CURSOR_CURSOR_COUNT];
static HardwareCursor Cursor_HardwareCursors[CURSOR_CURSOR_COUNT];

static uint8_t Cursor_DefaultWindowCursorLut[] = {
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_MAP,      CURSOR_ARROW_N, CURSOR_ARROW_NE, CURSOR_ARROW_NE,
    CURSOR_ARROW_E, CURSOR_ARROW_SE, CURSOR_ARROW_SE, CURSOR_ARROW_S, CURSOR_ARROW_SW, CURSOR_ARROW_SW,
    CURSOR_ARROW_W, CURSOR_ARROW_NW, CURSOR_ARROW_NW, CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,
    CURSOR_HAND,    CURSOR_HAND,     CURSOR_HAND,     CURSOR_HAND,    CURSOR_HAND};

static uint8_t Cursor_ActiveCursorIndex = CURSOR_HIDDEN;
static float Cursor_CurrentScale = 1.0f;
static bool Cursor_IsVisible = false;
static bool Cursor_NeedsRegeneration = true;
static bool Cursor_CacheInitialized = false;

static void Cursor_AnimationTick(void);

/*
 * \brief Converts an sRGB color component to linear RGB color space.
 *
 * Applies the inverse sRGB gamma transfer function to convert from sRGB to linear RGB.
 *
 * \param srgb The sRGB color component value (0-255).
 * \return The linearized color component value (0-255).
 */
static uint8_t Cursor_ConvertSRGBToLinear(uint8_t srgb) {
    float normalized = srgb / 255.0f;
    float linear = (normalized <= 0.04045f) ? (normalized / 12.92f) : SDL_powf((normalized + 0.055f) / 1.055f, 2.4f);
    return static_cast<uint8_t>(linear * 255.0f + 0.5f);
}

static SDL_Surface* Cursor_CreateScaledSurface(uint8_t* indexed_data, int32_t width, int32_t height, int32_t full,
                                               uint8_t transparent_index, float scale) {
    int32_t scaled_w = static_cast<int32_t>(width * scale + 0.5f);
    int32_t scaled_h = static_cast<int32_t>(height * scale + 0.5f);

    if (scaled_w < 1) {
        scaled_w = 1;
    }

    if (scaled_h < 1) {
        scaled_h = 1;
    }

    SDL_Surface* surface = SDL_CreateSurface(scaled_w, scaled_h, SDL_PIXELFORMAT_ARGB8888);

    if (!surface) {
        SDL_Log("Cursor_CreateScaledSurface: SDL_CreateSurface failed: %s", SDL_GetError());

        return nullptr;
    }

    // Set colorspace to sRGB to prevent washed out appearance with modern GPU drivers
    SDL_SetSurfaceColorspace(surface, SDL_COLORSPACE_SRGB);

    uint32_t* pixels = static_cast<uint32_t*>(surface->pixels);
    SDL_Palette* palette = Svga_GetPalette();

    if (!palette || !palette->colors) {
        SDL_Log("Cursor_CreateScaledSurface: No palette available");
        SDL_DestroySurface(surface);

        return nullptr;
    }

    // Nearest-neighbor scaling with palette lookup
    for (int32_t dst_y = 0; dst_y < scaled_h; ++dst_y) {
        int32_t src_y = static_cast<int32_t>(dst_y / scale);

        if (src_y >= height) {
            src_y = height - 1;
        }

        for (int32_t dst_x = 0; dst_x < scaled_w; ++dst_x) {
            int32_t src_x = static_cast<int32_t>(dst_x / scale);

            if (src_x >= width) {
                src_x = width - 1;
            }

            uint8_t index = indexed_data[src_x + src_y * full];

            if (index == transparent_index) {
                pixels[dst_x + dst_y * (surface->pitch / sizeof(uint32_t))] = 0x00000000;  // Fully transparent

            } else {
                SDL_Color* c = &palette->colors[index];
                uint8_t r, g, b;

                // Apply color space conversion based on cursor_colorspace setting
                // 0 = sRGB
                // 1 = Linear RGB
                int32_t cursor_colorspace = ResourceManager_GetSettings()->GetNumericValue("cursor_colorspace");

                if (cursor_colorspace == 1) {
                    r = Cursor_ConvertSRGBToLinear(c->r);
                    g = Cursor_ConvertSRGBToLinear(c->g);
                    b = Cursor_ConvertSRGBToLinear(c->b);

                } else {
                    r = c->r;
                    g = c->g;
                    b = c->b;
                }

                pixels[dst_x + dst_y * (surface->pitch / sizeof(uint32_t))] = (0xFFu << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }

    return surface;
}

static SDL_Cursor* Cursor_CreateHardwareCursor(Cursor_Descriptor* desc, int32_t frame_index, float scale) {
    if (!desc || !desc->data) {
        return nullptr;
    }

    uint8_t* frame_data = desc->data;

    if (frame_index > 0 && desc->frame_count > 1) {
        frame_data = &desc->data[desc->width * desc->height * frame_index];
    }

    uint8_t transparent_index = desc->data[0];

    SDL_Surface* surface =
        Cursor_CreateScaledSurface(frame_data, desc->width, desc->height, desc->width, transparent_index, scale);

    if (!surface) {
        return nullptr;
    }

    int32_t hot_x = static_cast<int32_t>(desc->ulx * scale + 0.5f);
    int32_t hot_y = static_cast<int32_t>(desc->uly * scale + 0.5f);

    SDL_Cursor* cursor = SDL_CreateColorCursor(surface, hot_x, hot_y);

    SDL_DestroySurface(surface);

    if (!cursor) {
        SDL_Log("Cursor_CreateHardwareCursor: SDL_CreateColorCursor failed: %s", SDL_GetError());
    }

    return cursor;
}

static void Cursor_DestroyHardwareCursor(HardwareCursor* hw) {
    if (hw->frames) {
        for (int32_t f = 0; f < hw->frame_count; ++f) {
            if (hw->frames[f]) {
                SDL_DestroyCursor(hw->frames[f]);
            }
        }

        SDL_free(hw->frames);

        hw->frames = nullptr;

    } else if (hw->cursor) {
        SDL_DestroyCursor(hw->cursor);
    }

    hw->cursor = nullptr;
    hw->frame_count = 0;
    hw->current_frame = 0;
    hw->is_animated = false;
}

static void Cursor_DrawAttackPowerCursorHelper(int32_t target_current_hits, int32_t attacker_damage,
                                               int32_t target_base_hits, uint8_t cursor_index) {
    Cursor_Descriptor* cursor = &Cursor_CursorDescriptorLut[cursor_index];
    int32_t lrx = cursor->ulx - 18;
    int32_t lry = cursor->uly + 15;
    uint8_t* data = &cursor->data[lrx + cursor->width * lry];

    if (target_base_hits) {
        if (attacker_damage > target_current_hits) {
            attacker_damage = target_current_hits;
        }

        target_current_hits -= attacker_damage;
        attacker_damage = (attacker_damage + target_current_hits) * 35 / target_base_hits;

        target_current_hits = target_current_hits * 35 / target_base_hits;
        attacker_damage -= target_current_hits;

        draw_box(data, cursor->width, 0, 0, 36, 4, COLOR_BLACK);

        if (target_current_hits) {
            buf_fill(&data[cursor->width + 1], target_current_hits, 3, cursor->width, 2);
        }

        if (attacker_damage) {
            buf_fill(&data[cursor->width + 1 + target_current_hits], attacker_damage, 3, cursor->width, 1);
        }

        if ((attacker_damage + target_current_hits) < 35) {
            buf_fill(&data[cursor->width + 1 + target_current_hits + attacker_damage],
                     35 - target_current_hits - attacker_damage, 3, cursor->width, cursor->data[0]);
        }
    } else {
        buf_fill(data, 37, 5, cursor->width, cursor->data[0]);
    }

    // Mark cursor for regeneration since indexed data was modified
    Cursor_NeedsRegeneration = true;
}

void Cursor_Init() {
    // Load cursor resource data into descriptors (no hardware cursor creation yet)
    for (int32_t cursor_index = 0; cursor_index < CURSOR_CURSOR_COUNT; ++cursor_index) {
        struct ImageSimpleHeader* const sprite =
            reinterpret_cast<struct ImageSimpleHeader*>(ResourceManager_LoadResource(Cursor_ResourceLut[cursor_index]));

        if (sprite) {
            Cursor_Descriptor* cursor = &Cursor_CursorDescriptorLut[cursor_index];

            cursor->data = &sprite->transparent_color;
            cursor->width = sprite->width;
            cursor->height = sprite->height;
            cursor->ulx = sprite->ulx;
            cursor->uly = sprite->uly;
            cursor->frame_count = 1;

            if (cursor->width < cursor->height) {
                cursor->frame_count = cursor->height / cursor->width;
                cursor->height = cursor->height / cursor->frame_count;
            }
        }

        // Initialize hardware cursor state (but don't create SDL cursors yet)
        Cursor_HardwareCursors[cursor_index].cursor = nullptr;
        Cursor_HardwareCursors[cursor_index].frames = nullptr;
        Cursor_HardwareCursors[cursor_index].frame_count = 0;
        Cursor_HardwareCursors[cursor_index].current_frame = 0;
        Cursor_HardwareCursors[cursor_index].frame_duration = 200;  // Default 200ms
        Cursor_HardwareCursors[cursor_index].is_animated = false;
    }

    Cursor_ActiveCursorIndex = CURSOR_CURSOR_COUNT;
    Cursor_CurrentScale = 0.0f;  // Force regeneration on first use
    Cursor_IsVisible = false;
    Cursor_NeedsRegeneration = true;  // Defer hardware cursor creation until palette is ready
    Cursor_CacheInitialized = false;

    // Add animation tick to background processes
    add_bk_process(Cursor_AnimationTick);
}

void Cursor_Deinit() {
    remove_bk_process(Cursor_AnimationTick);

    for (int32_t i = 0; i < CURSOR_CURSOR_COUNT; ++i) {
        Cursor_DestroyHardwareCursor(&Cursor_HardwareCursors[i]);
    }

    Cursor_ActiveCursorIndex = CURSOR_CURSOR_COUNT;
    Cursor_CurrentScale = 0.0f;
    Cursor_IsVisible = false;
    Cursor_CacheInitialized = false;
}

void Cursor_RegenerateCache(float new_scale) {
    if (new_scale <= 0.0f) {
        new_scale = 1.0f;
    }

    // Hide cursor during regeneration to avoid visual artifacts
    bool was_visible = Cursor_IsVisible;

    if (was_visible) {
        SDL_HideCursor();
    }

    // Destroy existing hardware cursors
    for (int32_t i = 0; i < CURSOR_CURSOR_COUNT; ++i) {
        Cursor_DestroyHardwareCursor(&Cursor_HardwareCursors[i]);
    }

    // Create new hardware cursors at the new scale
    for (int32_t i = 0; i < CURSOR_CURSOR_COUNT; ++i) {
        Cursor_Descriptor* desc = &Cursor_CursorDescriptorLut[i];
        HardwareCursor* hw = &Cursor_HardwareCursors[i];

        if (!desc->data) {
            continue;
        }

        if (desc->frame_count <= 1) {
            // Static cursor
            hw->cursor = Cursor_CreateHardwareCursor(desc, 0, new_scale);
            hw->frame_count = 1;
            hw->is_animated = false;

        } else {
            // Animated cursor - create array of frame cursors
            hw->frames = static_cast<SDL_Cursor**>(SDL_calloc(desc->frame_count, sizeof(SDL_Cursor*)));
            if (hw->frames) {
                for (int32_t f = 0; f < desc->frame_count; ++f) {
                    hw->frames[f] = Cursor_CreateHardwareCursor(desc, f, new_scale);
                }

                hw->cursor = hw->frames[0];
                hw->frame_count = desc->frame_count;
                hw->current_frame = 0;
                hw->is_animated = true;
            }
        }
    }

    Cursor_CurrentScale = new_scale;
    Cursor_NeedsRegeneration = false;
    Cursor_CacheInitialized = true;

    // Restore cursor visibility and apply the new cursor
    if (was_visible && Cursor_ActiveCursorIndex < CURSOR_CURSOR_COUNT) {
        HardwareCursor* hw = &Cursor_HardwareCursors[Cursor_ActiveCursorIndex];

        if (hw->cursor) {
            SDL_SetCursor(hw->cursor);
        }

        SDL_ShowCursor();
    }
}

float Cursor_ComputeScale() {
    int32_t physical_w, physical_h;

    SDL_Window* window = Svga_GetWindow();
    if (!window) {
        return 1.0f;
    }

    if (!SDL_GetWindowSizeInPixels(window, &physical_w, &physical_h)) {
        return 1.0f;
    }

    float logical_w = static_cast<float>(Svga_GetScreenWidth());
    float logical_h = static_cast<float>(Svga_GetScreenHeight());

    if (logical_w <= 0.0f || logical_h <= 0.0f) {
        return 1.0f;
    }

    float scale_x = static_cast<float>(physical_w) / logical_w;
    float scale_y = static_cast<float>(physical_h) / logical_h;

    // Use the smaller scale (letterbox scaling)
    return (scale_x < scale_y) ? scale_x : scale_y;
}

void Cursor_UpdateScale() {
    float new_scale = Cursor_ComputeScale();

    // Regenerate if: scale changed significantly, regeneration is needed, or cache was never initialized
    if (!Cursor_CacheInitialized || Cursor_NeedsRegeneration || SDL_fabsf(new_scale - Cursor_CurrentScale) > 0.01f) {
        Cursor_RegenerateCache(new_scale);
    }
}

uint8_t Cursor_GetCursor() { return Cursor_ActiveCursorIndex; }

uint8_t Cursor_GetDefaultWindowCursor(uint8_t window_index) { return Cursor_DefaultWindowCursorLut[window_index]; }

void Cursor_SetCursor(uint8_t cursor_index) {
    if (cursor_index >= CURSOR_CURSOR_COUNT) {
        return;
    }

    // Ensure hardware cursor cache is initialized before use
    if (!Cursor_CacheInitialized || Cursor_NeedsRegeneration) {
        Cursor_UpdateScale();
    }

    if (cursor_index != Cursor_ActiveCursorIndex) {
        HardwareCursor* hw = &Cursor_HardwareCursors[cursor_index];

        if (hw->cursor) {
            // Reset animation to first frame
            if (hw->is_animated && hw->frames) {
                hw->current_frame = 0;
                hw->cursor = hw->frames[0];
            }

            if (Cursor_IsVisible) {
                SDL_SetCursor(hw->cursor);
            }
        }

        Cursor_ActiveCursorIndex = cursor_index;
    }
}

extern "C" void Cursor_Show() {
    if (!Cursor_IsVisible) {
        // Regenerate cursors if needed (e.g., after palette change or scale change)
        if (Cursor_NeedsRegeneration) {
            Cursor_UpdateScale();
        }

        Cursor_IsVisible = true;

        if (Cursor_ActiveCursorIndex < CURSOR_CURSOR_COUNT) {
            HardwareCursor* hw = &Cursor_HardwareCursors[Cursor_ActiveCursorIndex];

            if (hw->cursor) {
                SDL_SetCursor(hw->cursor);
                SDL_ShowCursor();
            }

        } else {
            ;  // No valid cursor selected, don't show anything
        }
    }
}

extern "C" void Cursor_Hide() {
    if (Cursor_IsVisible) {
        Cursor_IsVisible = false;

        SDL_HideCursor();
    }
}

bool Cursor_IsHidden() { return !Cursor_IsVisible; }

void Cursor_MarkDirty() { Cursor_NeedsRegeneration = true; }

static void Cursor_AnimationTick() {
    static uint64_t ticker = 0;

    if (!Cursor_IsVisible || Cursor_ActiveCursorIndex >= CURSOR_CURSOR_COUNT) {
        return;
    }

    // Regenerate cursors if palette changed while cursor was visible
    if (Cursor_NeedsRegeneration) {
        Cursor_UpdateScale();
    }

    HardwareCursor* hw = &Cursor_HardwareCursors[Cursor_ActiveCursorIndex];

    if (!hw->is_animated || !hw->frames || hw->frame_count <= 1) {
        return;
    }

    if (timer_elapsed_time(ticker) >= hw->frame_duration) {
        ticker = timer_get();

        hw->current_frame = (hw->current_frame + 1) % hw->frame_count;
        hw->cursor = hw->frames[hw->current_frame];

        SDL_SetCursor(hw->cursor);
    }
}

static void Cursor_RegenerateHardwareCursor(uint8_t cursor_index) {
    if (cursor_index >= CURSOR_CURSOR_COUNT || !Cursor_CacheInitialized) {
        return;
    }

    Cursor_Descriptor* desc = &Cursor_CursorDescriptorLut[cursor_index];
    HardwareCursor* hw = &Cursor_HardwareCursors[cursor_index];

    if (!desc->data) {
        return;
    }

    SDL_Cursor* new_cursor = Cursor_CreateHardwareCursor(desc, 0, Cursor_CurrentScale);

    if (new_cursor) {
        if (cursor_index == Cursor_ActiveCursorIndex && Cursor_IsVisible) {
            SDL_SetCursor(new_cursor);
        }

        Cursor_DestroyHardwareCursor(hw);

        hw->cursor = new_cursor;
        hw->frame_count = 1;
        hw->is_animated = false;
    }

    Cursor_NeedsRegeneration = false;
}

void Cursor_DrawAttackPowerCursor(UnitInfo* selected_unit, UnitInfo* target_unit, uint8_t cursor_index) {
    if (target_unit) {
        Cursor_DrawAttackPowerCursorHelper(target_unit->hits,
                                           UnitsManager_GetAttackDamage(selected_unit, target_unit, 0),
                                           target_unit->GetBaseValues()->GetAttribute(ATTRIB_HITS), cursor_index);

    } else {
        Cursor_DrawAttackPowerCursorHelper(0, 0, 0, cursor_index);
    }

    Cursor_RegenerateHardwareCursor(cursor_index);
}

void Cursor_DrawStealthActionChanceCursor(int32_t experience_level, uint8_t cursor_index) {
    Cursor_Descriptor* cursor = &Cursor_CursorDescriptorLut[cursor_index];
    int32_t lrx = cursor->ulx - 18;
    int32_t lry = cursor->uly + 15;
    uint8_t* data = &cursor->data[lrx + cursor->width * lry];
    int32_t chance = experience_level * 35 / 100;
    int32_t reminder = 35 - experience_level * 35 / 100;

    draw_box(data, cursor->width, 0, 0, 36, 4, COLOR_BLACK);

    if (chance) {
        buf_fill(&data[cursor->width + 1], chance, 3, cursor->width, 1);
    }

    if (reminder) {
        buf_fill(&data[cursor->width + 1 + chance], reminder, 3, cursor->width, data[0]);
    }

    Cursor_RegenerateHardwareCursor(cursor_index);
}

void Cursor_RenderToBuffer(uint8_t* buffer, int32_t buffer_width, int32_t buffer_height) {
    if (!Cursor_IsVisible || Cursor_ActiveCursorIndex >= CURSOR_CURSOR_COUNT) {
        return;
    }

    Cursor_Descriptor* desc = &Cursor_CursorDescriptorLut[Cursor_ActiveCursorIndex];

    if (!desc->data) {
        return;
    }

    int32_t mouse_x, mouse_y;

    mouse_get_position(&mouse_x, &mouse_y);

    // Calculate cursor top-left position (account for hotspot)
    int32_t cursor_x = mouse_x - desc->ulx;
    int32_t cursor_y = mouse_y - desc->uly;

    // Get frame data for animated cursors
    uint8_t* frame_data = desc->data;
    HardwareCursor* hw = &Cursor_HardwareCursors[Cursor_ActiveCursorIndex];

    if (hw->is_animated && hw->current_frame > 0 && hw->current_frame < desc->frame_count) {
        frame_data = &desc->data[desc->width * desc->height * hw->current_frame];
    }

    uint8_t transparent_index = desc->data[0];

    for (int32_t y = 0; y < desc->height; ++y) {
        int32_t dst_y = cursor_y + y;

        if (dst_y < 0 || dst_y >= buffer_height) {
            continue;
        }

        for (int32_t x = 0; x < desc->width; ++x) {
            int32_t dst_x = cursor_x + x;

            if (dst_x < 0 || dst_x >= buffer_width) {
                continue;
            }

            uint8_t pixel = frame_data[x + y * desc->width];

            if (pixel != transparent_index) {
                buffer[dst_x + dst_y * buffer_width] = pixel;
            }
        }
    }
}
