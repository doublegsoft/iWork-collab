#pragma once

#include <string>
#include <optional>
#include <vector>

namespace proto {

// Simple role enum for chat-like messages.
enum class Role {
    System,
    User,
    Assistant
};

struct Message {
    std::string prompt;
    std::string instruction;
    Role role = Role::User;
};

// Encodes a Message as a compact JSON string.
std::string encode_json(const Message& msg);

// Decodes a JSON string into Message. Returns nullopt if invalid.
std::optional<Message> decode_json(const std::string& json);

// Helpers for role conversion.
std::string role_to_string(Role role);
std::optional<Role> role_from_string(const std::string& s);

} // namespace proto
