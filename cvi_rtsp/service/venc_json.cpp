#include <cvi_rtsp_service/venc_json.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sample_comm.h>

static void parse_coding_param(const nlohmann::json &params, chnInputCfg *pIc)
{
    const auto section = "Coding Param";
    if (params.find(section) == params.end()) {
        std::cout << "No " << section << " params, use default value" << std::endl;
        return;
    }

    auto coding_param = params.at(section);
    for (auto &item : coding_param.at("items")) {
        std::string key = item.at("key");
        if (key == "FrmLostOpen") {
            pIc->frameLost = item.at("value");
        } else if (key == "LostMode") {
            //std::cout << key << " not support yet." << std::endl;
        } else if (key == "FrmLostBpsThr") {
            pIc->frameLostBspThr = item.at("value");
        } else if (key == "EncFrmGaps") {
            pIc->frameLostGap = item.at("value");
        } else if (key == "IntraCost") {
            pIc->u32IntraCost = item.at("value");
        } else if (key == "ChromaQpOffset") {
            pIc->h264ChromaQpOffset = item.at("value");
        } else if (key == "CbQpOffset") {
            pIc->h265CbQpOffset = item.at("value");
        } else if (key == "CrQpOffset") {
            pIc->h265CrQpOffset = item.at("value");
        } else if (key == "aspectRatioInfoPresentFlag") {
            pIc->aspectRatioInfoPresentFlag = item.at("value");
        } else if (key == "overscanInfoPresentFlag") {
            pIc->overscanInfoPresentFlag = item.at("value");
        } else if (key == "videoSignalTypePresentFlag") {
            pIc->videoSignalTypePresentFlag = item.at("value");
        } else if (key == "videoFormat") {
            pIc->videoFormat = item.at("value");
        } else if (key == "videoFullRangeFlag") {
            pIc->videoFullRangeFlag = item.at("value");
        } else if (key == "colourDescriptionPresentFlag") {
            pIc->colourDescriptionPresentFlag = item.at("value");
        } else {
            std::cout << "unknown parameter " << key << ": " << item.at("value") << std::endl;
        }
#ifdef DEBUG
        std::cout << key << ": " << item.at("value") << std::endl;
#endif
    }
}

static void parse_gop_mode(const nlohmann::json &params, chnInputCfg *pIc)
{
    const auto section = "Gop Mode";
    if (params.find(section) == params.end()) {
        std::cout << "No " << section << " params, use default value" << std::endl;
        return;
    }

    auto gop_mode = params.at(section);
    for (auto &item: gop_mode["items"]) {
        std::string key = item.at("key");
        if (key == "GopMode") {
            pIc->gopMode = item.at("value");
        } else if (key == "IPQpDelta") {
            pIc->s32IPQpDelta = item.at("value");
        } else if (key == "BgInterval") {
            pIc->bgInterval = item.at("value");
        } else if (key == "BgQpDelta") {
            //stGopAttr->stSmartP.s32BgQpDelta = item.at("value");
            //stGopAttr->stAdvSmartP.s32BgQpDelta = item.at("value");
        } else if (key == "ViQpDelta") {
            //stGopAttr->stSmartP.s32ViQpDelta= item.at("value");
            //stGopAttr->stAdvSmartP.s32ViQpDelta = item.at("value");
        } else {
            std::cout << "unknown parameter " << key << ": " << item.at("value") << std::endl;
        }
#ifdef DEBUG
        std::cout << key << ": " << item.at("value") << std::endl;
#endif
    }
}

static void parse_rc_attr(const nlohmann::json &params, chnInputCfg *pIc)
{
    const auto section = "RC Attr";
    if (params.find(section) == params.end()) {
        std::cout << "No " << section << " params, use default value" << std::endl;
        return;
    }

    auto rc_attr = params.at(section);
    for (auto &item: rc_attr["items"]) {
        std::string key = item.at("key");
        if (key == "RcMode") {
            pIc->rcMode = item.at("value");
        } else if (key == "Gop") {
            pIc->gop = item.at("value");
        } else if (key == "VariableFPS") {
            pIc->bVariFpsEn = item.at("value");
        } else if (key == "SrcFrmRate") {
            pIc->srcFramerate = item.at("value");
        } else if (key == "DstFrmRate") {
            pIc->framerate = item.at("value");
        } else if (key == "StatTime") {
            pIc->statTime = item.at("value");
        } else if (key == "BitRate") {
            pIc->bitrate = item.at("value");
        } else if (key == "MaxBitrate") {
            pIc->maxbitrate = item.at("value");
        } else if (key == "IQP") {
            pIc->iqp = item.at("value");
        } else if (key == "PQP") {
            pIc->pqp = item.at("value");
        } else {
            std::cout << "unknown parameter " << key << ": " << item.at("value") << std::endl;
        }
#ifdef DEBUG
        std::cout << key << ": " << item.at("value") << std::endl;
#endif
    }
}

static void parse_rc_param(const nlohmann::json &params, chnInputCfg *pIc)
{
    const auto section = "RC Param";
    if (params.find(section) == params.end()) {
        std::cout << "No " << section << " params, use default value" << std::endl;
        return;
    }

    auto rc_param = params.at(section);
    for (auto &item: rc_param["items"]) {
        std::string key = item.at("key");
        if (key == "ThrdLv") {
            pIc->u32ThrdLv = item.at("value");
        } else if (key == "RowQpDelta") {
            pIc->u32RowQpDelta = item.at("value");
        } else if (key == "FirstFrameStartQp") {
            pIc->firstFrmstartQp = item.at("value");
        } else if (key == "InitialDelay") {
            pIc->initialDelay = item.at("value");
        } else if (key == "MaxIprop") {
            pIc->maxIprop = item.at("value");
        } else if (key == "MaxQp") {
            pIc->maxQp = item.at("value");
        } else if (key == "MinQp") {
            pIc->minQp = item.at("value");
        } else if (key == "MaxIQp") {
            pIc->maxIqp = item.at("value");
        } else if (key == "MinIQp") {
            pIc->minIqp = item.at("value");
        } else if (key == "ChangePos") {
            pIc->s32ChangePos = item.at("value");
        } else if (key == "MinStillPercent") {
            pIc->s32MinStillPercent = item.at("value");
        } else if (key == "MaxStillQP") {
            pIc->u32MaxStillQP = item.at("value");
        } else if (key == "MotionSensitivity") {
            pIc->u32MotionSensitivity = item.at("value");
        } else if (key == "PureStillThr") {
            pIc->s32AvbrPureStillThr = item.at("value");
        } else if (key == "AvbrFrmLostOpen") {
            pIc->s32AvbrFrmLostOpen = item.at("value");
        } else if (key == "AvbrFrmGap") {
            pIc->s32AvbrFrmGap = item.at("value");
        } else {
            std::cout << "unknown parameter " << key << ": " << item.at("value") << std::endl;
        }
#ifdef DEBUG
        std::cout << key << ": " << item.at("value") << std::endl;
#endif
    }
}

int CVI_RTSP_SERVICE_SetVencCfg(const char* venc_json, chnInputCfg *pIc)
{
    if (venc_json == nullptr || strlen(venc_json) == 0 || pIc == nullptr) {
        return -1;
    }

    std::ifstream file;
    file.open(venc_json);
    if (!file.is_open()) {
        std::cout << "Open Venc Json file: " << venc_json << " failed, use default VC setting !" << std::endl;
        return -1;
    }

    nlohmann::json params;

    try {
        file >> params;
    } catch (...) {
        std::cout << "fail to parse venc config " << venc_json << std::endl;
        file.close();
        return -1;
    }

    file.close();

    parse_coding_param(params, pIc);
    parse_gop_mode(params, pIc);
    parse_rc_attr(params, pIc);
    parse_rc_param(params, pIc);

    return 0;
}

