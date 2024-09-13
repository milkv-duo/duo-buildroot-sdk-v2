#pragma once

#include "ctx.h"
#include <nlohmann/json.hpp>

int load_json_config(SERVICE_CTX *ctx, const  nlohmann::json &params);
void dump_venc_cfg(chnInputCfg *venc_cfg);
