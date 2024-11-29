#ifndef _AWB_PLATFORM_H_
#define _AWB_PLATFORM_H_

#define MB_OK (0)

struct AWB_JUDGE_ST {
	CVI_U16 u16IdealR;
	CVI_U16 u16IdealB;
	CVI_FLOAT fIdealRThresh;
	CVI_FLOAT fIdealBThresh;
};

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



void MessageBox(int v1, char *str1, char *str2, int v2);

void BMP_ImgInit(void);
void BMP_ImgSave(char *fname);

void BMP_line(int x0, int y0, int x1, int y1, int r, int g, int b);
void BMP_dashline(int x0, int y0, int x1, int y1, int r, int g, int b);
void BMP_circle(unsigned int x0, unsigned int y0, unsigned int radius, int r, int g, int b);
void BMP_circle_fill(unsigned int x0, unsigned int y0, unsigned int radius, int r, int g, int b);
void BMP_Setpixel(int x, int y, int col_r, int col_g, int col_b);

void BMP_DrawDbginfo(s_AWB_DBG_S *pTmp, char *fname);
void SaveAwbInfo(void);

#endif
