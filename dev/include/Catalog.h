#pragma once

#include <string>
#include <vector>

// Shared, dependency-free helpers for the name-keyed catalogs. Both the enemy
// catalog and the content (item) catalog resolve entries by their `name` field
// and reject unnamed entries, so that logic lives here once.
namespace Catalog
{

    // Returns a pointer to the first entry whose `name` equals `name`, or nullptr.
    // The pointer remains valid as long as `entries` is alive (catalogs are
    // function-local statics with process lifetime).
    template <typename Entry>
    const Entry *FindByName(const std::vector<Entry> &entries, const std::string &name)
    {
        for (const auto &entry : entries)
            if (entry.name == name)
                return &entry;
        return nullptr;
    }

    // True when every entry carries a non-empty name.
    template <typename Entry>
    bool AllNamed(const std::vector<Entry> &entries)
    {
        for (const auto &entry : entries)
            if (entry.name.empty())
                return false;
        return true;
    }

} // namespace Catalog
