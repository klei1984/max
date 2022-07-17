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

#include "paths_manager.hpp"

#include "resource_manager.hpp"
#include "searcher.hpp"

class PathsManager {
    static unsigned char **PathsManager_AccessMap;

    SmartPointer<PathRequest> request;
    SmartList<PathRequest> requests;
    unsigned int time_stamp;
    unsigned int elapsed_time;
    Searcher *forward_searcher;
    Searcher *backward_searcher;

public:
    PathsManager();
    ~PathsManager();

    void PushBack(PathRequest &object);
    void PushFront(PathRequest &object);
    void Clear();
    int GetRequestCount(unsigned short team) const;
    void RemoveRequest(PathRequest *path_request);
    void RemoveRequest(UnitInfo *unit);
    void EvaluateTiles();
    bool HasRequest(UnitInfo *unit) const;
};

unsigned char **PathsManager::PathsManager_AccessMap;

PathsManager PathsManager_Instance;

PathsManager::PathsManager() : forward_searcher(nullptr), backward_searcher(nullptr) {}

PathsManager::~PathsManager() {
    delete forward_searcher;
    delete backward_searcher;

    if (PathsManager_AccessMap) {
        for (int i = 0; i < ResourceManager_MapSize.x; ++i) {
            delete[] PathsManager_AccessMap[i];
        }

        delete[] PathsManager_AccessMap;
        PathsManager_AccessMap = nullptr;
    }
}

void PathsManager::PushBack(PathRequest &object) { requests.PushBack(object); }

void PathsManager::Clear() {
    delete forward_searcher;
    forward_searcher = nullptr;

    delete backward_searcher;
    backward_searcher = nullptr;

    request = nullptr;
    requests.Clear();
}

void PathsManager::PushFront(PathRequest &object) {
    if (request != nullptr) {
        requests.PushFront(*request);

        delete forward_searcher;
        forward_searcher = nullptr;

        delete backward_searcher;
        backward_searcher = nullptr;

        request = nullptr;
    }

    requests.PushFront(object);
}

int PathsManager::GetRequestCount(unsigned short team) const {
    int count;

    count = 0;

    for (SmartList<PathRequest>::Iterator it = requests.Begin(); it != requests.End(); ++it) {
        UnitInfo *unit = (*it).GetUnit1();

        if (unit && unit->team == team) {
            ++count;
        }
    }

    if (request != nullptr && request->GetUnit1() && request->GetUnit1()->team == team) {
        ++count;
    }

    return count;
}

void PathsManager::RemoveRequest(PathRequest *path_request) {
    if (request == path_request) {
        delete forward_searcher;
        forward_searcher = nullptr;

        delete backward_searcher;
        backward_searcher = nullptr;

        request = nullptr;
    }

    requests.Remove(*path_request);

    path_request->Cancel();
}

void PathsManager::RemoveRequest(UnitInfo *unit) {
    for (SmartList<PathRequest>::Iterator it = requests.Begin(); it != requests.End(); ++it) {
        if ((*it).GetUnit1() == unit) {
            (*it).Cancel();
            requests.Remove(it);
        }
    }

    if (request != nullptr && request->GetUnit1() == unit) {
        RemoveRequest(&*request);
    }
}

void PathsManager::EvaluateTiles() {
    /// \todo
}

bool PathsManager::HasRequest(UnitInfo *unit) const {
    for (SmartList<PathRequest>::Iterator it = requests.Begin(); it != requests.End(); ++it) {
        if ((*it).GetUnit1() == unit) {
            return true;
        }
    }

    if (request != nullptr && request->GetUnit1() == unit) {
        return true;
    }

    return false;
}

int PathsManager_GetRequestCount(unsigned short team) { return PathsManager_Instance.GetRequestCount(team); }

void PathsManager_RemoveRequest(PathRequest *request) { PathsManager_Instance.RemoveRequest(request); }

void PathsManager_RemoveRequest(UnitInfo *unit) { PathsManager_Instance.RemoveRequest(unit); }

void PathsManager_PushBack(PathRequest &object) { PathsManager_Instance.PushBack(object); }

void PathsManager_PushFront(PathRequest &object) { PathsManager_Instance.PushFront(object); }

void PathsManager_EvaluateTiles() { PathsManager_Instance.EvaluateTiles(); }

void PathsManager_Clear() { PathsManager_Instance.Clear(); }

bool PathsManager_HasRequest(UnitInfo *unit) { return PathsManager_Instance.HasRequest(unit); }
