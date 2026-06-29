#include "PaletteProvider.h"

namespace planetgen
{

const Palette& PaletteProvider::Resolve(const PaletteRegistry& registry,
                                        const std::string& paletteId) const
{
    return registry.Resolve(paletteId);
}

} // namespace planetgen
