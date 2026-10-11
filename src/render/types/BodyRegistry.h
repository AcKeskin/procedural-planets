#pragma once

#include <string>
#include <vector>

namespace planets::render::types
{

// One planet type found on disk: a body JSON in the bodies folder.
struct BodyEntry
{
    std::string id;   // file stem, e.g. "earth" (what --body and setBody take)
    std::string name; // metadata.name from the JSON, falls back to the id
    std::string path; // path of the JSON file
};

// Every *.json in `dir`, Earth first, the rest by id. Adding a planet type = adding a file.
std::vector<BodyEntry> ScanBodies(const std::string& dir);

// Index of the entry with this id, or -1.
int FindBody(const std::vector<BodyEntry>& bodies, const std::string& id);

} // namespace planets::render::types
