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

#ifndef VERSION_HPP
#define VERSION_HPP

#if !defined(GAME_VERSION_STRING)
#define GAME_VERSION_STRING ""
#warning "Game version string is not set by CMake."
#endif

#if !defined(GAME_VERSION_MAJOR)
#define GAME_VERSION_MAJOR 0
#warning "Game major version number is not set by CMake."
#endif

#if !defined(GAME_VERSION_MINOR)
#define GAME_VERSION_MINOR 0
#warning "Game minor version number is not set by CMake."
#endif

#if !defined(GAME_VERSION_PATCH)
#define GAME_VERSION_PATCH 0
#warning "Game patch version number is not set by CMake."
#endif

#define GAME_VERSION_CREATE(major, minor, patch) (((major) << 16) | ((minor) << 8) | ((patch) << 0))

#define GAME_VERSION_GET_MAJOR(version) (((version) >> 16) & 0xFF)
#define GAME_VERSION_GET_MINOR(version) (((version) >> 8) & 0xFF)
#define GAME_VERSION_GET_PATCH(version) (((version) >> 0) & 0xFF)

#define GAME_VERSION GAME_VERSION_CREATE(GAME_VERSION_MAJOR, GAME_VERSION_MINOR, GAME_VERSION_PATCH)

#endif /* VERSION_HPP */
