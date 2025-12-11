/* Copyright (c) 2025 M.A.X. Port Team
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

#ifndef WORLD_HPP
#define WORLD_HPP

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "color.h"
#include "gnw.h"
#include "point.hpp"
#include "resourcetable.hpp"
#include "timer.h"

class DrawLoadBar;

/**
 * \brief Defines a single color animation cycle with palette index range, rotation direction, and timing parameters.
 *
 * Used by World class to manage animated palette color animations like water caustics. Each cycle rotates RGB triplets
 * within a specified palette index range at a configured frame rate.
 */
struct ColorCycleData {
    uint16_t start_index;      ///< First palette index in the animation cycle range (inclusive).
    uint16_t end_index;        ///< Last palette index in the animation cycle range (inclusive).
    uint8_t rotate_direction;  ///< 0 = rotate left (forward), 1 = rotate right (backward) through palette indices.
    uint64_t time_limit;       ///< Milliseconds between animation frames (calculated from FPS via TIMER_FPS_TO_MS).
    uint64_t time_stamp;       ///< Last update timestamp in milliseconds from timer_get(), used for throttling.
};

/**
 * \class World
 * \brief Encapsulates WRL (World) file parsing, tile management, palette handling, and color animation cycling for game
 * maps.
 *
 * The World class provides a two-tier loading system: lightweight preview mode for map selection menus, and full
 * loading mode for in-game rendering. Supports both enhanced graphics (64x64 tiles) and low-resolution mode (32x32
 * downsampled tiles).
 */
class World {
public:
    /**
     * \brief Constructs a World object in preview mode, loading only minimap, palette, and metadata without tile data
     * buffers.
     *
     * This lightweight constructor is designed for map selection menus where multiple World instances exist
     * simultaneously.
     *
     * \param resource_id The ResourceID enum value (e.g., SNOW_1, DESERT_3) identifying which WRL file to load from
     * game data directory.
     */
    explicit World(ResourceID resource_id);

    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    /**
     * \brief Loads full map data including all tile pixel buffers, surface map, and cargo map, displaying progress via
     * load bar widget.
     *
     * Performs heavyweight loading of tile buffers (2-5 MB depending on tile count and enhanced graphics mode), access
     * map, and cargo map. Integrates with DrawLoadBar to display progress at key milestones: 0% (start), 20% (tile IDs
     * loaded), 70% (tiles loaded), 73% (palette applied), 76% (surface map built), 85% (brightness tables generated),
     * 100% (complete). Can only be called once per instance.
     *
     * \param load_bar Pointer to DrawLoadBar instance for displaying loading progress UI. Must not be null. Progress
     * values range 0-100.
     * \return True if loading succeeded and map is fully operational, false if file I/O error occurred or insufficient
     * memory available.
     */
    [[nodiscard]] bool LoadFullMap(DrawLoadBar* load_bar);

    /**
     * \brief Releases all tile pixel buffers, surface map, and cargo map to free memory while retaining preview data
     * for future reloads.
     *
     * Reduces memory footprint from ~2-5 MB back to ~50 KB by deleting tile buffers. Preview data (minimap, palette,
     * dimensions) remains loaded. Calling LoadFullMap() after UnloadFullMap() will re-parse the WRL file from disk.
     * Safe to call multiple times or when not fully loaded (no-op).
     */
    void UnloadFullMap();

    /**
     * \brief Checks whether full map data (tiles, surface map, cargo map) is currently loaded in memory and ready for
     * rendering access.
     *
     * \return True if LoadFullMap() completed successfully and UnloadFullMap() has not been called, false if only
     * preview data is loaded.
     */
    [[nodiscard]] bool IsFullyLoaded() const;

    /**
     * \brief Updates palette color animation cycles based on elapsed time, rotating RGB triplets for water caustics and
     * other effects.
     *
     * Manages 13 active color cycles (water shimmer, shore effects, lava glow, etc.) running at 2-10 FPS. Checks
     * timestamps and rotates palette indices (ranges 9-127) when time_limit thresholds are exceeded. Must be called
     * every game tick (typically 24 FPS) to maintain smooth animations. Only effective when IsFullyLoaded() returns
     * true.
     *
     * \param current_time The current game time in milliseconds from timer_get(). Used to calculate time deltas for
     * FPS-based animations.
     */
    void UpdateColorAnimations(uint64_t current_time);

    /**
     * \brief Retrieves tileset-specific RGB intensity weights and factor for red-tinted message box backgrounds.
     *
     * \param r_level Output parameter for red intensity weight (0-63).
     * \param g_level Output parameter for green intensity weight (0-63).
     * \param b_level Output parameter for blue intensity weight (0-63).
     * \param factor Output parameter for tint intensity multiplier (0-63).
     */
    void GetRedTintParameters(uint8_t& r_level, uint8_t& g_level, uint8_t& b_level, uint8_t& factor) const;

    /**
     * \brief Retrieves tileset-specific RGB intensity weights for neutral-tinted message box backgrounds.
     *
     * \param r_level Output parameter for red intensity weight (0-63).
     * \param g_level Output parameter for green intensity weight (0-63).
     * \param b_level Output parameter for blue intensity weight (0-63).
     */
    void GetWorldTintParameters(uint8_t& r_level, uint8_t& g_level, uint8_t& b_level) const;

    /**
     * \brief Retrieves the surface type value at the specified grid coordinates from the active world's surface map.
     *
     * \param grid_x The X coordinate (column index) in the map grid.
     * \param grid_y The Y coordinate (row index) in the map grid.
     * \return The surface type value at the specified position (SURFACE_TYPE_LAND, SURFACE_TYPE_WATER, etc.), or
     * SURFACE_TYPE_NONE if invalid position.
     */
    [[nodiscard]] uint8_t GetSurfaceType(int32_t grid_x, int32_t grid_y) const;

    [[nodiscard]] const Point& GetMapSize() const { return m_map_size; }
    [[nodiscard]] const uint16_t* GetTileIds() const { return m_tile_ids.get(); }
    [[nodiscard]] const uint8_t* GetTileBuffer() const { return m_tile_buffer.get(); }
    [[nodiscard]] const uint8_t* GetSurfaceMap() const { return m_surface_map.get(); }
    [[nodiscard]] uint16_t* GetCargoMap() const { return m_cargo_map.get(); }
    [[nodiscard]] const uint8_t* GetMinimap() const { return m_minimap.get(); }
    [[nodiscard]] const uint8_t* GetMinimapFov() const { return m_minimap_fov.get(); }
    [[nodiscard]] const Point& GetMinimapWindowSize() const { return m_minimap_window_size; }
    [[nodiscard]] const Point& GetMinimapWindowOffset() const { return m_minimap_window_offset; }
    [[nodiscard]] double GetMinimapWindowScale() const { return m_minimap_window_scale; }

private:
    [[nodiscard]] bool ParseWrlFilePreview(FILE* fp);
    [[nodiscard]] bool LoadTilesAndMetadata(FILE* fp, DrawLoadBar* load_bar);
    [[nodiscard]] bool LoadMapTiles(FILE* fp, DrawLoadBar* load_bar);
    void ApplyPaletteAndTints(DrawLoadBar* load_bar);
    void InitColorCycles();
    void ApplyBugFixes();
    void CalculateMinimapScaling();
    [[nodiscard]] std::string ComputeHash() const;

    ResourceID m_resource_id;
    std::filesystem::path m_file_path;
    bool m_is_fully_loaded;

    Point m_map_size;
    uint16_t m_tile_count;
    uint16_t m_layer_count;

    std::unique_ptr<uint16_t[]> m_tile_ids;
    std::unique_ptr<uint8_t[]> m_tile_buffer;
    std::unique_ptr<uint8_t[]> m_surface_map;
    std::unique_ptr<uint16_t[]> m_cargo_map;

    std::unique_ptr<uint8_t[]> m_minimap;
    std::unique_ptr<uint8_t[]> m_minimap_fov;

    Point m_minimap_window_size;
    Point m_minimap_window_offset;
    double m_minimap_window_scale;

    std::unique_ptr<uint8_t[]> m_palette;
    std::vector<ColorCycleData> m_color_cycles;
    uint8_t m_color_cycle_step;
};

#endif  // WORLD_HPP
