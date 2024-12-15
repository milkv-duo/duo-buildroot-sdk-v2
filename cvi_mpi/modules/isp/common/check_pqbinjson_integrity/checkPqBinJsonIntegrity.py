#coding:utf-8
import sys
import re

"""
For example:
typedef struct _ISP_MI_ATTR_S {
	CVI_U32	u32HoldValue; /*RW; Range:[0x0, 0x3E8]*/
	ISP_IRIS_F_NO_E	enIrisFNO; /*RW; Range:[0x0, 0xA]*/
} ISP_MI_ATTR_S;
struct ST_ISP_AWB_SKIN_S {
	CVI_U8 u8Mode;
	CVI_U16 u16RgainDiff;
	CVI_U16 u16BgainDiff;
	CVI_U8 u8Radius;
};
"""
findStDef1FromH = re.compile('typedef struct[^\{]{1,}\{[^\}]+[^ ]\}[^\{\}]+\;', re.DOTALL)
findStDef2FromH = re.compile('struct[^\{]{2,}\{[^\}]+[^ ]\}[^\{\}]*\;', re.DOTALL)
findStName1FromH = re.compile('(?<=\}).{2,}(?=;)', re.DOTALL)
findStName2FromH = re.compile('(?<=struct).+(?=\{)', re.DOTALL)
findStMemberStrFromH = re.compile('(?<=\{).+(?=\})', re.DOTALL)
findStMemberFromH = re.compile('\w+\s+[\w\[\]]+(?=;)', re.DOTALL)
findStMemberNameFromH = re.compile('(?<=[\t ])\w+', re.DOTALL)
"""
For example:
void ISP_AF_V_PARAM_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AF_V_PARAM_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_S8, s8VFltHpCoeff, FIR_V_GAIN_NUM);

	JSON_END(r_w_flag);
}
"""
findApiDefFromC = re.compile('void \w*_S_JSON\([^\{]*\{[^\}]*\}[^\{]', re.DOTALL)
findStNameFromC = re.compile('\w+_S(?=_JSON)', re.DOTALL)
findStMemberStrFromC = re.compile('(?<=JSON)[^\{\};]+(?=\);)', re.DOTALL)

def parseStructDefine(str):
    return findStDef1FromH.findall(str) + findStDef2FromH.findall(str)
def parseApiDefine(str):
    return findApiDefFromC.findall(str)
def parseStruct_MemberNum_FromH(HFileApiList):
    dict = {}
    for i in range(len(HFileApiList)):
        structName = findStName1FromH.findall(HFileApiList[i])
        if (len(structName) == 0):
            structName = findStName2FromH.findall(HFileApiList[i])
        memberStr = findStMemberStrFromH.findall(HFileApiList[i])
        if len(memberStr) != 0:
            memberName = []
            member = findStMemberFromH.findall(memberStr[0])
            for i in range(len(member)):
                memberNameList = findStMemberNameFromH.findall(member[i])
                if (len(memberNameList) != 0):
                    memberName.append(memberNameList[0])
        else:
            pass
            #print("err", HFileApiList[i],structName[0],memberStr)
        if (len(structName) != 0) and (len(member) != 0) :
            tmp = {structName[0].strip(" "): [len(memberName), memberName]}
            dict.update(tmp)
        else:
            pass
            #print("err", HFileApiList[i],structName[0],len(member))
    return dict

def parseStruct_MemberNum_FromC(CFileApiList):
    dict = {}
    for i in range(len(CFileApiList)):
        structName = findStNameFromC.findall(CFileApiList[i])
        memberStr = findStMemberStrFromC.findall(CFileApiList[i])
        memberName = []
        if (len(structName) != 0) and (len(memberStr) != 0) :
            for i in range(len(memberStr)):
                if len(memberStr[i].split(",")) >= 3:
                    memberName.append(memberStr[i].split(",")[2].strip(" "))
            tmp = {structName[0]:[len(memberName),memberName]}
            dict.update(tmp)
        else:
            print(CFileApiList[i], structName[0], len(memberName))
    return dict

if __name__ == '__main__':
    CFileApiList = []
    HFileStructList = []
    cStructDict = {}
    hStructDict = {}
    print("check json&struct convert start")
    for idx in range(len(sys.argv)):
        if idx == 0:
            continue
        if idx == 1:
            f = open(sys.argv[1], 'r')
            CFileApiList = parseApiDefine(f.read())
            cStructDict = parseStruct_MemberNum_FromC(CFileApiList)
            f.close
        else:
            f = open(sys.argv[idx], 'r')
            HFileStructList += parseStructDefine(f.read())
            hStructDict = parseStruct_MemberNum_FromH(HFileStructList)
            f.close
    if (len(hStructDict) - len(cStructDict)) == len(set(hStructDict) - set(cStructDict)):
        exclusiveSet = set(hStructDict) - set(cStructDict)
        for i in exclusiveSet:
            hStructDict.pop(i)
        if len(hStructDict) == len(cStructDict):
            print("check {0} struct".format(len(cStructDict)))
            for k in cStructDict.keys():
                if k in hStructDict.keys():
                    if cStructDict[k][0] != hStructDict[k][0]:
                        print("{0}->[json:{1},struct:{2}]".format(k, cStructDict[k][0], hStructDict[k][0]))
                        cDiff = set(cStructDict[k][1]) - (set(hStructDict[k][1]) & set(cStructDict[k][1]))
                        hDiff = set(hStructDict[k][1]) - (set(hStructDict[k][1]) & set(cStructDict[k][1]))
                        if len(cDiff) != 0:
                            print("you need check {0} from isp_json_struct.c".format(cDiff))
                        if len(hDiff) != 0:
                            print("you need update {0} to isp_json_struct.c".format(hDiff))
                else:
                    print("cStructDict and hStructDict keys not match, please check")
    else:
        print("cStructDict not a subset of hStructDict, please check h file parser!!")
    print("check json&struct convert end")