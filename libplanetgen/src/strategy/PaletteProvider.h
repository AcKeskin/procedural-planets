#pragma once

#include "IGenerationStrategy.h"

namespace planetgen
{

// Built-in palette provider: a pure lookup against the PaletteRegistry by id.
// No GPU work — isolates palette resolution from the compute stages.
class PaletteProvider final : public IPaletteProvider
{
public:
    const Palette& Resolve(const PaletteRegistry& registry,
                           const std::string& paletteId) const override;
};

} // namespace planetgen
