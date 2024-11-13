#ifndef _EXT_MISC_H_
#define _EXT_MISC_H_

#define NORMAL_COLOR 7
#define GREEN_COLOR 10
#define BLUE_COLOR 11
#define RED_COLOR 12
#define PURPLE_COLOR 13
#define YELLOW_COLOR 14

int AWB_DLL_Main(struct AWB_JUDGE_ST *pstJudge, char *wbinName, char *jsonName, char *outputPath);
s_AWB_DBG_S *pc_AWB_DbgBinInit(CVI_U8 sID);
void pc_AWB_RunAlgo(CVI_U8 sID);
void pc_AWB_SaveLog(CVI_U8 sID);
void pc_SetAWB_Dump2File(CVI_BOOL val);
sWBCurveInfo *pc_Get_stCurveInfo(CVI_U8 sID);
void pc_Set_u8AwbDbgBinFlag(CVI_U8 val);
sWBInfo *pc_Get_sWBInfo(CVI_U8 sID);
sWBCurveInfo *pc_Get_sWBCurveInfo(CVI_U8 sID);
ISP_AWB_Calibration_Gain_S *pc_Get_stWbCalibration(CVI_U8 sID);
ISP_WB_ATTR_S *pc_Get_stAwbMpiAttr(CVI_U8 sID);
ISP_WB_ATTR_S *pc_Get_stAwbAttrInfo(CVI_U8 sID);
ISP_AWB_ATTR_EX_S *pc_Get_stAwbMpiAttrEx(CVI_U8 sID);
ISP_AWB_ATTR_EX_S *pc_Get_stAwbAttrInfoEx(CVI_U8 sID);
SEXTRA_LIGHT_RB *pc_Get_stEXTRA_LIGHT_RB(void);
int getAttrFromJson(char *filename, int flag);


void ColorPrintf(int color, const char *szFmt, ...);
void pcprintf(const char *szFmt, ...);
#endif /* endof #ifndef _EXT_MISC_H_ */
