#include "BodyVariety.h"

#include <iostream>
#include <random>
#include <string>

namespace planetgen
{

namespace
{
using nlohmann::json;

// Walks "a.b.c" from the root, creating objects on the way; returns the slot to write.
json& Slot(json& root, const std::string& key)
{
    json* node = &root;
    size_t start = 0;
    while (true)
    {
        const size_t dot = key.find('.', start);
        const std::string part = key.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        if (!node->is_object())
            *node = json::object();
        node = &(*node)[part];
        if (dot == std::string::npos)
            return *node;
        start = dot + 1;
    }
}

bool Roll(const json& rule, std::mt19937& rng, json& out)
{
    if (!rule.is_object() || rule.size() != 1)
        return false;
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    if (rule.contains("range"))
    {
        const json& r = rule["range"];
        if (!r.is_array() || r.size() != 2 || !r[0].is_number() || !r[1].is_number())
            return false;
        if (r[0].is_number_integer() && r[1].is_number_integer())
            out = std::uniform_int_distribution<int>(r[0].get<int>(), r[1].get<int>())(rng);
        else
            out = r[0].get<float>() + (r[1].get<float>() - r[0].get<float>()) * unit(rng);
        return true;
    }
    if (rule.contains("pick"))
    {
        const json& p = rule["pick"];
        if (!p.is_array() || p.empty())
            return false;
        out = p[std::uniform_int_distribution<size_t>(0, p.size() - 1)(rng)];
        return true;
    }
    if (rule.contains("mix"))
    {
        const json& m = rule["mix"];
        if (!m.is_array() || m.size() != 2)
            return false;
        const float t = unit(rng);
        auto blend = [t](const json& a, const json& b)
        {
            return a.get<float>() + (b.get<float>() - a.get<float>()) * t;
        };
        if (m[0].is_number() && m[1].is_number())
        {
            out = blend(m[0], m[1]);
            return true;
        }
        if (m[0].is_array() && m[1].is_array() && m[0].size() == m[1].size())
        {
            out = json::array();
            for (size_t i = 0; i < m[0].size(); ++i)
            {
                if (!m[0][i].is_number() || !m[1][i].is_number())
                    return false;
                out.push_back(blend(m[0][i], m[1][i]));
            }
            return true;
        }
    }
    return false;
}
} // namespace

bool HasVariety(const json& body)
{
    return body.contains("variety") && body["variety"].is_object() && !body["variety"].empty();
}

json RollVariety(const json& body, uint32_t seed)
{
    json world = body;
    if (!HasVariety(body))
        return world;

    // json objects iterate in key order, so the draws happen in the same order every time.
    // seed_seq spreads the seed out: neighbouring seeds would otherwise start alike.
    std::seed_seq seq{seed, 0x9E3779B9u};
    std::mt19937 rng(seq);
    for (const auto& [key, rule] : body["variety"].items())
    {
        json value;
        if (Roll(rule, rng, value))
            Slot(world, key) = value;
        else
            std::cerr << "[planetgen] variety: Skipping '" << key << "': expected {range|pick|mix: [...]}" << std::endl;
    }
    return world;
}

} // namespace planetgen
