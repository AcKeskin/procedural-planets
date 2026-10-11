#include "SystemTab.h"
#include "Widgets.h"
#include "../settings/TerrainSettings.h"
#include <imgui.h>

namespace planets::render
{

bool SystemTab::Draw(LodConfig& lod, const TerrainStats& stats, float frameMs)
{
    using namespace ui;
    bool regen = false;

    _frameHistory[_historyHead] = frameMs;
    _historyHead = (_historyHead + 1) % HistorySize;

    if (ImGui::CollapsingHeader("Frame", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("%.1f ms  %.0f fps", frameMs, frameMs > 0.0f ? 1000.0f / frameMs : 0.0f);
        ImGui::PlotLines("##frames",
                         _frameHistory,
                         HistorySize,
                         _historyHead,
                         "last 120 frames (ms)",
                         0.0f,
                         40.0f,
                         ImVec2(-1.0f, ImGui::GetFontSize() * 3.0f));
        ImGui::TextDisabled("GPU terrain %.2f / ocean %.2f / effects %.2f ms",
                            stats.gpuTerrainMs,
                            stats.gpuOceanMs,
                            stats.gpuEffectsMs);
        if (stats.patchCount > 0)
        {
            const float culled = 100.0f * (1.0f - static_cast<float>(stats.visiblePatchCount) / stats.patchCount);
            ImGui::TextDisabled(
                "Patches %d, visible %d, culled %.0f%%", stats.patchCount, stats.visiblePatchCount, culled);
        }
    }

    if (ImGui::CollapsingHeader("Mesh Detail (LOD)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        regen |= SliderRegen("Split Threshold",
                             lod.splitThreshold,
                             0.5f,
                             12.0f,
                             "%.1f",
                             "Higher = finer mesh near the camera, more GPU cost.");
        regen |= SliderRegen(
            "Patch Budget", lod.maxActivePatches, 50, 4000, "Most terrain patches drawn at once. Caps the LOD cost.");
        if (ImGui::TreeNode("Advanced##lod"))
        {
            const char* densities[] = {"20 patches", "80 patches", "320 patches", "1280 patches"};
            if (ImGui::Combo("Patch Density", &lod.patchSubdivisions, densities, 4))
                regen = true;
            ImGui::SetItemTooltip("Number of root patches covering the planet.");
            regen |= SliderRegen("Mesh Resolution",
                                 lod.meshResolution,
                                 8,
                                 64,
                                 "Vertices along each patch edge. Higher = smoother, slower.");
            regen |=
                SliderRegen("Max Depth", lod.maxDepth, 2, 12, "Deepest LOD level: finer mesh up close, more patches.");
            regen |= SliderRegen("Hysteresis",
                                 lod.hysteresis,
                                 1.0f,
                                 2.0f,
                                 "%.2f",
                                 "Gap between split and merge distance, so patches do not flicker.");
            regen |= SliderRegen("Skirt",
                                 lod.skirtFraction,
                                 0.0f,
                                 0.1f,
                                 "%.3f",
                                 "Depth of the edge skirts that hide cracks between LOD levels.");
            ImGui::TreePop();
        }
        if (ImGui::Button("Rebuild LOD"))
            regen = true;
        ImGui::SetItemTooltip("Rebuild every terrain patch with the current settings.");
    }

    return regen;
}

} // namespace planets::render
