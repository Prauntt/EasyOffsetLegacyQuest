#pragma once
#include "config-utils/shared/config-utils.hpp"
#include <string>

DECLARE_CONFIG(ModConfig) {
    CONFIG_VALUE(Enabled, bool, "Enable 1.29.1 EasyOffset Math", true);

    // Headset Profile: "Quest 3 / 3S", "Quest 2", or "Quest Pro"
    CONFIG_VALUE(HeadsetType, std::string, "Headset Profile", "Quest 3 / 3S");

    // LEFT HAND SETTINGS (cm & degrees)
    CONFIG_VALUE(LeftPivotX, float, "Left Pivot X (cm)", 0.0f);
    CONFIG_VALUE(LeftPivotY, float, "Left Pivot Y (cm)", 0.0f);
    CONFIG_VALUE(LeftPivotZ, float, "Left Pivot Z (cm)", 0.0f);
    CONFIG_VALUE(LeftRotX, float, "Left Rotation X (deg)", 0.0f);
    CONFIG_VALUE(LeftRotY, float, "Left Rotation Y (deg)", 0.0f);
    CONFIG_VALUE(LeftRotZ, float, "Left Rotation Z (deg)", 0.0f);
    CONFIG_VALUE(LeftZOffset, float, "Left Z-Offset (cm)", 0.0f);

    // RIGHT HAND SETTINGS (cm & degrees)
    CONFIG_VALUE(RightPivotX, float, "Right Pivot X (cm)", 0.0f);
    CONFIG_VALUE(RightPivotY, float, "Right Pivot Y (cm)", 0.0f);
    CONFIG_VALUE(RightPivotZ, float, "Right Pivot Z (cm)", 0.0f);
    CONFIG_VALUE(RightRotX, float, "Right Rotation X (deg)", 0.0f);
    CONFIG_VALUE(RightRotY, float, "Right Rotation Y (deg)", 0.0f);
    CONFIG_VALUE(RightRotZ, float, "Right Rotation Z (deg)", 0.0f);
    CONFIG_VALUE(RightZOffset, float, "Right Z-Offset (cm)", 0.0f);
};
