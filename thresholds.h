#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "CCharacter.hpp"
#include "server_id.hpp"

namespace thresholds {

struct ParseException : std::exception {
    ParseException(std::string what): message(std::move(what)) {
    }

    const char* what() const {
        return message.c_str();
    }

    std::string message;
};

// A node contains either a mapping to children nodes or a leaf value.
// The leaf values can be a number or a mob mapping.
struct Node {
    Node() : is_leaf(false), number(0) {
    }

    explicit Node(uint32_t n) : is_leaf(true), number(n) {
    }

    std::unordered_map<std::string, std::shared_ptr<Node>> children;
    bool is_leaf;
    uint32_t number;

    std::unordered_map<ServerIDType, std::shared_ptr<Node>> by_server;
    std::unordered_map<uint16_t, int> mobs;
};

class Thresholds {
public:
    void LoadFromContent(std::string content);
    void LoadFromFile(std::string file_name);
    void LoadFromStream(std::istream& stream);

    uint32_t Value(std::string key, const CCharacter& chr, ServerIDType server_id) const;
    const std::unordered_map<uint16_t, int>* Mobs(std::string key, const CCharacter& chr, ServerIDType server_id) const;

private:
    const Node* Path(std::string key) const;
    const Node* Descend(const Node* at, const CCharacter& chr, ServerIDType server_id) const;

private:
    std::shared_ptr<Node> root;
};

extern Thresholds thresholds;

} // namespace thresholds