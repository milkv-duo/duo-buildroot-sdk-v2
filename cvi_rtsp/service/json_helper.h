#pragma once

#include "ctx.h"
#include <nlohmann/json.hpp>

int load_json_config(SERVICE_CTX *ctx, const  nlohmann::json &params);
void dump_venc_cfg(chnInputCfg *venc_cfg);
int json_parse_from_file(const char *jsonFile, nlohmann::json &params);
int json_parse_from_string(const char *jsonStr, nlohmann::json &params);
