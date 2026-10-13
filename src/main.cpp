#include "ModConfig.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include "scotland2/shared/loader.hpp"
#include "paper2_scotland2/shared/logger.hpp"

#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSMLDataCache.hpp"
#include "HMUI/ViewController.hpp"

#include "GlobalNamespace/VRController.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/XR/XRNode.hpp"
#include "UnityEngine/XR/InputTracking.hpp"

#include <fstream>
#include <filesystem>

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};
constexpr auto Logger = Paper::ConstLoggerContext("EasyOffsetLegacyQuest");

static const std::string PRESET_FILE_PATH = "/sdcard/ModData/com.beatgames.beatsaber/Configs/EasyOffsetPreset.json";

struct HeadsetDelta {
    UnityEngine::Vector3 pos;
    UnityEngine::Vector3 rotEuler;
};

static HeadsetDelta GetHeadsetDelta(const std::string& headset) {
    if (headset == "Quest 2") {
        return { UnityEngine::Vector3(0.0f, -0.0080f, -0.0150f), UnityEngine::Vector3(40.0f, 0.0f, 0.0f) };
    } else if (headset == "Quest Pro") {
        return { UnityEngine::Vector3(0.0015f, -0.0065f, -0.0135f), UnityEngine::Vector3(39.2f, 0.0f, 0.0f) };
    }
    // Default: Quest 3 / 3S
    return { UnityEngine::Vector3(0.0f, -0.0095f, -0.0168f), UnityEngine::Vector3(40.0f, 0.0f, 0.0f) };
}

static UnityEngine::Vector3 RotateVector(UnityEngine::Quaternion q, UnityEngine::Vector3 v) {
    UnityEngine::Vector3 u(q.x, q.y, q.z);
    float s = q.w;
    float dotUV = u.x * v.x + u.y * v.y + u.z * v.z;
    float dotUU = u.x * u.x + u.y * u.y + u.z * u.z;
    UnityEngine::Vector3 crossUV(
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x
    );
    float factorU = 2.0f * dotUV;
    float factorV = (s * s) - dotUU;
    float factorCross = 2.0f * s;
    return UnityEngine::Vector3(
        factorU * u.x + factorV * v.x + factorCross * crossUV.x,
        factorU * u.y + factorV * v.y + factorCross * crossUV.y,
        factorU * u.z + factorV * v.z + factorCross * crossUV.z
    );
}

static UnityEngine::Quaternion MultiplyQuat(UnityEngine::Quaternion lhs, UnityEngine::Quaternion rhs) {
    return UnityEngine::Quaternion(
        lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x,
        lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z
    );
}

static bool ImportPCVREasyOffsetPreset() {
    if (!std::filesystem::exists(PRESET_FILE_PATH)) {
        return false;
    }

    std::ifstream file(PRESET_FILE_PATH);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError() || !doc.IsObject()) {
        return false;
    }

    auto readCoord = [](float val) -> float {
        return (std::abs(val) <= 0.5f && val != 0.0f) ? (val * 100.0f) : val;
    };

    if (doc.HasMember("leftHandPivotPosition") && doc["leftHandPivotPosition"].IsObject()) {
        auto& p = doc["leftHandPivotPosition"];
        getModConfig().LeftPivotX.SetValue(readCoord(p["x"].GetFloat()));
        getModConfig().LeftPivotY.SetValue(readCoord(p["y"].GetFloat()));
        getModConfig().LeftPivotZ.SetValue(readCoord(p["z"].GetFloat()));
    }
    if (doc.HasMember("leftHandSaberRotation") && doc["leftHandSaberRotation"].IsObject()) {
        auto& r = doc["leftHandSaberRotation"];
        if (r.HasMember("w")) {
            UnityEngine::Quaternion q(r["x"].GetFloat(), r["y"].GetFloat(), r["z"].GetFloat(), r["w"].GetFloat());
            auto euler = q.get_eulerAngles();
            getModConfig().LeftRotX.SetValue(euler.x);
            getModConfig().LeftRotY.SetValue(euler.y);
            getModConfig().LeftRotZ.SetValue(euler.z);
        } else {
            getModConfig().LeftRotX.SetValue(r["x"].GetFloat());
            getModConfig().LeftRotY.SetValue(r["y"].GetFloat());
            getModConfig().LeftRotZ.SetValue(r["z"].GetFloat());
        }
    }
    if (doc.HasMember("leftHandZOffset")) {
        getModConfig().LeftZOffset.SetValue(readCoord(doc["leftHandZOffset"].GetFloat()));
    }

    if (doc.HasMember("rightHandPivotPosition") && doc["rightHandPivotPosition"].IsObject()) {
        auto& p = doc["rightHandPivotPosition"];
        getModConfig().RightPivotX.SetValue(readCoord(p["x"].GetFloat()));
        getModConfig().RightPivotY.SetValue(readCoord(p["y"].GetFloat()));
        getModConfig().RightPivotZ.SetValue(readCoord(p["z"].GetFloat()));
    }
    if (doc.HasMember("rightHandSaberRotation") && doc["rightHandSaberRotation"].IsObject()) {
        auto& r = doc["rightHandSaberRotation"];
        if (r.HasMember("w")) {
            UnityEngine::Quaternion q(r["x"].GetFloat(), r["y"].GetFloat(), r["z"].GetFloat(), r["w"].GetFloat());
            auto euler = q.get_eulerAngles();
            getModConfig().RightRotX.SetValue(euler.x);
            getModConfig().RightRotY.SetValue(euler.y);
            getModConfig().RightRotZ.SetValue(euler.z);
        } else {
            getModConfig().RightRotX.SetValue(r["x"].GetFloat());
            getModConfig().RightRotY.SetValue(r["y"].GetFloat());
            getModConfig().RightRotZ.SetValue(r["z"].GetFloat());
        }
    }
    if (doc.HasMember("rightHandZOffset")) {
        getModConfig().RightZOffset.SetValue(readCoord(doc["rightHandZOffset"].GetFloat()));
    }

    return true;
}

MAKE_HOOK_MATCH(
    VRController_Update,
    &GlobalNamespace::VRController::Update,
    void,
    GlobalNamespace::VRController* self
) {
    if (!getModConfig().Enabled.GetValue()) {
        VRController_Update(self);
        return;
    }

    auto node = self->_node;
    if (node != UnityEngine::XR::XRNode::LeftHand && node != UnityEngine::XR::XRNode::RightHand) {
        VRController_Update(self);
        return;
    }

    bool isLeft = (node == UnityEngine::XR::XRNode::LeftHand);

    UnityEngine::Vector3 rawPos = UnityEngine::XR::InputTracking::GetLocalPosition(node);
    UnityEngine::Quaternion rawRot = UnityEngine::XR::InputTracking::GetLocalRotation(node);

    if (rawPos.x == 0.0f && rawPos.y == 0.0f && rawPos.z == 0.0f &&
        rawRot.x == 0.0f && rawRot.y == 0.0f && rawRot.z == 0.0f) {
        rawPos = self->_lastTrackedPosition;
    } else {
        self->_lastTrackedPosition = rawPos;
    }

    HeadsetDelta hmdDelta = GetHeadsetDelta(getModConfig().HeadsetType.GetValue());
    float sign = isLeft ? -1.0f : 1.0f;

    UnityEngine::Vector3 deltaPos(hmdDelta.pos.x * sign, hmdDelta.pos.y, hmdDelta.pos.z);
    UnityEngine::Quaternion deltaRot = UnityEngine::Quaternion::Euler(
        hmdDelta.rotEuler.x,
        hmdDelta.rotEuler.y * sign,
        hmdDelta.rotEuler.z * sign
    );

    UnityEngine::Vector3 ovrBasePos = rawPos + RotateVector(rawRot, deltaPos);
    UnityEngine::Quaternion ovrBaseRot = MultiplyQuat(rawRot, deltaRot);

    UnityEngine::Vector3 pivotPos;
    UnityEngine::Quaternion eoRot;
    float zOffsetMeters = 0.0f;

    if (isLeft) {
        pivotPos = UnityEngine::Vector3(
            getModConfig().LeftPivotX.GetValue() * 0.01f,
            getModConfig().LeftPivotY.GetValue() * 0.01f,
            getModConfig().LeftPivotZ.GetValue() * 0.01f
        );
        eoRot = UnityEngine::Quaternion::Euler(
            getModConfig().LeftRotX.GetValue(),
            getModConfig().LeftRotY.GetValue(),
            getModConfig().LeftRotZ.GetValue()
        );
        zOffsetMeters = getModConfig().LeftZOffset.GetValue() * 0.01f;
    } else {
        pivotPos = UnityEngine::Vector3(
            getModConfig().RightPivotX.GetValue() * 0.01f,
            getModConfig().RightPivotY.GetValue() * 0.01f,
            getModConfig().RightPivotZ.GetValue() * 0.01f
        );
        eoRot = UnityEngine::Quaternion::Euler(
            getModConfig().RightRotX.GetValue(),
            getModConfig().RightRotY.GetValue(),
            getModConfig().RightRotZ.GetValue()
        );
        zOffsetMeters = getModConfig().RightZOffset.GetValue() * 0.01f;
    }

    UnityEngine::Quaternion finalLocalRot = MultiplyQuat(ovrBaseRot, eoRot);
    UnityEngine::Vector3 worldPivotPos = ovrBasePos + RotateVector(ovrBaseRot, pivotPos);
    UnityEngine::Vector3 zOffsetVec(0.0f, 0.0f, zOffsetMeters);
    UnityEngine::Vector3 finalLocalPos = worldPivotPos + RotateVector(finalLocalRot, zOffsetVec);

    auto transform = self->get_transform();
    transform->set_localPosition(finalLocalPos);
    transform->set_localRotation(finalLocalRot);
}

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if (!firstActivation) return;

    auto container = BSML::Lite::CreateScrollableSettingsContainer(self->get_transform());

    AddConfigValueToggle(container->get_transform(), getModConfig().Enabled);

    auto hmdBtn = BSML::Lite::CreateUIButton(
        container->get_transform(),
        "Headset: " + getModConfig().HeadsetType.GetValue(),
        []() {
            std::string current = getModConfig().HeadsetType.GetValue();
            std::string next = "Quest 3 / 3S";
            if (current == "Quest 3 / 3S") next = "Quest 2";
            else if (current == "Quest 2") next = "Quest Pro";
            getModConfig().HeadsetType.SetValue(next);
        }
    );

    BSML::Lite::CreateUIButton(
        container->get_transform(),
        "Import EasyOffsetPreset.json",
        []() {
            ImportPCVREasyOffsetPreset();
        }
    );

    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().LeftPivotX, 1, 0.1f, -15.0f, 15.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().LeftPivotY, 1, 0.1f, -15.0f, 15.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().LeftPivotZ, 1, 0.1f, -15.0f, 15.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().LeftRotX, 1, 0.5f, -180.0f, 180.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().LeftRotY, 1, 0.5f, -180.0f, 180.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().LeftRotZ, 1, 0.5f, -180.0f, 180.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().LeftZOffset, 1, 0.1f, -15.0f, 15.0f);

    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().RightPivotX, 1, 0.1f, -15.0f, 15.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().RightPivotY, 1, 0.1f, -15.0f, 15.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().RightPivotZ, 1, 0.1f, -15.0f, 15.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().RightRotX, 1, 0.5f, -180.0f, 180.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().RightRotY, 1, 0.5f, -180.0f, 180.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().RightRotZ, 1, 0.5f, -180.0f, 180.0f);
    AddConfigValueIncrementFloat(container->get_transform(), getModConfig().RightZOffset, 1, 0.1f, -15.0f, 15.0f);
}

extern "C" void setup(CModInfo* info) {
    info->id = MOD_ID;
    info->version = VERSION;
    info->version_long = 0;
    modInfo = *info;

    getModConfig().Init(modInfo);
    ImportPCVREasyOffsetPreset();
}

extern "C" void late_load() {
    il2cpp_functions::Init();
    BSML::Init();
    BSML::Register::RegisterSettingsMenu("EasyOffset 1.29.1", DidActivate, false);
    INSTALL_HOOK(Logger, VRController_Update);
}
