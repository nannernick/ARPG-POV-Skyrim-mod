#pragma once

// The outline renderer (src/render, from Highlight Lootable Corpses) reads its options
// through this small interface. Outline.cpp fills it in from this mod's own Settings.

PLUGIN_NAMESPACE_BEGIN

struct Config
{
    enum class DisplayMode : uint8_t
    {
        e_silhouette = 0,
        e_outline    = 1
    };

    DisplayMode display_mode{ DisplayMode::e_outline };
    int outline_thickness{ 1 };
};

class Setting
{
public:
    static Setting& instance()
    {
        static Setting s_instance;
        return s_instance;
    }

    Config& get_config() noexcept { return m_config; }

public:
    constexpr static int Min_Outline_Thickness = 1;
    constexpr static int Max_Outline_Thickness = 20;
private:
    Config m_config;
};

PLUGIN_NAMESPACE_END
