#coding:utf-8
import sys
import re
import json
from jinja2 import Template
#TODO add parse marco to auto know defineDict-oliver

#log print config
ENABLE_ENUMJSON_LOG = 0
ENABLE_PRINT_ENUMJSON = 0
ENABLE_STRUCTJSON_LOG = 0
ENABLE_PRINT_STRUCTJSON = 0

#use cusaliasDict cusTitleCountDict cusTypeDict replace h file content
USE_PATCH = 1
#use level.json apply
USE_LEVEL = 1

#hardCode content
hardCodeContent = \
"""
typedef struct _ISP_TOP_ATTR_S {
    CVI_U8 ViPipe; /*RW; Range:[0x0, 0x3]*/
    CVI_U8 ViChn; /*RW; Range:[0x0, 0x3]*/
    CVI_U8 VpssGrp; /*RW; Range:[0x0, 0xF]*/
    CVI_U8 VpssChn; /*RW; Range:[0x0, 0x3]*/
} ISP_TOP_ATTR_S;

typedef struct _PQT_ISP_AE_ROUTE_S {
	CVI_U32 u32TotalNum; /*RW; Range:[0x1, 0x10]*/
	CVI_U32 astRouteNode[16][4];
} PQT_ISP_AE_ROUTE_S;

typedef struct _PQT_AE_ROUTE_S {
	PQT_ISP_AE_ROUTE_S ISP_AE_ROUTE_S;
} PQT_AE_ROUTE_S;

typedef struct _PQT_AE_ROUTE_SF_S {
	PQT_ISP_AE_ROUTE_S ISP_AE_ROUTE_S;
} PQT_AE_ROUTE_SF_S;

typedef struct _PQT_ISP_AE_ROUTE_EX_S {
	CVI_U32 u32TotalNum; /*RW; Range:[0x1, 0x10]*/
	CVI_U32 astRouteExNode[16][4];
} PQT_ISP_AE_ROUTE_EX_S;

typedef struct _PQT_AE_ROUTE_EX_S {
	PQT_ISP_AE_ROUTE_EX_S ISP_AE_ROUTE_EX_S;
} PQT_AE_ROUTE_EX_S;

typedef struct _PQT_AE_ROUTE_EX_SF_S {
	PQT_ISP_AE_ROUTE_EX_S ISP_AE_ROUTE_EX_S;
} PQT_AE_ROUTE_EX_SF_S;

typedef struct _VPSS_ATTR_S {
	CVI_S32 brightness; /*RW; Range:[0x0, 0x64]*/
	CVI_S32 contrast; /*RW; Range:[0x0, 0x64]*/
	CVI_S32 saturation; /*RW; Range:[0x0, 0x64]*/
	CVI_S32 hue; /*RW; Range:[0x0, 0x64]*/
} VPSS_ADJUSTMENT_ATTR_S;

typedef struct _VO_GAMMA_INFO_S {
	CVI_BOOL enable;
	CVI_BOOL osd_apply;
	CVI_U8 value[65];
} VO_GAMMA_INFO_S;

typedef struct _VO_BIN_INFO_S {
	VO_GAMMA_INFO_S gamma_info;
	CVI_U32 guard_magic;
} VO_BIN_INFO_S;

"""

rpcHardCode = \
"""
			{
				"STRUCT": "VPSS_ADJUSTMENT_ATTR_S",
				"GET": "CVI_VPSS_GetGrpProcAmp",
				"SET": "CVI_VPSS_SetGrpProcAmp"
			},
"""

levelDict = {}
typeDict = {"CVI_BOOL":["0x0","0x1"],"CVI_U8":["0x0","0xFF"],"CVI_U16":["0x0","0xFFFF"],"CVI_U32":["0x0","0xFFFFFFFF"],"CVI_U64":["0x0","0xFFFFFFFFFFFFFFFF"],
	        "CVI_S8":["-0x80","0x7F"],"CVI_S16":["-0x8000","0x7FFF"],"CVI_S32":["-0x80000000","0x7fffffff"],"CVI_S64":["-2147483648","2147483647"]}

defineDict = {"ISP_AUTO_ISO_STRENGTH_NUM":
                   [16,["ISO100",
                        "ISO200",
                        "ISO400",
                        "ISO800",
                        "ISO1600",
                        "ISO3200",
                        "ISO6400",
                        "ISO12800",
                        "ISO25600",
                        "ISO51200",
                        "ISO102400",
                        "ISO204800",
                        "ISO409600",
                        "ISO819200",
                        "ISO1638400",
                        "ISO3276800"]],
                "ISP_AUTO_LV_NUM":
                   [21,["LV-5",
                         "LV-4",
                         "LV-3",
                         "LV-2",
                         "LV-1",
                         "LV0",
                         "LV1",
                         "LV2",
                         "LV3",
                         "LV4",
                         "LV5",
                         "LV6",
                         "LV7",
                         "LV8",
                         "LV9",
                         "LV10",
                         "LV11",
                         "LV12",
                         "LV13",
                         "LV14",
                         "LV15"]],
              "LV_TOTAL_NUM":
                    [21,["LV-5",
                         "LV-4",
                         "LV-3",
                         "LV-2",
                         "LV-1",
                         "LV0",
                         "LV1",
                         "LV2",
                         "LV3",
                         "LV4",
                         "LV5",
                         "LV6",
                         "LV7",
                         "LV8",
                         "LV9",
                         "LV10",
                         "LV11",
                         "LV12",
                         "LV13",
                         "LV14",
                         "LV15"]],
              "AWB_CURVE_BOUND_NUM":
                    [8,["Low_Down",
                        "Low_Up",
                        "Mid1_Down",
                        "Mid1_Up",
                        "Mid2_Down",
                        "Mid2_Up",
                        "High_Down",
                        "High_Up"]],
              }
cusaliasDict = {}
cusTitleCountDict = {}
cusTypeDict = {}
cusRangeDict = {}
cusAccess = {}
if USE_PATCH:
    cusaliasDict = {
                "PQT_AE_ROUTE_S":{"PQT_AE_ROUTE_S":"AE Route","ISP_AE_ROUTE_S":"AE Route"},
                "PQT_AE_ROUTE_SF_S":{"PQT_AE_ROUTE_SF_S":"AE RouteSF","ISP_AE_ROUTE_S":"AE RouteSF"},
                "PQT_AE_ROUTE_EX_S":{"PQT_AE_ROUTE_EX_S":"AE RouteEx","ISP_AE_ROUTE_EX_S":"AE RouteEx"},
                "PQT_AE_ROUTE_EX_SF_S":{"PQT_AE_ROUTE_EX_SF_S":"AE RouteExSF","ISP_AE_ROUTE_EX_S":"AE RouteExSF"},
                "ISP_MESH_SHADING_GAIN_LUT_ATTR_S":{"Size":"LscGainLut Size"},
                "VO_GAMMA_INFO_S":{"enable":"Enable","osd_apply":"OSD Apply","value":"Table"},
                "ISP_EXP_INFO_S":{"au32AE_Hist1024Value":"AE_Hist256Value"}
                }
    cusTitleCountDict = {"ISP_COLORMATRIX_ATTR_S":{"CCM":[["R","G","B"],["R","G","B"],[3,3]]},
                        "ISP_CCM_MANUAL_ATTR_S":{"CCM":[["R","G","B"],["R","G","B"],[3,3]]},
                        "PQT_ISP_AE_ROUTE_S":{"astRouteNode":[["IntTime","SysGain","IrisFNO","IrisFNOLin"],["1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16"],[16,4]]},
                        "PQT_ISP_AE_ROUTE_EX_S":{"astRouteExNode":[["IntTime","Again","Dgain","IspDgain","IrisFNO","IrisFNOLin"],["1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16"],[16,6]]},
                        "ISP_AWB_ATTR_S":{"au8ZoneWt":[0, 0, [32,32]]},
                        "ISP_WB_INFO_S":{"au16CCM":[0, 0, [3,3]]}
                        }
    cusTypeDict = {"ISP_EXP_INFO_S":{"stAERoute":["PQT_ISP_AE_ROUTE_S"],"stAERouteEx":["PQT_ISP_AE_ROUTE_EX_S"],"stAERouteSF":["PQT_ISP_AE_ROUTE_S"],"stAERouteSFEx":["PQT_ISP_AE_ROUTE_EX_S"]}}
    cusRangeDict = {"ISP_PUB_ATTR_S":{"f32FrameRate":[0,65535]},
                    "ISP_CMOS_NOISE_CALIBRATION_S":{"CalibrationCoef":[0,9999]},
                    "ISP_EXP_INFO_S":{"fLightValue":[-128,127]},
                    "PQT_ISP_AE_ROUTE_S":{"astRouteNode":[0,2147483647,0,2147483647,0,10,0,1024]},
                    "PQT_ISP_AE_ROUTE_EX_S":{"astRouteExNode":[0,2147483647,0,2147483647,0,2147483647,0,2147483647,0,10,0,1024]}}
    cusAccess = {"ISP_PUB_ATTR_S":{"stWndRect":"R","stSnsSize":"R","enBayer":"R","enWDRMode":"R","u8SnsMode":"R"}}

def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        pass

    try:
        import unicodedata
        unicodedata.numeric(s)
        return True
    except (TypeError, ValueError):
        pass

    return False

class MarcoDict():
    def __init__(self, strlist):
        self.marcoDict = {}
        for str in strlist:
            key, value = self.parseStr(str)
            self.marcoDict.update({key:value})
        self.marcoDict.update({"LV_TOTAL_NUM":"21"})
        self.marcoDict.update({"ISP_AWB_COLORTEMP_NUM":"2"})
        self.marcoDict.update({"ISP_BAYER_CHN_NUM":"4"})
        self.marcoDict.update({"AWB_ZONE_WT_NUM":"1024"})
        self.marcoDict.update({"ISP_RLSC_COLOR_TEMPERATURE_SIZE":"7"})
        self.marcoDict.update({"ISP_MLSC_COLOR_TEMPERATURE_SIZE":"7"})
        self.marcoDict.update({"CVI_ISP_LSC_GRID_POINTS":"1369"})
        self.marcoDict.update({"ISP_CHANNEL_MAX_NUM":"3"})
        self.marcoDict.update({"MAX_HIST_BINS":"256"})
        self.marcoDict.update({"AWB_ZONE_NUM":"1024"})
        self.marcoDict.update({"ISP_WDR_FRAME_IDX_SIZE":"4"})
        self.marcoDict.update({"VO_LVDS_LANE_MAX":"5"})
        self.marcoDict.update({"PROC_AMP_MAX":"4"})
        self.addViNumMarcoDict()
    def parseStr(self, str):
        str = re.sub("\s+"," ",str)
        name = str.split(" ")[1]
        if is_number(str.split(" ")[2]):
            if "0x" in str.split(" ")[2]:
                num =  int(str.split(" ")[2].replace("(","").replace(")",""),16)
            else:
                num =  int(str.split(" ")[2].replace("(","").replace(")",""),10)
        else:
            num = str.split(" ")[2].replace("(","").replace(")","")

        return name,num
    def getDict(self):
        return self.marcoDict
    def addViNumMarcoDict(self):
        phy_pipe_num = self.marcoDict["VI_MAX_PHY_PIPE_NUM"]
        vir_pipe_num = self.marcoDict["VI_MAX_VIR_PIPE_NUM"]
        num = phy_pipe_num + vir_pipe_num
        self.marcoDict.update({"VI_MAX_PIPE_NUM":num})
        if is_number(self.marcoDict["AWB_SENSOR_NUM"]) == False:
            self.marcoDict.update({"AWB_SENSOR_NUM":num})

class EnumJson():
    ID = ""
    ALIAS = ""
    COMMENT = ""
    MEMBER = []
    class EnumMemberJson():
        ID = ""
        VALUE = ""
        def __init__(self, str):
            self.parseID(str)
            self.parseValue(str)
            if ENABLE_ENUMJSON_LOG:
                print("member ID:",self.ID,"member value:",self.VALUE)
        def parseID(self, str):
            pattern = re.compile("\w+", re.DOTALL)
            self.ID = pattern.findall(str)[0]
            #str = str.strip().replace(",", "").replace(" ", "").replace("=", "")
            #self.ID = ''.join([i for i in str if not i.isdigit()])
        def parseValue(self, str):
            pattern = re.compile('(?<=\=).*', re.DOTALL)
            if len(pattern.findall(str)) == 1:
                if "0x" in pattern.findall(str)[0]:
                    num = int(pattern.findall(str)[0],16)
                    self.VALUE = "%d"%num
                else:
                    self.VALUE = pattern.findall(str)[0].strip(" ")
    def __init__(self, str):
        self.MEMBER = []
        self.parseEnumInfo(str)
        if ENABLE_ENUMJSON_LOG:
            print("---------------------------------------------------------------------------------------------------")
            print("enum define:\n",str)
            print("enum ID:",self.ID,"enum ALIAS:",self.ALIAS)
        self.parseEnumMember(str)
        if ENABLE_ENUMJSON_LOG:
            print("---------------------------------------------------------------------------------------------------")
    def parseEnumInfo(self, str):
        pattern = re.compile('(?<=\}).+(?=\;)', re.DOTALL)
        if len(pattern.findall(str)) == 1:
            self.ID = pattern.findall(str)[0].strip()
            self.ALIAS = self.ID
            self.COMMENT = ""
        else:
            #enum xxx {
            # }; format
            pattern = re.compile('\w+\s*(?=\{)', re.DOTALL)
            if len(pattern.findall(str)) == 1:
                self.ID = pattern.findall(str)[0].strip()
                self.ALIAS = self.ID
                self.COMMENT = "Defines " + self.ID
            else:
                print(str + "is err")
    def parseEnumMember(self, str):
        pattern1 = re.compile('(?<=\{).*(?=\})', re.DOTALL)
        str = pattern1.findall(str)
        if len(str) == 1:
            pattern2 = re.compile('[a-zA-Z0-9_= ]+_[a-zA-Z0-9_= ]+', re.DOTALL)
            str = pattern2.findall(str[0])
            if "BUTT" in str[-1] or "MAX" in str[-1] or "NUM" in str[-1] or "SIZE" in str[-1]:
                if len(str) != 1:
                    del(str[-1])
            for enumStr in str:
                self.MEMBER.append(self.EnumMemberJson(enumStr))
    def format(self):
        template = """
                {
                    "ID": "{{ID}}",
                    "ALIAS": "{{ALIAS}}",
                    "COMMENT": "{{COMMENT}}",
                    "ENUM_ITEM": [
{%- for member in MEMBER %}
                        {
{%- if member.VALUE|length == 0 %}
                            "ID": "{{member.ID}}"
{%- else %}
                            "ID": "{{member.ID}}",
                            "VALUE": {{member.VALUE}}
{%- endif %}
{%- if loop.index == MEMBER|length %}
                        }
{%- else %}
                        },
{%- endif %}
{%- endfor %}
                    ]
                }
"""
        if ENABLE_PRINT_ENUMJSON:
            print(Template(template).render(ID=self.ID, ALIAS=self.ALIAS, COMMENT=self.COMMENT, MEMBER=self.MEMBER))
        return Template(template).render(ID=self.ID, ALIAS=self.ALIAS, COMMENT=self.COMMENT, MEMBER=self.MEMBER)
class StructJson():
    STATUS = False
    ID = ""
    ALIAS = ""
    COMMENT = ""
    MEMBER = []
    manualOrAutoSt = ""
    class StructMemberJson():
        ID = ""
        ALIAS = ""
        COMMENT = ""
        TYPE = ""
        ACCESS = ""
        TITLE_H = []
        COUNT = []
        RANGE = []
        FORMAT = ""
        LEVEL = 0

        def __init__(self, str, manualOrAutoSt, structId, marcoDict):
            self.structId = structId
            self.marcoDict = marcoDict
            self.RANGE = []
            self.COUNT = []
            self.TITLE_H = []
            self.TITLE_V = []
            if ENABLE_STRUCTJSON_LOG:
                print("---------------------------------------------------------------------------------------------------")
            self.parseParam(str, manualOrAutoSt)
            if ENABLE_STRUCTJSON_LOG:
                print("id:",self.ID,"alias:",self.ALIAS,"type:",self.TYPE,"ACCESS:",self.ACCESS)
                print("title_H:",self.TITLE_H,"title_V:",self.TITLE_V,"count:",self.COUNT,"range:",self.RANGE,"level:",self.LEVEL)
            if ENABLE_STRUCTJSON_LOG:
                print("---------------------------------------------------------------------------------------------------")
        def id2alias(self, str):
            pattern = []
            pattern.append(re.compile(r"(?<=^b|^f)[A-Z].*"))
            pattern.append(re.compile(r"(?<=^en|^st)[A-Z].*"))
            pattern.append(re.compile(r"(?<=^u8|^s8).*"))
            pattern.append(re.compile(r"(?<=^u16|^u32|^u64|^s16|^s32|^s64|^au8|^as8|^f32|^ast).*"))
            pattern.append(re.compile(r"(?<=^au16|^au32|^au64|^as16|^as32|^as64).*"))
            if self.structId in cusaliasDict.keys() and str in cusaliasDict[self.structId].keys():
                if ENABLE_STRUCTJSON_LOG:
                    print("id->alias:",str,"->",cusaliasDict[self.structId][str])
                return cusaliasDict[self.structId][str]
            else:
                for item in pattern:
                    if len(item.findall(str)) == 1:
                        Uppercase = item.findall(str)[0][:1].upper() + item.findall(str)[0][1:]
                        if ENABLE_STRUCTJSON_LOG:
                            print("id->alias:",str,"->",Uppercase)
                        return Uppercase
                return str[:1].upper() + str[1:]
        def parseParam(self, str, manualOrAutoSt):
            if len(str) != 0 and ";" in str:
                pattern = re.compile("\w+\s+\**\s*\S+\S(?=\;)")
                statement = pattern.findall(str)
                if ENABLE_STRUCTJSON_LOG:
                    print("str: ", str)
                    print("statement: ", statement, " ", len(statement))
                if len(statement) == 1:
                    #Replace \t with " "
                    statement[0] = statement[0].replace('\t', ' ')
                    statementArray = statement[0].strip().replace(";", "").split(" ", 100)
                    self.parseID_COUNT_TITLEH(statementArray, manualOrAutoSt)
                    self.parseType(statementArray)
                else:
                    print(str, statement, "statement count = {}".format(len(statement)))
            pattern = re.compile("(?<=\/\*).+(?=\*\/)")
            annotations = pattern.findall(str)
            if ENABLE_STRUCTJSON_LOG:
                print("annotations: ", annotations, "len:", len(annotations))
            if len(annotations) == 1:
                pattern = re.compile("\w+(?=\;)", re.DOTALL)
                access = pattern.findall(annotations[0])
                if len(access) == 1:
                    self.parseAccess(access[0])
                pattern = re.compile("(?<=\[).*(?=\])")
                range = pattern.findall(annotations[0])
                if len(range) == 1:
                    range = range[0].replace(" ", "").split(",", 100)
                    self.parseRange(range)
                else:
                    if typeDict.get(self.TYPE) != None:
                        if ENABLE_STRUCTJSON_LOG:
                            print(typeDict[self.TYPE])
                        self.parseRange(typeDict[self.TYPE])
                    else:
                        self.parseRange("")
                #pattern = re.compile("(?<=Level\:)\d+")
                #level = pattern.findall(annotations[0])
                self.parseLevel()
            else:
                self.parseAccess("RW")
                if typeDict.get(self.TYPE) != None:
                    if ENABLE_STRUCTJSON_LOG:
                        print(typeDict[self.TYPE])
                    self.parseRange(typeDict[self.TYPE])
                else:
                    self.parseRange("")
                self.parseLevel()
        def parseType(self, statementArray):
            if self.structId in cusTypeDict.keys() and self.ID in cusTypeDict[self.structId].keys():
                self.TYPE = cusTypeDict[self.structId][self.ID][0]
            else:
                self.TYPE = statementArray[0]

        def parseID_COUNT_TITLEH(self, statementArray, manualOrAutoSt):
            pattern = re.compile('\w*\w')
            id = pattern.findall(statementArray[1])
            if len(id) >= 1:
                self.ID = id[0]
                self.ALIAS = manualOrAutoSt + self.id2alias(self.ID)
            if len(id) > 1:
                for i in range(1, len(id)):
                    if is_number(id[i]):
                        if self.structId in cusTitleCountDict.keys() and self.ID in cusTitleCountDict[self.structId].keys():
                            if isinstance(cusTitleCountDict[self.structId][self.ID][0], list):
                                self.TITLE_H = cusTitleCountDict[self.structId][self.ID][0]
                            if isinstance(cusTitleCountDict[self.structId][self.ID][1], list):
                                self.TITLE_V = cusTitleCountDict[self.structId][self.ID][1]
                            if isinstance(cusTitleCountDict[self.structId][self.ID][2], list):
                                self.COUNT = cusTitleCountDict[self.structId][self.ID][2]
                            elif isinstance(cusTitleCountDict[self.structId][self.ID][2], int) and cusTitleCountDict[self.structId][self.ID][2]!=0:
                                self.COUNT = cusTitleCountDict[self.structId][self.ID][2]
                        else:
                            self.COUNT.append(id[i])
                    else:
                        if defineDict.get(id[i]):
                            self.COUNT.append(defineDict[id[i]][0])
                            self.TITLE_H = defineDict[id[i]][1]
                        elif self.marcoDict.get(id[i]):
                            if self.structId in cusTitleCountDict.keys() and self.ID in cusTitleCountDict[self.structId].keys():
                                if isinstance(cusTitleCountDict[self.structId][self.ID][0], list):
                                    self.TITLE_H = cusTitleCountDict[self.structId][self.ID][0]
                                if isinstance(cusTitleCountDict[self.structId][self.ID][1], list):
                                    self.TITLE_V = cusTitleCountDict[self.structId][self.ID][1]
                                if isinstance(cusTitleCountDict[self.structId][self.ID][2], list):
                                    self.COUNT = cusTitleCountDict[self.structId][self.ID][2]
                                elif isinstance(cusTitleCountDict[self.structId][self.ID][2], int) and cusTitleCountDict[self.structId][self.ID][2]!=0:
                                    self.COUNT = cusTitleCountDict[self.structId][self.ID][2]
                            else:
                                self.COUNT.append(self.marcoDict[id[i]])
                        else:
                            print("count err->", id[i])
                            self.COUNT.append(id[i])
        def parseAccess(self, access):
            if self.structId in cusAccess.keys() and self.ID in cusAccess[self.structId].keys():
                self.ACCESS = cusAccess[self.structId][self.ID]
            else:
                self.ACCESS = access
        def parseRange(self, Range):
            if self.structId in cusRangeDict.keys() and self.ID in cusRangeDict[self.structId].keys():
                for i in range(0, len(cusRangeDict[self.structId][self.ID])):
                    self.RANGE.append(cusRangeDict[self.structId][self.ID][i])
            else:
                if isinstance(Range, list):
                    if self.marcoDict.get(Range[0]):
                        self.RANGE.append(int(self.marcoDict[Range[0]], 16))
                    else:
                        self.RANGE.append(int(Range[0], 16))
                    if self.marcoDict.get(Range[1]):
                        self.RANGE.append(self.marcoDict[Range[1]])
                    else:
                        self.RANGE.append(int(Range[1], 16))
        def parseLevel(self):
            if USE_LEVEL:
                if self.structId in levelDict.keys():
                    if self.ID in levelDict[self.structId].keys():
                        self.LEVEL = levelDict[self.structId][self.ID]
                    else:
                        self.LEVEL = 0
                else:
                    self.LEVEL = 0
            else:
                self.LEVEL = 0
    def __init__(self, str, marcoDict):
        self.MEMBER = []
        self.parseStructInfo(str)
        self.marcoDict = marcoDict
        if ENABLE_STRUCTJSON_LOG:
            print("---------------------------------------------------------------------------------------------------")
            print(str)
            print("struct ID:", self.ID, "struct ALIAS:", self.ALIAS, "struct COMMENT:", self.COMMENT)
        self.parseStructMember(str)
        if ENABLE_STRUCTJSON_LOG:
            print("---------------------------------------------------------------------------------------------------")
    def parseStructInfo(self, str):
        pattern = re.compile('(?<=\}).+(?=\;)', re.DOTALL)
        if len(pattern.findall(str)) == 1:
            self.ID = pattern.findall(str)[0].strip()
            if self.ID in cusaliasDict.keys():
                if self.ID in cusaliasDict[self.ID].keys():
                    self.ALIAS = cusaliasDict[self.ID][self.ID]
                else:
                    self.ALIAS = self.ID
            else:
                self.ALIAS = self.ID
            self.COMMENT = "Defines " + self.ID
            #print(ID)
            self.STATUS = True
            # if "AUTO" in self.ID:
            #     self.manualOrAutoSt = "Auto."
            # if "MANUAL" in self.ID:
            #     self.manualOrAutoSt = "Manual."
        else :
            #struct xxx {
            # }; format
            pattern = re.compile('\w+\s*(?=\{)', re.DOTALL)
            if len(pattern.findall(str)) == 1:
                self.ID = pattern.findall(str)[0].strip()
                self.ALIAS = self.ID
                self.COMMENT = "Defines " + self.ID
                self.STATUS = True
            else:
                print(str + "is err")
    def parseStructMember(self, str):
        memberStrs = str.split("\n", 100)
        del(memberStrs[0])
        del(memberStrs[-1])
        if ENABLE_STRUCTJSON_LOG:
            print(memberStrs)
        for member in memberStrs:
            structMember = self.StructMemberJson(member, self.manualOrAutoSt, self.ID, self.marcoDict)
            if structMember.ID != "":
                self.MEMBER.append(structMember)
    def format(self):
        template = """
                {
                    "ID": "{{ID}}",
                    "ALIAS": "{{ALIAS}}",
                    "COMMENT": "{{COMMENT}}",
                    "MEMBER": [
{%- for member in MEMBER %}
                        {
                            "ID": "{{member.ID}}",
                            "ALIAS": "{{member.ALIAS}}",
                            "COMMENT": "{{member.COMMENT}}",
                            "TYPE": "{{member.TYPE}}",
                            "ACCESS": "{{member.ACCESS}}",
                            "LEVEL": {{member.LEVEL}},
{%- if member.TITLE_H|length != 0 %}
                            "TITLE_H": [
{%- for title in member.TITLE_H %}
{%- if loop.index == member.TITLE_H|length %}
                                "{{title}}"
{%- else %}
                                "{{title}}",
{%- endif %}
{%- endfor %}
                            ],
{%- endif %}
{%- if member.TITLE_V|length != 0 %}
                            "TITLE_V": [
{%- for title in member.TITLE_V %}
{%- if loop.index == member.TITLE_V|length %}
                                "{{title}}"
{%- else %}
                                "{{title}}",
{%- endif %}
{%- endfor %}
                            ],
{%- endif %}
{%- if member.COUNT|length > 1 %}
                            "COUNT": [
{%- for count in member.COUNT %}
{%- if loop.index == member.COUNT|length %}
                                {{count}}
{%- else %}
                                {{count}},
{%- endif %}
{%- endfor %}
                            ],
{%- elif member.COUNT|length == 1 %}
                            "COUNT": {{member.COUNT[0]}},
{%- endif %}
{%- if member.RANGE|length != 0 %}
                            "RANGE": [
{%- for range in member.RANGE %}
{%- if loop.index == member.RANGE|length %}
                                {{range}}
{%- else %}
                                {{range}},
{%- endif %}
{%- endfor %}
                            ],
{%- endif %}
                            "FORMAT": ""
{%- if loop.index == MEMBER|length %}
                        }
{%- else %}
                        },
{%- endif %}
{%- endfor %}
                    ]
                }
"""

        if ENABLE_PRINT_STRUCTJSON:
            print(Template(template).render(ID= self.ID, ALIAS= self.ALIAS, COMMENT= self.COMMENT, MEMBER= self.MEMBER))
        return Template(template).render(ID= self.ID, ALIAS= self.ALIAS, COMMENT= self.COMMENT, MEMBER= self.MEMBER)
def parseContent(str):
    # pattern = re.compile('\n\/\/\-TJS\-.*\/\/\-TJE\-', re.DOTALL)
    # return pattern.findall(str)
    return str
def parseMarco(str):
    pattern = re.compile("#define\s+\w+\s+\w+")
    list1 = pattern.findall(str)
    pattern = re.compile("#define\s+\w+\s+\(\w+\)")
    list2 = pattern.findall(str)
    pattern = re.compile("#define\s+\w+\s+\(\w+\s*\*\s*\w\)")
    list3 = pattern.findall(str)
    return list1 + list2 + list3
def parseStruct(str):
    pattern = re.compile('struct\s*\w+\s*\{[^\}\{\#]*\}[^\}\{\#\/\*]*\;', re.DOTALL)
    return pattern.findall(str)
def parseEnum(str):
    pattern = re.compile('enum[^\{\#]*\{[^\}\#]*\}[^\}\{\#\/\*]*\;', re.DOTALL)
    return pattern.findall(str)
def SaveEnumJson2File(fp, enumStrs):
    startLine = """
        "ENUM_DEF": {
            "ENUM": ["""
    endLine = """
            ]
        },"""
    fp.write(startLine)
    enumJsonContextList = []
    for idx in range(len(enumStrs)):
        enumJsonContext = EnumJson(enumStrs[idx]).format()
        if enumJsonContext != "":
            enumJsonContextList.append(enumJsonContext)
    for idx in range(len(enumJsonContextList)):
        if idx == len(enumJsonContextList) - 1:
            fp.write(enumJsonContextList[idx])
        else:
            fp.write(enumJsonContextList[idx] + ",")
    fp.write(endLine)
def SaveStructJson2File(fp, marcoStrs, structStrs):
    marcoDict = MarcoDict(marcoStrs).getDict()
    startLine = """
        "STRUCT_DEF": {
            "STRUCT": ["""
    endLine = """
            ]
        }"""
    fp.write(startLine)
    structJsonContextList = []
    for idx in range(len(structStrs)):
        structJsonContext = StructJson(structStrs[idx], marcoDict).format()
        if structJsonContext != "":
            structJsonContextList.append(structJsonContext)
    for idx in range(len(structJsonContextList)):
        if idx == len(structJsonContextList) - 1:
            fp.write(structJsonContextList[idx])
        else:
            fp.write(structJsonContextList[idx] + ",")
    fp.write(endLine)
def SaveDeviceJson2File(fp):
    f = open('device.json', 'r')
    fileStr = f.read()
    fp.write(fileStr+"\n")
    f.close()
def SaveDefinitionJson2File(fp,marcoStrs,enumStrs,structStrs):
    startLine = """\
    "DEFINITIONS": {"""
    endLine = """
    },
"""
    fp.write(startLine)
    SaveEnumJson2File(fp, enumStrs)
    SaveStructJson2File(fp, marcoStrs, structStrs)
    fp.write(endLine)
def SaveLayoutJson2File(fileName, fp):
    f = open(fileName, 'r')
    fileStr = f.readlines()
    for index, value in enumerate(fileStr):
        if index !=0 and index != len(fileStr)-1:
            fp.write(value)
    f.close()
def SaveRpcJson2File(fileName, fp):
    f = open(fileName, 'r')
    fileStr = f.readlines()
    for index, value in enumerate(fileStr):
        if index !=0 and index != len(fileStr)-1 and index != len(fileStr)-2:
            if index == 3:
                fp.write(rpcHardCode.strip("\n")+"\n")
            fp.write(value)
        elif index == len(fileStr)-2:
            fp.write(value.strip("\n")+","+"\n")
    f.close()
def SaveJson2File(marcoStrs, enumStrs, structStrs, fp, layoutFileName, rpcFileName):
    startLine = """{\n"""
    endLine = """}\n"""
    fp.write(startLine)
    SaveDeviceJson2File(fp)
    SaveDefinitionJson2File(fp, marcoStrs, enumStrs, structStrs)
    SaveRpcJson2File(rpcFileName, fp)
    SaveLayoutJson2File(layoutFileName, fp)
    fp.write(endLine)

def readLevelDictFromFile(levelJsonFile):
    global levelDict
    jsonFIle = open(levelJsonFile, "r")
    tmpDict = json.load(jsonFIle)
    for key in tmpDict.keys():
        if "@" in  key:
            levelDict.update({key.strip("@").replace("ATTR", "MANUAL_ATTR"):tmpDict[key]})
            levelDict.update({key.strip("@").replace("ATTR", "AUTO_ATTR"):tmpDict[key]})
        else:
            levelDict.update({key:tmpDict[key]})
    paramNum = 0
    for key in tmpDict.keys():
        for member in tmpDict[key]:
            paramNum = paramNum + 1


if __name__ == '__main__':
    content = ""
    levelFileName = ""
    rpcFileName = ""
    layoutFileName = ""
    for idx in range(len(sys.argv)):
        if idx == 0:
            continue
        if idx == 1:
            levelFileName = sys.argv[idx]
        if idx == 2:
            layoutFileName = sys.argv[idx]
        if idx == 3:
            rpcFileName = sys.argv[idx]
        print(sys.argv[idx])
        f = open(sys.argv[idx], 'r')
        content += parseContent(f.read())
        f.close
    readLevelDictFromFile(levelFileName)
    marcoStrs = parseMarco(content)
    structStrs = parseStruct(hardCodeContent + content)
    enumStrs = parseEnum(content)
    try:
        jsonFile = open('pqtool_definition.json', 'w')
        SaveJson2File(marcoStrs, enumStrs, structStrs, jsonFile, layoutFileName, rpcFileName)
        jsonFile.flush()
    finally:
        if jsonFile:
            jsonFile.close()
    print("generate pqtool_definition.json success")
