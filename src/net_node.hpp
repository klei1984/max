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

#ifndef NET_NODE_HPP
#define NET_NODE_HPP

#include "net_address.hpp"
#include "smartobjectarray.hpp"

struct NetNode {
    uint16_t entity_id;
    NetAddress address;
    char name[30];
    bool is_host;
};

class NetNodeArray {
    SmartObjectArray<NetNode> nodes;

public:
    void Add(NetNode& node) {
        NetNode* node_address;

        node_address = Find(node.address);
        if (node_address) {
            *node_address = node;
        } else {
            node_address = Find(node.entity_id);
            if (node_address) {
                *node_address = node;
            } else {
                nodes.PushBack(&node);
            }
        }
    }

    NetNode* Find(uint16_t entity_id) const {
        for (uint32_t i = 0; i < nodes.GetCount(); ++i) {
            if (nodes[i]->entity_id == entity_id) {
                return nodes[i];
            }
        }

        return nullptr;
    }

    NetNode* Find(NetAddress& address) const {
        for (uint32_t i = 0; i < nodes.GetCount(); ++i) {
            if (nodes[i]->address == address) {
                return nodes[i];
            }
        }

        return nullptr;
    }

    NetNode* operator[](uint16_t position) const { return nodes.GetCount() > position ? nodes[position] : nullptr; }

    void Clear() { nodes.Clear(); }

    void Remove(uint16_t entity_id) {
        for (uint32_t i = 0; i < nodes.GetCount(); ++i) {
            if (nodes[i]->entity_id == entity_id) {
                nodes.Remove(i);
                break;
            }
        }
    }

    uint32_t GetCount() const { return nodes.GetCount(); }
};

#endif /* NET_NODE_HPP */
