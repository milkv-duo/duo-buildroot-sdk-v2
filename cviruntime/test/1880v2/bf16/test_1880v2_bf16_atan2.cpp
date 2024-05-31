/**
 */
#include "../1880v2_test_util.h"
#define OUT
#define IN
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cfloat>
//#include <boost/math/special_functions/next.hpp>
//#define DBG
//#define LOCAL_MEM_SIZE (32*1024) //<! 1880v2, 32k

using namespace std;

//<! 1880v2 hw config
// TODO: get from ctx
static u32 channel = 32; //<! 1880v2 hardcode
static u32 table_h = 32;
static u32 table_w = 8;
static u32 table_hw = table_h * table_w;
static double *lut = (double *)malloc(sizeof(double) * table_hw);

/**
 * pre_data means we test fixed pattern, it should be same sa lut
 */
enum TEST_MODE {
  PRE_DATA_COMPARE_FIX = 0, // pre-data + fix compare
  DATA_COMPARE, //generate 2^-20 ~ 2^20 value that check epsilon
  DATA_COMPARE_U8, //generate 2^-20 ~ 2^20 value that check epsilon, result bf16->u8
  TEST_MODE_MAX,
};

static TEST_MODE mode;

static u16 test_pattern[] = {
  0x0000,
  0x38D2,
  0x3952,
  0x399D,
  0x39D2,
  0x3A03,
  0x3A1D,
  0x3A38,
  0x3A52,
  0x3A6C,
  0x3A83,
  0x3A90,
  0x3A9D,
  0x3AAA,
  0x3AB8,
  0x3AC5,
  0x3AD2,
  0x3ADF,
  0x3AEC,
  0x3AF9,
  0x3B03,
  0x3B0A,
  0x3B10,
  0x3B17,
  0x3B1D,
  0x3B24,
  0x3B2A,
  0x3B31,
  0x3B38,
  0x3B3E,
  0x3B45,
  0x3B4B,
  0x3B52,
  0x3B58,
  0x3B5F,
  0x3B65,
  0x3B6C,
  0x3B72,
  0x3B79,
  0x3B80,
  0x3B83,
  0x3B86,
  0x3B8A,
  0x3B8D,
  0x3B90,
  0x3B93,
  0x3B97,
  0x3B9A,
  0x3B9D,
  0x3BA1,
  0x3BA4,
  0x3BA7,
  0x3BAA,
  0x3BAE,
  0x3BB1,
  0x3BB4,
  0x3BB8,
  0x3BBB,
  0x3BBE,
  0x3BC1,
  0x3BC5,
  0x3BC8,
  0x3BCB,
  0x3BCE,
  0x3BD2,
  0x3BD5,
  0x3BD8,
  0x3BDC,
  0x3BDF,
  0x3BE2,
  0x3BE5,
  0x3BE9,
  0x3BEC,
  0x3BEF,
  0x3BF2,
  0x3BF6,
  0x3BF9,
  0x3BFC,
  0x3C00,
  0x3C01,
  0x3C03,
  0x3C05,
  0x3C06,
  0x3C08,
  0x3C0A,
  0x3C0B,
  0x3C0D,
  0x3C0F,
  0x3C10,
  0x3C12,
  0x3C13,
  0x3C15,
  0x3C17,
  0x3C18,
  0x3C1A,
  0x3C1C,
  0x3C1D,
  0x3C1F,
  0x3C21,
  0x3C22,
  0x3C24,
  0x3C25,
  0x3C27,
  0x3C29,
  0x3C2A,
  0x3C2C,
  0x3C2E,
  0x3C2F,
  0x3C31,
  0x3C33,
  0x3C34,
  0x3C36,
  0x3C38,
  0x3C39,
  0x3C3B,
  0x3C3C,
  0x3C3E,
  0x3C40,
  0x3C41,
  0x3C43,
  0x3C45,
  0x3C46,
  0x3C48,
  0x3C4A,
  0x3C4B,
  0x3C4D,
  0x3C4E,
  0x3C50,
  0x3C52,
  0x3C53,
  0x3C55,
  0x3C57,
  0x3C58,
  0x3C5A,
  0x3C5C,
  0x3C5D,
  0x3C5F,
  0x3C60,
  0x3C62,
  0x3C64,
  0x3C65,
  0x3C67,
  0x3C69,
  0x3C6A,
  0x3C6C,
  0x3C6E,
  0x3C6F,
  0x3C71,
  0x3C72,
  0x3C74,
  0x3C76,
  0x3C77,
  0x3C79,
  0x3C7B,
  0x3C7C,
  0x3C7E,
  0x3C80,
  0x3C81,
  0x3C81,
  0x3C82,
  0x3C83,
  0x3C84,
  0x3C85,
  0x3C86,
  0x3C86,
  0x3C87,
  0x3C88,
  0x3C89,
  0x3C8A,
  0x3C8A,
  0x3C8B,
  0x3C8C,
  0x3C8D,
  0x3C8E,
  0x3C8F,
  0x3C8F,
  0x3C90,
  0x3C91,
  0x3C92,
  0x3C93,
  0x3C93,
  0x3C94,
  0x3C95,
  0x3C96,
  0x3C97,
  0x3C98,
  0x3C98,
  0x3C99,
  0x3C9A,
  0x3C9B,
  0x3C9C,
  0x3C9C,
  0x3C9D,
  0x3C9E,
  0x3C9F,
  0x3CA0,
  0x3CA1,
  0x3CA1,
  0x3CA2,
  0x3CA3,
  0x3CA4,
  0x3CA5,
  0x3CA5,
  0x3CA6,
  0x3CA7,
  0x3CA8,
  0x3CA9,
  0x3CAA,
  0x3CAA,
  0x3CAB,
  0x3CAC,
  0x3CAD,
  0x3CAE,
  0x3CAE,
  0x3CAF,
  0x3CB0,
  0x3CB1,
  0x3CB2,
  0x3CB3,
  0x3CB3,
  0x3CB4,
  0x3CB5,
  0x3CB6,
  0x3CB7,
  0x3CB8,
  0x3CB8,
  0x3CB9,
  0x3CBA,
  0x3CBB,
  0x3CBC,
  0x3CBC,
  0x3CBD,
  0x3CBE,
  0x3CBF,
  0x3CC0,
  0x3CC1,
  0x3CC1,
  0x3CC2,
  0x3CC3,
  0x3CC4,
  0x3CC5,
  0x3CC5,
  0x3CC6,
  0x3CC7,
  0x3CC8,
  0x3CC9,
  0x3CCA,
  0x3CCA,
  0x3CCB,
  0x3CCC,
  0x3CCD,
  0x3CCE,
  0x3CCE,
  0x3CCF,
  0x3CD0,
  0x3CD1,
  0x3CD2,
  0x3CD3,
  0x3CD3,
  0x3CD4,
  0x3CD5,
  0x3CD6,
  0x3CD7,
  0x3CD7,
  0x3CD8,
  0x3CD9,
  0x3CDA,
  0x3CDB,
  0x3CDC,
  0x3CDC,
  0x3CDD,
  0x3CDE,
  0x3CDF,
  0x3CE0,
  0x3CE0,
  0x3CE1,
  0x3CE2,
  0x3CE3,
  0x3CE4,
  0x3CE5,
  0x3CE5,
  0x3CE6,
  0x3CE7,
  0x3CE8,
  0x3CE9,
  0x3CE9,
  0x3CEA,
  0x3CEB,
  0x3CEC,
  0x3CED,
  0x3CEE,
  0x3CEE,
  0x3CEF,
  0x3CF0,
  0x3CF1,
  0x3CF2,
  0x3CF2,
  0x3CF3,
  0x3CF4,
  0x3CF5,
  0x3CF6,
  0x3CF7,
  0x3CF7,
  0x3CF8,
  0x3CF9,
  0x3CFA,
  0x3CFB,
  0x3CFB,
  0x3CFC,
  0x3CFD,
  0x3CFE,
  0x3CFF,
  0x3D00,
  0x3D00,
  0x3D01,
  0x3D01,
  0x3D01,
  0x3D02,
  0x3D02,
  0x3D03,
  0x3D03,
  0x3D03,
  0x3D04,
  0x3D04,
  0x3D05,
  0x3D05,
  0x3D06,
  0x3D06,
  0x3D06,
  0x3D07,
  0x3D07,
  0x3D08,
  0x3D08,
  0x3D08,
  0x3D09,
  0x3D09,
  0x3D0A,
  0x3D0A,
  0x3D0A,
  0x3D0B,
  0x3D0B,
  0x3D0C,
  0x3D0C,
  0x3D0C,
  0x3D0D,
  0x3D0D,
  0x3D0E,
  0x3D0E,
  0x3D0F,
  0x3D0F,
  0x3D0F,
  0x3D10,
  0x3D10,
  0x3D11,
  0x3D11,
  0x3D11,
  0x3D12,
  0x3D12,
  0x3D13,
  0x3D13,
  0x3D13,
  0x3D14,
  0x3D14,
  0x3D15,
  0x3D15,
  0x3D16,
  0x3D16,
  0x3D16,
  0x3D17,
  0x3D17,
  0x3D18,
  0x3D18,
  0x3D18,
  0x3D19,
  0x3D19,
  0x3D1A,
  0x3D1A,
  0x3D1A,
  0x3D1B,
  0x3D1B,
  0x3D1C,
  0x3D1C,
  0x3D1C,
  0x3D1D,
  0x3D1D,
  0x3D1E,
  0x3D1E,
  0x3D1F,
  0x3D1F,
  0x3D1F,
  0x3D20,
  0x3D20,
  0x3D21,
  0x3D21,
  0x3D21,
  0x3D22,
  0x3D22,
  0x3D23,
  0x3D23,
  0x3D23,
  0x3D24,
  0x3D24,
  0x3D25,
  0x3D25,
  0x3D25,
  0x3D26,
  0x3D26,
  0x3D27,
  0x3D27,
  0x3D28,
  0x3D28,
  0x3D28,
  0x3D29,
  0x3D29,
  0x3D2A,
  0x3D2A,
  0x3D2A,
  0x3D2B,
  0x3D2B,
  0x3D2C,
  0x3D2C,
  0x3D2C,
  0x3D2D,
  0x3D2D,
  0x3D2E,
  0x3D2E,
  0x3D2E,
  0x3D2F,
  0x3D2F,
  0x3D30,
  0x3D30,
  0x3D31,
  0x3D31,
  0x3D31,
  0x3D32,
  0x3D32,
  0x3D33,
  0x3D33,
  0x3D33,
  0x3D34,
  0x3D34,
  0x3D35,
  0x3D35,
  0x3D35,
  0x3D36,
  0x3D36,
  0x3D37,
  0x3D37,
  0x3D38,
  0x3D38,
  0x3D38,
  0x3D39,
  0x3D39,
  0x3D3A,
  0x3D3A,
  0x3D3A,
  0x3D3B,
  0x3D3B,
  0x3D3C,
  0x3D3C,
  0x3D3C,
  0x3D3D,
  0x3D3D,
  0x3D3E,
  0x3D3E,
  0x3D3E,
  0x3D3F,
  0x3D3F,
  0x3D40,
  0x3D40,
  0x3D41,
  0x3D41,
  0x3D41,
  0x3D42,
  0x3D42,
  0x3D43,
  0x3D43,
  0x3D43,
  0x3D44,
  0x3D44,
  0x3D45,
  0x3D45,
  0x3D45,
  0x3D46,
  0x3D46,
  0x3D47,
  0x3D47,
  0x3D47,
  0x3D48,
  0x3D48,
  0x3D49,
  0x3D49,
  0x3D4A,
  0x3D4A,
  0x3D4A,
  0x3D4B,
  0x3D4B,
  0x3D4C,
  0x3D4C,
  0x3D4C,
  0x3D4D,
  0x3D4D,
  0x3D4E,
  0x3D4E,
  0x3D4E,
  0x3D4F,
  0x3D4F,
  0x3D50,
  0x3D50,
  0x3D50,
  0x3D51,
  0x3D51,
  0x3D52,
  0x3D52,
  0x3D53,
  0x3D53,
  0x3D53,
  0x3D54,
  0x3D54,
  0x3D55,
  0x3D55,
  0x3D55,
  0x3D56,
  0x3D56,
  0x3D57,
  0x3D57,
  0x3D57,
  0x3D58,
  0x3D58,
  0x3D59,
  0x3D59,
  0x3D59,
  0x3D5A,
  0x3D5A,
  0x3D5B,
  0x3D5B,
  0x3D5C,
  0x3D5C,
  0x3D5C,
  0x3D5D,
  0x3D5D,
  0x3D5E,
  0x3D5E,
  0x3D5E,
  0x3D5F,
  0x3D5F,
  0x3D60,
  0x3D60,
  0x3D60,
  0x3D61,
  0x3D61,
  0x3D62,
  0x3D62,
  0x3D63,
  0x3D63,
  0x3D63,
  0x3D64,
  0x3D64,
  0x3D65,
  0x3D65,
  0x3D65,
  0x3D66,
  0x3D66,
  0x3D67,
  0x3D67,
  0x3D67,
  0x3D68,
  0x3D68,
  0x3D69,
  0x3D69,
  0x3D69,
  0x3D6A,
  0x3D6A,
  0x3D6B,
  0x3D6B,
  0x3D6C,
  0x3D6C,
  0x3D6C,
  0x3D6D,
  0x3D6D,
  0x3D6E,
  0x3D6E,
  0x3D6E,
  0x3D6F,
  0x3D6F,
  0x3D70,
  0x3D70,
  0x3D70,
  0x3D71,
  0x3D71,
  0x3D72,
  0x3D72,
  0x3D72,
  0x3D73,
  0x3D73,
  0x3D74,
  0x3D74,
  0x3D75,
  0x3D75,
  0x3D75,
  0x3D76,
  0x3D76,
  0x3D77,
  0x3D77,
  0x3D77,
  0x3D78,
  0x3D78,
  0x3D79,
  0x3D79,
  0x3D79,
  0x3D7A,
  0x3D7A,
  0x3D7B,
  0x3D7B,
  0x3D7B,
  0x3D7C,
  0x3D7C,
  0x3D7D,
  0x3D7D,
  0x3D7E,
  0x3D7E,
  0x3D7E,
  0x3D7F,
  0x3D7F,
  0x3D80,
  0x3D80,
  0x3D80,
  0x3D80,
  0x3D81,
  0x3D81,
  0x3D81,
  0x3D81,
  0x3D81,
  0x3D82,
  0x3D82,
  0x3D82,
  0x3D82,
  0x3D82,
  0x3D83,
  0x3D83,
  0x3D83,
  0x3D83,
  0x3D83,
  0x3D84,
  0x3D84,
  0x3D84,
  0x3D84,
  0x3D85,
  0x3D85,
  0x3D85,
  0x3D85,
  0x3D85,
  0x3D86,
  0x3D86,
  0x3D86,
  0x3D86,
  0x3D86,
  0x3D87,
  0x3D87,
  0x3D87,
  0x3D87,
  0x3D87,
  0x3D88,
  0x3D88,
  0x3D88,
  0x3D88,
  0x3D88,
  0x3D89,
  0x3D89,
  0x3D89,
  0x3D89,
  0x3D89,
  0x3D8A,
  0x3D8A,
  0x3D8A,
  0x3D8A,
  0x3D8A,
  0x3D8B,
  0x3D8B,
  0x3D8B,
  0x3D8B,
  0x3D8B,
  0x3D8C,
  0x3D8C,
  0x3D8C,
  0x3D8C,
  0x3D8C,
  0x3D8D,
  0x3D8D,
  0x3D8D,
  0x3D8D,
  0x3D8E,
  0x3D8E,
  0x3D8E,
  0x3D8E,
  0x3D8E,
  0x3D8F,
  0x3D8F,
  0x3D8F,
  0x3D8F,
  0x3D8F,
  0x3D90,
  0x3D90,
  0x3D90,
  0x3D90,
  0x3D90,
  0x3D91,
  0x3D91,
  0x3D91,
  0x3D91,
  0x3D91,
  0x3D92,
  0x3D92,
  0x3D92,
  0x3D92,
  0x3D92,
  0x3D93,
  0x3D93,
  0x3D93,
  0x3D93,
  0x3D93,
  0x3D94,
  0x3D94,
  0x3D94,
  0x3D94,
  0x3D94,
  0x3D95,
  0x3D95,
  0x3D95,
  0x3D95,
  0x3D96,
  0x3D96,
  0x3D96,
  0x3D96,
  0x3D96,
  0x3D97,
  0x3D97,
  0x3D97,
  0x3D97,
  0x3D97,
  0x3D98,
  0x3D98,
  0x3D98,
  0x3D98,
  0x3D98,
  0x3D99,
  0x3D99,
  0x3D99,
  0x3D99,
  0x3D99,
  0x3D9A,
  0x3D9A,
  0x3D9A,
  0x3D9A,
  0x3D9A,
  0x3D9B,
  0x3D9B,
  0x3D9B,
  0x3D9B,
  0x3D9B,
  0x3D9C,
  0x3D9C,
  0x3D9C,
  0x3D9C,
  0x3D9C,
  0x3D9D,
  0x3D9D,
  0x3D9D,
  0x3D9D,
  0x3D9D,
  0x3D9E,
  0x3D9E,
  0x3D9E,
  0x3D9E,
  0x3D9F,
  0x3D9F,
  0x3D9F,
  0x3D9F,
  0x3D9F,
  0x3DA0,
  0x3DA0,
  0x3DA0,
  0x3DA0,
  0x3DA0,
  0x3DA1,
  0x3DA1,
  0x3DA1,
  0x3DA1,
  0x3DA1,
  0x3DA2,
  0x3DA2,
  0x3DA2,
  0x3DA2,
  0x3DA2,
  0x3DA3,
  0x3DA3,
  0x3DA3,
  0x3DA3,
  0x3DA3,
  0x3DA4,
  0x3DA4,
  0x3DA4,
  0x3DA4,
  0x3DA4,
  0x3DA5,
  0x3DA5,
  0x3DA5,
  0x3DA5,
  0x3DA5,
  0x3DA6,
  0x3DA6,
  0x3DA6,
  0x3DA6,
  0x3DA7,
  0x3DA7,
  0x3DA7,
  0x3DA7,
  0x3DA7,
  0x3DA8,
  0x3DA8,
  0x3DA8,
  0x3DA8,
  0x3DA8,
  0x3DA9,
  0x3DA9,
  0x3DA9,
  0x3DA9,
  0x3DA9,
  0x3DAA,
  0x3DAA,
  0x3DAA,
  0x3DAA,
  0x3DAA,
  0x3DAB,
  0x3DAB,
  0x3DAB,
  0x3DAB,
  0x3DAB,
  0x3DAC,
  0x3DAC,
  0x3DAC,
  0x3DAC,
  0x3DAC,
  0x3DAD,
  0x3DAD,
  0x3DAD,
  0x3DAD,
  0x3DAD,
  0x3DAE,
  0x3DAE,
  0x3DAE,
  0x3DAE,
  0x3DAE,
  0x3DAF,
  0x3DAF,
  0x3DAF,
  0x3DAF,
  0x3DB0,
  0x3DB0,
  0x3DB0,
  0x3DB0,
  0x3DB0,
  0x3DB1,
  0x3DB1,
  0x3DB1,
  0x3DB1,
  0x3DB1,
  0x3DB2,
  0x3DB2,
  0x3DB2,
  0x3DB2,
  0x3DB2,
  0x3DB3,
  0x3DB3,
  0x3DB3,
  0x3DB3,
  0x3DB3,
  0x3DB4,
  0x3DB4,
  0x3DB4,
  0x3DB4,
  0x3DB4,
  0x3DB5,
  0x3DB5,
  0x3DB5,
  0x3DB5,
  0x3DB5,
  0x3DB6,
  0x3DB6,
  0x3DB6,
  0x3DB6,
  0x3DB6,
  0x3DB7,
  0x3DB7,
  0x3DB7,
  0x3DB7,
  0x3DB8,
  0x3DB8,
  0x3DB8,
  0x3DB8,
  0x3DB8,
  0x3DB9,
  0x3DB9,
  0x3DB9,
  0x3DB9,
  0x3DB9,
  0x3DBA,
  0x3DBA,
  0x3DBA,
  0x3DBA,
  0x3DBA,
  0x3DBB,
  0x3DBB,
  0x3DBB,
  0x3DBB,
  0x3DBB,
  0x3DBC,
  0x3DBC,
  0x3DBC,
  0x3DBC,
  0x3DBC,
  0x3DBD,
  0x3DBD,
  0x3DBD,
  0x3DBD,
  0x3DBD,
  0x3DBE,
  0x3DBE,
  0x3DBE,
  0x3DBE,
  0x3DBE,
  0x3DBF,
  0x3DBF,
  0x3DBF,
  0x3DBF,
  0x3DBF,
  0x3DC0,
  0x3DC0,
  0x3DC0,
  0x3DC0,
  0x3DC1,
  0x3DC1,
  0x3DC1,
  0x3DC1,
  0x3DC1,
  0x3DC2,
  0x3DC2,
  0x3DC2,
  0x3DC2,
  0x3DC2,
  0x3DC3,
  0x3DC3,
  0x3DC3,
  0x3DC3,
  0x3DC3,
  0x3DC4,
  0x3DC4,
  0x3DC4,
  0x3DC4,
  0x3DC4,
  0x3DC5,
  0x3DC5,
  0x3DC5,
  0x3DC5,
  0x3DC5,
  0x3DC6,
  0x3DC6,
  0x3DC6,
  0x3DC6,
  0x3DC6,
  0x3DC7,
  0x3DC7,
  0x3DC7,
  0x3DC7,
  0x3DC7,
  0x3DC8,
  0x3DC8,
  0x3DC8,
  0x3DC8,
  0x3DC8,
  0x3DC9,
  0x3DC9,
  0x3DC9,
  0x3DC9,
  0x3DCA,
  0x3DCA,
  0x3DCA,
  0x3DCA,
  0x3DCA,
  0x3DCB,
  0x3DCB,
  0x3DCB,
  0x3DCB,
  0x3DCB,
  0x3DCC,
  0x3DCC,
  0x3DCC,
  0x3DCC,
  0x3DCC,
  0x3DCD,
  0x3DCE,
  0x3DCF,
  0x3DD0,
  0x3DD1,
  0x3DD2,
  0x3DD3,
  0x3DD4,
  0x3DD5,
  0x3DD6,
  0x3DD7,
  0x3DD8,
  0x3DD9,
  0x3DDA,
  0x3DDB,
  0x3DDC,
  0x3DDD,
  0x3DDE,
  0x3DDF,
  0x3DE0,
  0x3DE1,
  0x3DE2,
  0x3DE3,
  0x3DE4,
  0x3DE5,
};

static u16 golden_bf16[] = {
  0x3fc9,
  0x3fbe,
  0x3fb4,
  0x3fac,
  0x3fa6,
  0x3fa0,
  0x3f9b,
  0x3f97,
  0x3f93,
  0x3f90,
  0x3f8e,
  0x3f8b,
  0x3f89,
  0x3f87,
  0x3f84,
  0x3f84,
  0x3f83,
  0x3f81,
  0x3f80,
  0x3f7d,
  0x3f7b,
  0x3f7a,
  0x3f78,
  0x3f76,
  0x3f75,
  0x3f73,
  0x3f72,
  0x3f70,
  0x3f70,
  0x3f6f,
  0x3f6d,
  0x3f6c,
  0x3f6c,
  0x3f6c,
  0x3f69,
  0x3f68,
  0x3f68,
  0x3f68,
  0x3f67,
  0x3f66,
  0x3f66,
  0x3f64,
  0x3f63,
  0x3f64,
  0x3f63,
  0x3f62,
  0x3f63,
  0x3f61,
  0x3f60,
  0x3f61,
  0x3f60,
  0x3f5f,
  0x3f5f,
  0x3f5f,
  0x3f5e,
  0x3f5e,
  0x3f5e,
  0x3f5e,
  0x3f5c,
  0x3f5c,
  0x3f5c,
  0x3f5e,
  0x3f5b,
  0x3f5c,
  0x3f5c,
  0x3f5b,
  0x3f5b,
  0x3f5a,
  0x3f5a,
  0x3f5a,
  0x3f5a,
  0x3f5a,
  0x3f59,
  0x3f5a,
  0x3f59,
  0x3f59,
  0x3f59,
  0x3f58,
  0x3f59,
  0x3f57,
  0x3f58,
  0x3f57,
  0x3f57,
  0x3f57,
  0x3f58,
  0x3f56,
  0x3f56,
  0x3f57,
  0x3f56,
  0x3f56,
  0x3f56,
  0x3f55,
  0x3f55,
  0x3f55,
  0x3f55,
  0x3f55,
  0x3f55,
  0x3f55,
  0x3f55,
  0x3f54,
  0x3f55,
  0x3f55,
  0x3f55,
  0x3f54,
  0x3f54,
  0x3f54,
  0x3f55,
  0x3f55,
  0x3f54,
  0x3f54,
  0x3f54,
  0x3f55,
  0x3f54,
  0x3f53,
  0x3f53,
  0x3f54,
  0x3f53,
  0x3f53,
  0x3f54,
  0x3f53,
  0x3f53,
  0x3f53,
  0x3f53,
  0x3f54,
  0x3f53,
  0x3f53,
  0x3f53,
  0x3f53,
  0x3f52,
  0x3f53,
  0x3f53,
  0x3f52,
  0x3f53,
  0x3f53,
  0x3f52,
  0x3f53,
  0x3f51,
  0x3f52,
  0x3f51,
  0x3f53,
  0x3f52,
  0x3f53,
  0x3f53,
  0x3f53,
  0x3f50,
  0x3f52,
  0x3f52,
  0x3f52,
  0x3f52,
  0x3f51,
  0x3f51,
  0x3f53,
  0x3f52,
  0x3f51,
  0x3f51,
  0x3f51,
  0x3f50,
  0x3f52,
  0x3f52,
  0x3f51,
  0x3f52,
  0x3f51,
  0x3f50,
  0x3f52,
  0x3f52,
  0x3f50,
  0x3f50,
  0x3f51,
  0x3f50,
  0x3f50,
  0x3f52,
  0x3f51,
  0x3f50,
  0x3f50,
  0x3f4f,
  0x3f4f,
  0x3f51,
  0x3f50,
  0x3f50,
  0x3f50,
  0x3f50,
  0x3f4f,
  0x3f51,
  0x3f50,
  0x3f50,
  0x3f50,
  0x3f50,
  0x3f4f,
  0x3f50,
  0x3f4f,
  0x3f50,
  0x3f50,
  0x3f50,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f4e,
  0x3f4e,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f50,
  0x3f4f,
  0x3f4e,
  0x3f50,
  0x3f50,
  0x3f50,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f4e,
  0x3f50,
  0x3f4e,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f50,
  0x3f4e,
  0x3f4f,
  0x3f4f,
  0x3f4f,
  0x3f4e,
  0x3f4f,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4e,
  0x3f4d,
  0x3f4e,
  0x3f4e,
  0x3f4e,
  0x3f4f,
  0x3f4e,
  0x3f4f,
  0x3f4f,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4d,
  0x3f4e,
  0x3f4e,
  0x3f4e,
  0x3f4f,
  0x3f4e,
  0x3f4f,
  0x3f4f,
  0x3f4e,
  0x3f4e,
  0x3f4d,
  0x3f4e,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4f,
  0x3f4e,
  0x3f4e,
  0x3f4d,
  0x3f4e,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4e,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4e,
  0x3f4e,
  0x3f4d,
  0x3f4e,
  0x3f4e,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4e,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4e,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4e,
  0x3f4e,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4d,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4c,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4b,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4c,
  0x3f4b,
  0x3f4a,
  0x3f49,
  0x3f4a,
  0x3f4a,
  0x3f4b,
  0x3f4b,
  0x3f4a,
  0x3f4b,
  0x3f4a,
  0x3f4b,
  0x3f4a,
  0x3f4a,
};

// <! gen invert sqrt
static double _gen_atan2(float y, float x) {
  // come from https://en.wikipedia.org/wiki/Atan2
  if (!(abs(atan2(y, x)  - 2 * atan(y / (sqrt(x*x + y*y)) + x)) < 0.001)) {
    //printf("atan2(%f, %f) is %f, y / (sqrt(x*x + y*y) + x) is %f\n",
    //    y, x, atan2(y, x), 2 * y / (sqrt(x*x + y*y) + x));
  }
  return atan2(y, x);
}

static double _gen_sqrt(int base, int p) {
  // y = x ^ 0.5
  double f = (double) (pow(base, p * 0.5));

  if (isnan(f)) {
    assert(0);
  }
  return f;
}

static double _gen_reciprocal(int base, int p) {
  // y = x ^ -1
  double f = (double) (pow(base, -1 * p));

  if (isnan(f)) {
    assert(0);
  }
  return f;
}

// <! gen atan f(x) = atan(x)
static double _gen_atan(float i) {
  return atan(i);
}

static void tl_lut_ref(
    u16 *ofmap,
    u16 *ifmap,
    u16 *ifmap2,
    tl_shape_t ifmap_shape
    )
{
  for (u32 i = 0; i < tl_shape_size(&ifmap_shape); i++) {
    // _gen_atan2(y, x)
    float x = convert_bf16_fp32(ifmap[i]);
    float y = convert_bf16_fp32(ifmap2[i]);
    float v = _gen_atan2(y, x);
#if 0 //ifdef DBG
    printf("ref out[%u] is %f\n", i, v);
#endif /* ifdef DBG */
    ofmap[i] = convert_fp32_bf16(v);

    if (mode == PRE_DATA_COMPARE_FIX) {
      ofmap[i] = golden_bf16[i];
    }
    else if (mode == DATA_COMPARE_U8) {
      ofmap[i] = (u8) convert_bf16_s8(ofmap[i]);
    }
  }
}

static void gen_sqrt(u16 *table_data, u64 table_size) {
  //<! 32*8 table, duplicate `channel` times;
  int exp_start = -62; // hard code for bf16 range
  int half = table_size / channel / 2;
  u64 idx = 0;
  assert(table_size);
  assert(half == 128);

  // prepare channel 0
  double s = 0.0;
  table_data[idx] = convert_fp32_bf16(s); // 0^0.5 = 0
#ifdef DBG
  printf("t [%" PRIu64 "] is %f(%.8lf)[idx:%f][2^%f] bf %x\n", idx, convert_bf16_fp32(table_data[idx]), s, (float)exp_start, (float)(exp_start/2), table_data[idx]);
#endif
  idx++;

  // > 0, exp from 0 -62 -61 ..  62  63
  for (int i = 0; i < half; i++) {
    //float exp = round((exp_start + i) / 2) * 2;
    int shift = (exp_start + i);
    bool is_odd = (shift % 2);
    float exp = shift;
    if (is_odd) {
      exp = shift > 0 ? exp - 1 : exp - 1;
    }

    double s = _gen_sqrt(2, exp);
    table_data[idx] = convert_fp32_bf16(s);
#ifdef DBG
    printf("t [%" PRIu64 "] is %f [idx:%f][2^%f(%f)] bf %x\n", idx,
        convert_bf16_fp32(table_data[idx]),
        float(exp_start + i), exp/2, (exp_start + i) / 2.0, 
        table_data[idx]);
#endif
    idx++;
  }

  //// idx = 127 dont care
#if 0
  s = _gen_sqrt(2, -0);
  table_data[idx] = convert_fp32_bf16(s);
#if 1
  printf("t [%" PRIu64 "] is %f[%d] bf %x\n", idx, convert_bf16_fp32(table_data[idx]), 0, table_data[idx]);
#endif
  idx++;

  for (int i = 1; i < half; i++) {
    float exp = exp_start + i;
    double s = _gen_sqrt(-2, exp);
    table_data[idx] = convert_fp32_bf16(s);
#ifdef DBG
    printf("t [%" PRIu64 "] is %f(%e - %.8lf)[(-2)^%f] bf %x\n", idx, convert_bf16_fp32(table_data[idx]), s, s, exp, table_data[idx]);
#endif
    idx++;
  }

  // idx = 255 dont care
  //s = _gen_sqrt(2, 0);
  //table_data[idx] = convert_fp32_bf16(s);
  //printf("t [%" PRIu64 "] is %f[%d]\n", idx, convert_bf16_fp32(table_data[idx]), 0);
  //idx++;
#endif

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (u32 i = 1; i < channel; i++) {
    memcpy(&table_data[i * table_hw], &table_data[0], sizeof(u16) * table_hw);
  }
}

static void gen_sqrt_mantissa(u16 IN *table_data, u16* OUT table_mantissa, u64 table_size) {

  u32 half = table_size / channel / 2;
  assert(half == 128);
  assert(table_data);

  int idx = 0;
  double d;
  for (u32 i = 0; i < half; i++) {
    d = 1 + i * 1 / 128.0;
    d = (double) pow(d, 0.5);
    table_mantissa[128+idx] = convert_fp32_bf16(d);
#ifdef DBG
    //printf(", [%u] is %lf\n", i+128, d);
#endif /* ifdef DBG */

    //13=2^3x1.625=(2^2)x(2^1x1.625)
    d = 2 * (1 + i * 1 / 128.0);
    d = (double) pow(d, 0.5);
    table_mantissa[idx] = convert_fp32_bf16(d);
#ifdef DBG
    //printf("mantissa [%u] is %lf", i, d);
#endif /* ifdef DBG */
    idx++;
  }
#ifdef DBG
  for (u32 i = 0; i < 2 * half; i++) {
    printf("mantissa [%u] is %lf, 0x%x\n", i, convert_bf16_fp32(table_mantissa[i]),
        table_mantissa[i]);
  }
#endif /* ifdef DBG */

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (u64 i = 1; i < channel; i++) {
    memcpy(&table_mantissa[table_hw * i], &table_mantissa[0], sizeof(u16) * table_hw);
  }
}

static void gen_reciprocal(u16 *table_data, u64 table_size) {
  //<! 32*8 table, duplicate `channel` times;
  int exp_start = -62; // hard code for bf16 range
  int half = table_size / channel / 2;
  u64 idx = 0;
  assert(table_size);
  assert(half == 128);

  // prepare channel 0
  double s = 0.0;
  // 0^-1 is invalid, skip it
  table_data[idx] = convert_fp32_bf16(s);
#if 0
  printf("t [%" PRIu64 "] is %f(%.8lf)[idx:%f][2^%f] bf %x\n", idx, convert_bf16_fp32(table_data[idx]), s, (float)exp_start, (float)(exp_start/2), table_data[idx]);
#endif
  idx++;

  // > 0, exp from 0 -62 -61 ..  62  63
  for (int i = 0; i < half; i++) {
    int shift = (exp_start + i);
    bool is_odd = (shift % 2);
    float exp = shift;
    if (is_odd) {
      exp = shift > 0 ? exp - 1 : exp - 1;
    }

    double s = _gen_reciprocal(2, exp);
    table_data[idx] = convert_fp32_bf16(s);
#ifdef DBG
    printf("t [%" PRIu64 "] is %f [idx:%f][2^%f] bf %x\n", idx,
        convert_bf16_fp32(table_data[idx]),
        float(exp_start + i), -1 * exp,
        table_data[idx]);
#endif
    idx++;
  }

  s = _gen_reciprocal(2, -0);
  table_data[idx] = convert_fp32_bf16(s);
#ifdef DBG
  printf("t [%" PRIu64 "] is %f[%d] bf %x\n", idx, convert_bf16_fp32(table_data[idx]), 0, table_data[idx]);
#endif
  idx++;

  for (int i = 1; i < half; i++) {
    int shift = (exp_start + i);
    bool is_odd = (shift % 2);
    float exp = shift;
    if (is_odd) {
      exp = shift > 0 ? exp - 1 : exp - 1;
    }

    double s = _gen_reciprocal(-2, exp);
    table_data[idx] = convert_fp32_bf16(s);
#ifdef DBG
    printf("t [%" PRIu64 "] is %f(%e - %.8lf)[(-2)^%f] bf %x\n", idx, convert_bf16_fp32(table_data[idx]), s, s, exp, table_data[idx]);
#endif
    idx++;
  }

  // idx = 255 dont care
  //s = _gen_reciprocal(2, 0);
  //table_data[idx] = convert_fp32_bf16(s);
  //printf("t [%" PRIu64 "] is %f[%d]\n", idx, convert_bf16_fp32(table_data[idx]), 0);
  //idx++;

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (u32 i = 1; i < channel; i++) {
    memcpy(&table_data[i * table_hw], &table_data[0], sizeof(u16) * table_hw);
  }
}

static void gen_reciprocal_mantissa(u16 IN *table_data, u16* OUT table_mantissa, u64 table_size) {

  u32 half = table_size / channel / 2;
  assert(half == 128);
  assert(table_data);

  int idx = 0;
  double d;
  for (u32 i = 0; i < half; i++) {
    d = 1 + i * 1 / 128.0;
    d = (double) pow(d, -1);
    table_mantissa[128+idx] = convert_fp32_bf16(d);

    //13=2^3x1.625=(2^2)x(2^1x1.625)
    d = 2 * (1 + i * 1 / 128.0);
    d = (double) pow(d, -1);
    table_mantissa[idx] = convert_fp32_bf16(d);
    idx++;
  }

#ifdef DBG
  for (u32 i = 0; i < 2 * half; i++) {
    printf("mantissa [%u] is %lf, 0x%x\n", i, convert_bf16_fp32(table_mantissa[i]),
        table_mantissa[i]);
  }
#endif /* ifdef DBG */

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (u64 i = 1; i < channel; i++) {
    memcpy(&table_mantissa[table_hw * i], &table_mantissa[0], sizeof(u16) * table_hw);
  }
}

static void gen_atan_y0(u16 *table_data_y0, u64 table_size,
    int range_start, int range_end) {
  float scale = table_hw / (1.0 * abs(range_start - range_end));
  //<! 32*8 table, duplicate `channel` times;
  int half = table_size / channel / 2;
  double s;
  u64 idx = 0;

  assert(table_size);
  assert(half == 128);

  // prepare channel 0
  // x [0, 127]
  for (int i = 0; i < half; i++) {
    float _idx = idx / scale;
    s = _gen_atan(_idx);
    lut[idx] = s;
    table_data_y0[idx] = convert_fp32_bf16(s);
#ifdef DBG
    printf("t [%" PRIu64 "] is %f[%d], 0x%x fp is %f d is %.8lf, input is %f\n", idx, convert_bf16_fp32(table_data_y0[idx]), i, table_data_y0[idx], (float)s, s, _idx);
#endif
    idx++;
  }

  // x = -128
  s = _gen_atan(range_start);
  lut[idx] = s;
  table_data_y0[idx] = convert_fp32_bf16(s);
#ifdef DBG
  printf("t [%" PRIu64 "] is %f[%d] bf %x\n", idx, convert_bf16_fp32(table_data_y0[idx]), 0, table_data_y0[idx]);
#endif
  idx++;

  // x [-128~-1], 2's complement
  for (int i = 1; i < half; i++) {
    float _idx = (i) / scale;
    s = _gen_atan(range_start + _idx);
    lut[idx] = s;
    table_data_y0[idx] = convert_fp32_bf16(s);
#ifdef DBG
    printf("t [%" PRIu64 "] is %f[%d], 0x%x fp is %f d is %.8lf input is %f\n", idx, convert_bf16_fp32(table_data_y0[idx]), -127 + i, table_data_y0[idx], (float)s, s, range_start + _idx);
#endif
    idx++;
  }

  // idx = 255 dont care
  //s = _gen_atan(2, 0);
  //table_data_y0[idx] = convert_fp32_bf16(s);
  //printf("t [%" PRIu64 "] is %f[%d]\n", idx, convert_bf16_fp32(table_data_y0[idx]), 0);
  //idx++;

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (u32 i = 1; i < channel; i++) {
    memcpy(&table_data_y0[i * table_hw], &table_data_y0[0], sizeof(u16) * table_hw);
  }
}

static void gen_atan_slope(u16 IN *table_data_y0, u16* OUT table_slope, u64 table_size,
    int range_start, int range_end) {

  float scale = table_hw / (1.0 * abs(range_start - range_end));
  u32 half = table_size / channel / 2;
  assert(half == 128);
  assert(table_data_y0);

  for (u32 i = 0; i < table_hw; i++) {
    double x0 = lut[i];
    double x1 = lut[i+1];
    double delta = 1.0;
    if (i == half - 1) {
      //<! slope[127] means f(127)~f(128)
      double f = _gen_atan(range_end);
      x1 = f;
    }
    else if (i == half) {
      // 128 index mean x1 is -129 and x0 is -128
      x1 = _gen_atan(range_start - 1/scale);
      delta = -1.0;
    }
    else if (i > half) {
      x0 = lut[i];
      x1 = lut[i-1];
      delta = -1.0;
    }
    double s = (x1 - x0) / delta; // x1 already scale up
    table_slope[i] = convert_fp32_bf16((float)s);
#ifdef DBG
    printf ("slope table [%u] = (bf16 %f double %.8lf float %f), 0x%x, %.8lf - %.8lf(%.8lf)\n",
        i, convert_bf16_fp32(table_slope[i]), s, (float)s, table_slope[i], x1, x0, x1-x0);
#endif
  }

#if 0 //def DBG
  for (u32 i = 0; i < 2 * half; i++) {
    printf("slope [%u] is %lf, 0x%x\n", i, convert_bf16_fp32(table_slope[i]),
        table_slope[i]);
  }
#endif /* ifdef DBG */

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (u64 i = 1; i < channel; i++) {
    memcpy(&table_slope[table_hw * i], &table_slope[0], sizeof(u16) * table_hw);
  }
}

static bool verify(u16 *ofmap_data, u16 *ref_data, u16* ifmap, u64 ifmap_shape_size,
    TEST_MODE mode, float epsilon) {
  u64 size = ifmap_shape_size;

  for (u64 i = 0; i < size; i++) {
    bool is_close;
    u16 ref;
    u16 ofmap_data_bf16;
    float ref_f;
    float ofmap_data_f;
    u32 shift;

    if (mode == DATA_COMPARE_U8) {
      shift = (i%2)*8;
      ref = ref_data[i];
      ofmap_data_bf16 = (u16)ofmap_data[i/2];
      ofmap_data_f = (float)(ofmap_data[i/2] >> shift);
      ref_f = (float)(ref);

      is_close = ((u8)(ofmap_data[i/2] >> shift)) == (u8)ref;

      //printf("[%" PRIu64 "] of is %x ref is %x\n", i, (u8)(ofmap_data[i/2] >> shift), (u8)ref);
    }
    else {
      ref = ref_data[i];
      ref_f = convert_bf16_fp32(ref);
      ofmap_data_f = convert_bf16_fp32(ofmap_data[i]);
      ofmap_data_bf16 = ofmap_data[i];

      if (mode == PRE_DATA_COMPARE_FIX) {
        is_close = ofmap_data[i] == ref;
      }
      else {
        is_close = fabs(ref_f-ofmap_data_f) < epsilon;
      }
    }

    if (!is_close) {
      fprintf(stderr,
          "comparing failed at ofmap_data[%" PRIu64 "](input:%e), got %x, exp %x, fp32: got %e exp %e\n",
          i, convert_bf16_fp32(ifmap[i]),
          ofmap_data_bf16, ref, ofmap_data_f, ref_f);
      exit(-1);
    }
  }

  return true;
}

/*
 * NOTICE: it could occupy 2 lookup table size which shape is <1,32,32,8> with bf16 data type
 *
 * \tl_buf, \tl_buf2 tmp buffer, the shape MUST be same with \tl_ifmap
 * \tl_ofmap_u8 result as u8 type, NULL means use bf16 result
 * \tl_ofmap_bf16 result as bf16, MUST given for tmp buffer used
 * \range_start, \range_end specify data range, default range is -8 ~ +8
 */
static int bf16_emit(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk,
    tl_t* tl_ifmap,
    tl_t* tl_ifmap2,
    tl_t* tl_buf,
    tl_t* tl_buf2,
    tl_t* OUT tl_ofmap_bf16,
    tl_t* OUT tl_ofmap_u8,
    tl_t *tl_table_answer, tl_t *tl_table_answer_mantissa,
    int range_start, int range_end
    ) {
  assert(tl_ofmap_bf16);
  assert(tl_ifmap2);
  assert(tl_buf2);
  assert(tl_buf);
  assert(tl_ofmap_bf16);

  fmt_t fmt = FMT_BF16;
  float scale = table_hw / (1.0 * abs(range_start - range_end));
  int data_type_size = bytesize_of_fmt(fmt);
  u64 table_size = tl_shape_size(&tl_table_answer->shape);
  u64 table_bytesize  =  table_size * data_type_size;

  bmk1880v2_tdma_tg2l_tensor_copy_param_t copy_p2, copy_p3;
  memset(&copy_p2, 0, sizeof(copy_p2));
  memset(&copy_p2, 0, sizeof(copy_p3));

  bmk1880v2_tdma_l2l_tensor_copy_param_t p10;
  memset(&p10, 0, sizeof(p10));

  // 1. get x^2 + y ^ 2
  bmk1880v2_tiu_element_wise_mul_param_t p1;
  memset(&p1, 0, sizeof(p1));
  p1.res_high = NULL;
  p1.res_low = tl_buf2;
  p1.a = tl_ifmap;
  p1.b_is_const = 0;
  p1.b = tl_ifmap;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(bmk, &p1);

  bmk1880v2_tiu_element_wise_mac_param_t p2;
  memset(&p2, 0, sizeof(p2));
  p2.res_high = 0;
  p2.res_low = tl_buf2;
  p2.res_is_int8 = 0;
  p2.a = tl_ifmap2;
  p2.b_is_const = 0;
  p2.b = tl_ifmap2;
  p2.lshift_bits = 0;
  p2.rshift_bits = 0;
  p2.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mac(bmk, &p2);

  // 2. sqrt

  // prepare exp table
  u16 *table_data = (u16 *)xmalloc(table_bytesize);
  gen_sqrt (table_data, table_size);

  // prepare mantissa table
  u16 *table_data_mantissa = (u16 *)xmalloc(table_bytesize);
  gen_sqrt_mantissa(table_data, table_data_mantissa, table_size);

  prepare_put_bf16_tensor_g2l(ctx, bmk, tl_table_answer, table_data, fmt, &copy_p2);
  prepare_put_bf16_tensor_g2l(ctx, bmk, tl_table_answer_mantissa, table_data_mantissa, fmt, &copy_p3);

  launch_put_bf16_tensor_g2l(ctx, bmk, copy_p2.src, &copy_p2); // table value
  launch_put_bf16_tensor_g2l(ctx, bmk, copy_p3.src, &copy_p3); // table mantissa

  // remove low 8 bits by int8 copy with stride
  // <! get index(pow)
  memset(&p10, 0x00, sizeof(bmk1880v2_tdma_l2l_tensor_copy_param_t));
  p10.dst = tl_buf;
  p10.src = tl_buf2;
  p10.mv_lut_idx = true;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(bmk, &p10);
  p10.mv_lut_idx = false;
  test_submit(ctx);

  // <! get f(x0) = 2^(x0*-0.5)
  bmk1880v2_tiu_lookup_table_param_t p12;
  memset(&p12, 0, sizeof(p12));
  p12.ofmap = tl_ofmap_bf16;
  p12.ifmap = tl_buf;
  p12.table = tl_table_answer;
  bmk1880v2_tiu_lookup_table(bmk, &p12);

  // <! get mantissa value
  p12.ofmap = tl_buf;
  p12.ifmap = tl_buf2;
  p12.table = tl_table_answer_mantissa;
  bmk1880v2_tiu_lookup_table(bmk, &p12);

  // sqrt = (2^exp) * mantissa
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_buf;
  p1.b_is_const = 0;
  p1.b = tl_ofmap_bf16;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(bmk, &p1);

  // 3. add x
  bmk1880v2_tiu_element_wise_add_param_t p4;
  memset(&p4, 0, sizeof(p4));
  p4.res_high = 0;
  p4.res_low = tl_ofmap_bf16;
  p4.a_high = 0;
  p4.a_low = tl_ofmap_bf16;
  p4.b_is_const = 0;
  p4.b_high = 0;
  p4.b_low = tl_ifmap;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  bmk1880v2_tiu_element_wise_add(bmk, &p4);
 
  // 4. get reciprocal
  gen_reciprocal (table_data, table_size);
  gen_reciprocal_mantissa(table_data, table_data_mantissa, table_size);
  prepare_put_bf16_tensor_g2l(ctx, bmk, tl_table_answer, table_data, fmt, &copy_p2);
  prepare_put_bf16_tensor_g2l(ctx, bmk, tl_table_answer_mantissa, table_data_mantissa, fmt, &copy_p3);

  launch_put_bf16_tensor_g2l(ctx, bmk, copy_p2.src, &copy_p2); // table value
  launch_put_bf16_tensor_g2l(ctx, bmk, copy_p3.src, &copy_p3); // table mantissa

  // <! get index(pow)
  memset(&p10, 0x00, sizeof(bmk1880v2_tdma_l2l_tensor_copy_param_t));
  p10.dst = tl_buf;
  p10.src = tl_ofmap_bf16;
  p10.mv_lut_idx = true;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(bmk, &p10);
  p10.mv_lut_idx = false;
  test_submit(ctx);

  // <! get f(x0) = 2^(x0*-0.5)
  p12.ofmap = tl_buf;
  p12.ifmap = tl_buf;
  p12.table = tl_table_answer;
  bmk1880v2_tiu_lookup_table(bmk, &p12);

  // <! get mantissa value
  p12.ofmap = tl_buf2;
  p12.ifmap = tl_ofmap_bf16;
  p12.table = tl_table_answer_mantissa;
  bmk1880v2_tiu_lookup_table(bmk, &p12);

  // reciprocal = (2^exp) * mantissa
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_buf;
  p1.b_is_const = 0;
  p1.b = tl_buf2;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(bmk, &p1);

  // 5. mul y
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_ofmap_bf16;
  p1.b_is_const = 0;
  p1.b = tl_ifmap2;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(bmk, &p1);

  // 6. get f(x) = atan(x)
  gen_atan_y0 (table_data, table_size, range_start, range_end);
  gen_atan_slope(table_data, table_data_mantissa, table_size, range_start, range_end);

  prepare_put_bf16_tensor_g2l(ctx, bmk, tl_table_answer, table_data, fmt, &copy_p2);
  prepare_put_bf16_tensor_g2l(ctx, bmk, tl_table_answer_mantissa, table_data_mantissa, fmt, &copy_p3);
  launch_put_bf16_tensor_g2l(ctx, bmk, copy_p2.src, &copy_p2); // table value
  launch_put_bf16_tensor_g2l(ctx, bmk, copy_p3.src, &copy_p3); // table mantissa

  // scale input for remap its idx(-x~x) to (-127~127), dirty tl_ifmap
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_ofmap_bf16;
  p1.b_is_const = 1;
  p1.b_const.val = convert_fp32_bf16(scale);
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(bmk, &p1);

  // <! get idx from bf16->int8
  tl_shape_t tl_shape_int8 = {1, channel, tl_ofmap_bf16->shape.h * tl_ofmap_bf16->shape.w, 1};
  memset(&p10, 0x00, sizeof(bmk1880v2_tdma_l2l_tensor_copy_param_t));
  bmk1880v2_tensor_lmem_t dst;
  memcpy(&dst, tl_ifmap, sizeof(bmk1880v2_tensor_lmem_t)); 
  dst.fmt = FMT_I8;
  dst.shape = tl_shape_int8;
  dst.stride = bmk1880v2_tensor_lmem_default_stride(bmk, dst.shape, dst.fmt, /*align*/1);
  dst.stride.h = dst.stride.h * 2;
  dst.int8_rnd_mode = 1;
  p10.dst = &dst;
  p10.src = tl_ofmap_bf16;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(bmk, &p10);
  test_submit(ctx);
  dst.int8_rnd_mode = 0; // reset

  // <! int8 to fb16 format cus for sub use, sub MUST in the same format
  memset(&p10, 0x00, sizeof(bmk1880v2_tdma_l2l_tensor_copy_param_t));
  p10.dst = tl_buf; //<! bf16
  p10.src = &dst;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(bmk, &p10);
  test_submit(ctx);

  // <! sub, diff base , a - b
  // (x - x0)
  bmk1880v2_tiu_element_wise_sub_param_t p5;
  memset(&p5, 0, sizeof(p5));
  p5.res_high = 0;
  p5.res_low = tl_buf;
  p5.a_high = 0;
  p5.a_low = tl_ofmap_bf16;
  p5.b_high = 0;
  p5.b_low = tl_buf;
  p5.rshift_bits = 0;
  bmk1880v2_tiu_element_wise_sub(bmk, &p5);

  // get f(x0) and slope(x)
  // reshape, 16->16
  dst.fmt = fmt;
  dst.shape = tl_buf->shape;
  dst.stride = tl_buf->stride;

  // <! get slope by index
  // <! ( (f(x1) - f(x0)) / (x1 - x0) )
  // <! TIU MUST with same shape and stride, we leverage output map shape and stride
  memset(&p12, 0x0, sizeof(bmk1880v2_tiu_lookup_table_param_t));
  p12.ofmap = tl_buf2;
  p12.ifmap = &dst;
  p12.table = tl_table_answer_mantissa;
  bmk1880v2_tiu_lookup_table(bmk, &p12);

  // base f(x0)
  memset(&p12, 0x0, sizeof(bmk1880v2_tiu_lookup_table_param_t));
  p12.ofmap = tl_ofmap_bf16;
  p12.ifmap = &dst;
  p12.table = tl_table_answer;
  bmk1880v2_tiu_lookup_table(bmk, &p12);

  // <! mac
  // <! part A + part B, a * b + res = res
  p2.res_high = 0;
  p2.res_low = tl_ofmap_bf16;
  p2.res_is_int8 = 0;
  p2.a = tl_buf2;
  p2.b_is_const = 0;
  p2.b = tl_buf;
  p2.lshift_bits = 0;
  p2.rshift_bits = 0;
  p2.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mac(bmk, &p2);

  // 7. 2*
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_ofmap_bf16;
  p1.b_is_const = 1;
  p1.b_const.val = convert_fp32_bf16(2);
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(bmk, &p1);

  if (tl_ofmap_u8) {
    p10.dst = tl_ofmap_u8;
    p10.src = tl_ofmap_bf16;
    bmk1880v2_tdma_l2l_bf16_tensor_copy(bmk, &p10);
  }

  test_submit(ctx);


  free_tl(bmk, tl_table_answer_mantissa);
  free_tl(bmk, tl_table_answer);

  free(table_data);
  free(table_data_mantissa);

  return 0;
}

static void gen_test_pattern(u16 *ifmap, u16 *ifmap2, TEST_MODE mode,
    u64 ifmap_shape_size,
    int range_start, int range_end) {

  if (mode == PRE_DATA_COMPARE_FIX) {
    memcpy(ifmap, &test_pattern, sizeof(test_pattern));
  }
  else {
    float LO = range_start;
    float HI = range_end;
    for (u64 i = 0; i < ifmap_shape_size; i++) {
      srand (static_cast <unsigned> (time(0)));
      std::random_device rd;
      std::mt19937 e2(rd());
      //std::uniform_real_distribution<> dist(pow(2,-62), pow(2,63));
      for (u64 i = 0; i < ifmap_shape_size; i++) {
        //float r3 = dist(e2);
        float r3 = LO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(HI-LO)));
        ifmap[i] = convert_fp32_bf16(r3);
      }
    }
  }

  for (u64 i = 0; i < ifmap_shape_size; i++) {
    ifmap2[i] = convert_fp32_bf16(convert_bf16_fp32(ifmap[i]) + 0.001);
  }

#if 0//#ifdef DBG
  for (u64 i = 0; i < ifmap_shape_size; i++) {
    printf("source if[%" PRIu64 "] bf16 %f 0x%x, log2f is %f\n", i, convert_bf16_fp32(ifmap[i]), ifmap[i], floor(log2((convert_bf16_fp32(ifmap[i])))));
  }
#endif /* ifdef DBG */
}

static void test_tl_int8_lut_bf16(CVI_RT_HANDLE *ctx, bmk_ctx_t *bmk,
    u32 input_n, u32 input_c, u32 input_h, u32 input_w,
    float epsilon, int range_start, int range_end)
{
  // TODO: check more shape / align
  tl_shape_t ifmap_shape = {input_n, input_c, input_h, input_w};
  tl_shape_t ofmap_shape = ifmap_shape;
  tl_shape_t table_shape = {1, channel, table_h, table_w}; // hard code for hw, hw:32x8

  u64 ifmap_shape_size = tl_shape_size(&ifmap_shape);
  u64 ofmap_size = tl_shape_size(&ofmap_shape);

  fmt_t fmt = FMT_BF16;

  int data_type_size = bytesize_of_fmt(fmt);
  u64 ifmap_bytesize  =  ifmap_shape_size * data_type_size;
  u64 ofmap_bytesize  =  ofmap_size * data_type_size;

  u16 *ifmap = (u16 *)xmalloc(ifmap_bytesize);
  u16 *ifmap2 = (u16 *)xmalloc(ifmap_bytesize);
  u16 *ref_data = (u16 *)xmalloc(ofmap_bytesize);

  tl_t *tl_ifmap = alloc_tl(bmk,ifmap_shape, fmt, /*align*/1);
  tl_t *tl_ifmap2 = alloc_tl(bmk,ifmap_shape, fmt, /*align*/1);
  tl_t *tl_ofmap_bf16 = alloc_tl(bmk,ofmap_shape, fmt, /*align*/1);
  tl_t *tl_buf = tl_ifmap ? alloc_tl(bmk, tl_ifmap->shape, fmt, /*align*/1) : nullptr;
  tl_t *tl_buf2 = alloc_tl(bmk, tl_ifmap->shape, fmt, /*align*/1);
  tl_t *tl_table_answer = alloc_tl(bmk, table_shape, fmt, /*align*/1);
  tl_t *tl_table_answer_mantissa = alloc_tl(bmk, table_shape, fmt, /*align*/1);

  gen_test_pattern(ifmap, ifmap2, mode, ifmap_shape_size, range_start, range_end);
  tl_lut_ref(ref_data, ifmap, ifmap2, ifmap_shape);

  tl_t *tl_ofmap_u8 = nullptr;
  tl_t *out = tl_ofmap_bf16;

  if (mode == DATA_COMPARE_U8) {
    tl_ofmap_u8 =
      alloc_tl(bmk,ofmap_shape, FMT_U8, /*align*/1);
    out = tl_ofmap_u8;
  }

  // <! FIXME: prepare it
  bmk1880v2_tdma_tg2l_tensor_copy_param_t copy_p1, copy_p2;
  memset(&copy_p1, 0, sizeof(copy_p1));
  memset(&copy_p2, 0, sizeof(copy_p2));
  prepare_put_bf16_tensor_g2l(ctx, bmk, tl_ifmap, ifmap, fmt, &copy_p1);
  prepare_put_bf16_tensor_g2l(ctx, bmk, tl_ifmap2, ifmap2, fmt, &copy_p2);

  launch_put_bf16_tensor_g2l(ctx, bmk, copy_p1.src, &copy_p1); // input
  launch_put_bf16_tensor_g2l(ctx, bmk, copy_p2.src, &copy_p2); // input 2

  bf16_emit(ctx, bmk,
      tl_ifmap,
      tl_ifmap2,
      tl_buf,
      tl_buf2,
      tl_ofmap_bf16,
      tl_ofmap_u8,
      tl_table_answer, tl_table_answer_mantissa,
      range_start, range_end
      );

  u16* ofmap_data = (u16*)get_bf16_tensor_l2g(ctx, bmk, out, out->fmt);

  verify(ofmap_data, ref_data, ifmap, ifmap_shape_size, mode, epsilon);

  if (tl_ofmap_u8) {
    free_tl(bmk, tl_ofmap_u8);
  }

  free_tl(bmk, tl_buf2);
  free_tl(bmk, tl_buf);
  free_tl(bmk, tl_ofmap_bf16);
  free_tl(bmk, tl_ifmap2);
  free_tl(bmk, tl_ifmap);

  free(ifmap);
  free(ifmap2);
  free(ref_data);
  free(ofmap_data);
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bmk;
  int round_mode;

  round_mode = set_store_feround();

  test_init(&ctx, &bmk);

  for (int i = PRE_DATA_COMPARE_FIX; i < DATA_COMPARE_U8; i++)
  //for (int i = PRE_DATA_COMPARE_FIX; i < DATA_COMPARE; i++)
  //for (int i = DATA_COMPARE; i < DATA_COMPARE_U8; i++) 
  {
    mode = static_cast<TEST_MODE>(i);
    printf ("test mode %d...\n", mode);

    int input_n = 1;
    int input_c = channel;
    int input_h = 1;
    int input_w = 1;
    float epsilon = 0.1;
    int range_start = -8;
    int range_end = 8;

    if (mode == PRE_DATA_COMPARE_FIX) {
      input_h = 4;
      input_w = 8;
    }
    else {
      input_h = input_w = 16;
    }

    test_tl_int8_lut_bf16(&ctx, bmk,
        input_n, input_c, input_h, input_w,
        epsilon, range_start, range_end);
  }

  test_exit(&ctx);
  restore_feround(round_mode);
  return 0;
}
