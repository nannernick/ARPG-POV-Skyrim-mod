//
// Created by AmazingBuff on 2026/9/13.
//

#pragma once

#define MASK_NAMESPACE_BEGIN PLUGIN_NAMESPACE_BEGIN namespace Mask {
#define MASK_NAMESPACE_END PLUGIN_NAMESPACE_END }

MASK_NAMESPACE_BEGIN

// mask RT clear color
inline constexpr float Mask_Clear_Color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
// Mask value for the player when it hides a target behind it; equal to OCCLUDER_ID in mask_glow.hlsl.
inline constexpr uint32_t Occluder_Id = 0xFFFFFFFEu;

MASK_NAMESPACE_END
