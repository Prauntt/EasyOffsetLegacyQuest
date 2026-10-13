#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include "scotland2/shared/loader.hpp"

#include "GlobalNamespace/VRController.hpp"
#include "GlobalNamespace/IVRPlatformHelper.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Quaternion.hpp"
#include "UnityEngine/XR/XRNode.hpp"

#include <android/log.h>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <string>

#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, "EasyOffsetLegacyQuest", __VA_ARGS__)
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "EasyOffsetLegacyQuest", __VA_ARGS__)

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

static const std::string CONFIG_DIR = "/sdcard/ModData/com.beatgames.beatsaber/Configs/";
static const std::string CONFIG_FILE = CONFIG_DIR + "EasyOffsetLegacyQuest.json";
static const std::string PCVR_PRESET_FILE = CONFIG_DIR + "EasyOffsetPreset.json";

struct ModSettings {
    bool enabled = true;
    std::string headsetType = "Quest 3 / 3S"; // Options: "Quest 3 / 3S", "Quest 2", "Quest Pro"

    // Left Hand (cm and degrees, matching EasyOffset UI)
    float leftPivotX = 0.0f;
    float leftPivotY = 0.0f;
    float leftPivotZ = 0.0f;
    float leftRotX = 0.0f;
    float leftRotY = 0.0f;
    float leftRotZ = 0.0f;
    float leftZOffset = 0.0f;

    // Right Hand (cm and degrees, matching EasyOffset UI)
    float rightPivotX = 0.0f;
    float rightPivotY = 0.0f;
    float rightPivotZ = 0.0f;
    float rightRotX = 0.0f;
    float rightRotY = 0.0f;
    float rightRotZ = 0.0f;
    float rightZOffset = 0.0f;
} settings;

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

static UnityEngine::Vector3 AddVec(UnityEngine::Vector3 a, UnityEngine::Vector3 b) {
    return UnityEngine::Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
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

static void SaveConfig() {
    std::filesystem::create_directories(CONFIG_DIR);
    std::ofstream out(CONFIG_FILE);
    if (!out.is_open()) return;

    out << "{\n"
        << "  \"Enabled\": " << (settings.enabled ? "true" : "false") << ",\n"
        << "  \"HeadsetType\": \"" << settings.headsetType << "\",\n"
        << "  \"LeftPivotX\": " << settings.leftPivotX << ",\n"
        << "  \"LeftPivotY\": " << settings.leftPivotY << ",\n"
        << "  \"LeftPivotZ\": " << settings.leftPivotZ << ",\n"
        << "  \"LeftRotX\": " << settings.leftRotX << ",\n"
        << "  \"LeftRotY\": " << settings.leftRotY << ",\n"
        << "  \"LeftRotZ\": " << settings.leftRotZ << ",\n"
        << "  \"LeftZOffset\": " << settings.leftZOffset << ",\n"
        << "  \"RightPivotX\": " << settings.rightPivotX << ",\n"
        << "  \"RightPivotY\": " << settings.rightPivotY << ",\n"
        << "  \"RightPivotZ\": " << settings.rightPivotZ << ",\n"
        << "  \"RightRotX\": " << settings.rightRotX << ",\n"
        << "  \"RightRotY\": " << settings.rightRotY << ",\n"
        << "  \"RightRotZ\": " << settings.rightRotZ << ",\n"
        << "  \"RightZOffset\": " << settings.rightZOffset << "\n"
        << "}\n";
}

static void LoadConfigAndPresets() {
    std::filesystem::create_directories(CONFIG_DIR);

    // 1. Load main config file if it exists, otherwise create default
    if (std::filesystem::exists(CONFIG_FILE)) {
        std::ifstream f(CONFIG_FILE);
        std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        rapidjson::Document d;
        if (!d.Parse(s.c_str()).HasParseError() && d.IsObject()) {
            if (d.HasMember("Enabled") && d["Enabled"].IsBool()) settings.enabled = d["Enabled"].GetBool();
            if (d.HasMember("HeadsetType") && d["HeadsetType"].IsString()) settings.headsetType = d["HeadsetType"].GetString();
            if (d.HasMember("LeftPivotX") && d["LeftPivotX"].IsNumber()) settings.leftPivotX = d["LeftPivotX"].GetFloat();
            if (d.HasMember("LeftPivotY") && d["LeftPivotY"].IsNumber()) settings.leftPivotY = d["LeftPivotY"].GetFloat();
            if (d.HasMember("LeftPivotZ") && d["LeftPivotZ"].IsNumber()) settings.leftPivotZ = d["LeftPivotZ"].GetFloat();
            if (d.HasMember("LeftRotX") && d["LeftRotX"].IsNumber()) settings.leftRotX = d["LeftRotX"].GetFloat();
            if (d.HasMember("LeftRotY") && d["LeftRotY"].IsNumber()) settings.leftRotY = d["LeftRotY"].GetFloat();
            if (d.HasMember("LeftRotZ") && d["LeftRotZ"].IsNumber()) settings.leftRotZ = d["LeftRotZ"].GetFloat();
            if (d.HasMember("LeftZOffset") && d["LeftZOffset"].IsNumber()) settings.leftZOffset = d["LeftZOffset"].GetFloat();

            if (d.HasMember("RightPivotX") && d["RightPivotX"].IsNumber()) settings.rightPivotX = d["RightPivotX"].GetFloat();
            if (d.HasMember("RightPivotY") && d["RightPivotY"].IsNumber()) settings.rightPivotY = d["RightPivotY"].GetFloat();
            if (d.HasMember("RightPivotZ") && d["RightPivotZ"].IsNumber()) settings.rightPivotZ = d["RightPivotZ"].GetFloat();
            if (d.HasMember("RightRotX") && d["RightRotX"].IsNumber()) settings.rightRotX = d["RightRotX"].GetFloat();
            if (d.HasMember("RightRotY") && d["RightRotY"].IsNumber()) settings.rightRotY = d["RightRotY"].GetFloat();
            if (d.HasMember("RightRotZ") && d["RightRotZ"].IsNumber()) settings.rightRotZ = d["RightRotZ"].GetFloat();
            if (d.HasMember("RightZOffset") && d["RightZOffset"].IsNumber()) settings.rightZOffset = d["RightZOffset"].GetFloat();
        }
    } else {
        SaveConfig();
    }

    // 2. If user dropped a PCVR EasyOffsetPreset.json file in Configs, auto-import & save it!
    if (std::filesystem::exists(PCVR_PRESET_FILE)) {
        std::ifstream f(PCVR_PRESET_FILE);
        std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        rapidjson::Document doc;
        if (!doc.Parse(s.c_str()).HasParseError() && doc.IsObject()) {
            auto readCoord = [](float val) -> float {
                return (std::abs(val) <= 0.5f && val != 0.0f) ? (val * 100.0f) : val;
            };
            if (doc.HasMember("leftHandPivotPosition") && doc["leftHandPivotPosition"].IsObject()) {
                auto& p = doc["leftHandPivotPosition"];
                settings.leftPivotX = readCoord(p["x"].GetFloat());
                settings.leftPivotY = readCoord(p["y"].GetFloat());
                settings.leftPivotZ = readCoord(p["z"].GetFloat());
            }
            if (doc.HasMember("leftHandSaberRotation") && doc["leftHandSaberRotation"].IsObject()) {
                auto& r = doc["leftHandSaberRotation"];
                if (r.HasMember("w")) {
                    UnityEngine::Quaternion q(r["x"].GetFloat(), r["y"].GetFloat(), r["z"].GetFloat(), r["w"].GetFloat());
                    auto euler = q.get_eulerAngles();
                    settings.leftRotX = euler.x;
                    settings.leftRotY = euler.y;
                    settings.leftRotZ = euler.z;
                } else {
                    settings.leftRotX = r["x"].GetFloat();
                    settings.leftRotY = r["y"].GetFloat();
                    settings.leftRotZ = r["z"].GetFloat();
                }
            }
            if (doc.HasMember("leftHandZOffset")) settings.leftZOffset = readCoord(doc["leftHandZOffset"].GetFloat());

            if (doc.HasMember("rightHandPivotPosition") && doc["rightHandPivotPosition"].IsObject()) {
                auto& p = doc["rightHandPivotPosition"];
                settings.rightPivotX = readCoord(p["x"].GetFloat());
                settings.rightPivotY = readCoord(p["y"].GetFloat());
                settings.rightPivotZ = readCoord(p["z"].GetFloat());
            }
            if (doc.HasMember("rightHandSaberRotation") && doc["rightHandSaberRotation"].IsObject()) {
                auto& r = doc["rightHandSaberRotation"];
                if (r.HasMember("w")) {
                    UnityEngine::Quaternion q(r["x"].GetFloat(), r["y"].GetFloat(), r["z"].GetFloat(), r["w"].GetFloat());
                    auto euler = q.get_eulerAngles();
                    settings.rightRotX = euler.x;
                    settings.rightRotY = euler.y;
                    settings.rightRotZ = euler.z;
                } else {
                    settings.rightRotX = r["x"].GetFloat();
                    settings.rightRotY = r["y"].GetFloat();
                    settings.rightRotZ = r["z"].GetFloat();
                }
            }
            if (doc.HasMember("rightHandZOffset")) settings.rightZOffset = readCoord(doc["rightHandZOffset"].GetFloat());

            SaveConfig();
        }
    }
}

MAKE_HOOK_MATCH(
    VRController_Update,
    &GlobalNamespace::VRController::Update,
    void,
    GlobalNamespace::VRController* self
) {
    if (!settings.enabled) {
        VRController_Update(self);
        return;
    }

    auto node = self->_node;
    if (node != UnityEngine::XR::XRNode::LeftHand && node != UnityEngine::XR::XRNode::RightHand) {
        VRController_Update(self);
        return;
    }

    bool isLeft = (node == UnityEngine::XR::XRNode::LeftHand);

    UnityEngine::Vector3 rawPos = self->_lastTrackedPosition;
    UnityEngine::Quaternion rawRot = UnityEngine::Quaternion::get_identity();

    if (self->_vrPlatformHelper != nullptr) {
        bool valid = self->_vrPlatformHelper->GetNodePose(node, self->_nodeIdx, byref(rawPos), byref(rawRot));
        if (valid) {
            self->_lastTrackedPosition = rawPos;
        } else {
            rawPos = self->_lastTrackedPosition;
        }
    }

    HeadsetDelta hmdDelta = GetHeadsetDelta(settings.headsetType);
    float sign = isLeft ? -1.0f : 1.0f;

    UnityEngine::Vector3 deltaPos(hmdDelta.pos.x * sign, hmdDelta.pos.y, hmdDelta.pos.z);
    UnityEngine::Quaternion deltaRot = UnityEngine::Quaternion::Euler(
        hmdDelta.rotEuler.x,
        hmdDelta.rotEuler.y * sign,
        hmdDelta.rotEuler.z * sign
    );

    UnityEngine::Vector3 ovrBasePos = AddVec(rawPos, RotateVector(rawRot, deltaPos));
    UnityEngine::Quaternion ovrBaseRot = MultiplyQuat(rawRot, deltaRot);

    UnityEngine::Vector3 pivotPos;
    UnityEngine::Quaternion eoRot;
    float zOffsetMeters = 0.0f;

    if (isLeft) {
        pivotPos = UnityEngine::Vector3(settings.leftPivotX * 0.01f, settings.leftPivotY * 0.01f, settings.leftPivotZ * 0.01f);
        eoRot = UnityEngine::Quaternion::Euler(settings.leftRotX, settings.leftRotY, settings.leftRotZ);
        zOffsetMeters = settings.leftZOffset * 0.01f;
    } else {
        pivotPos = UnityEngine::Vector3(settings.rightPivotX * 0.01f, settings.rightPivotY * 0.01f, settings.rightPivotZ * 0.01f);
        eoRot = UnityEngine::Quaternion::Euler(settings.rightRotX, settings.rightRotY, settings.rightRotZ);
        zOffsetMeters = settings.rightZOffset * 0.01f;
    }

    UnityEngine::Quaternion finalLocalRot = MultiplyQuat(ovrBaseRot, eoRot);
    UnityEngine::Vector3 worldPivotPos = AddVec(ovrBasePos, RotateVector(ovrBaseRot, pivotPos));
    UnityEngine::Vector3 zOffsetVec(0.0f, 0.0f, zOffsetMeters);
    UnityEngine::Vector3 finalLocalPos = AddVec(worldPivotPos, RotateVector(finalLocalRot, zOffsetVec));

    auto transform = self->get_transform();
    transform->set_localPosition(finalLocalPos);
    transform->set_localRotation(finalLocalRot);
}

extern "C" __attribute__((visibility("default"))) void setup(CModInfo* info) noexcept {
    *info = modInfo.to_c();
    LoadConfigAndPresets();
    LOG_INFO("EasyOffsetLegacyQuest setup complete!");
}

extern "C" __attribute__((visibility("default"))) void late_load() noexcept {
    il2cpp_functions::Init();
    LoadConfigAndPresets();
    INSTALL_HOOK_ORIG(EasyOffsetLegacyQuest, VRController_Update);
    LOG_INFO("EasyOffsetLegacyQuest hooks installed!");
}
