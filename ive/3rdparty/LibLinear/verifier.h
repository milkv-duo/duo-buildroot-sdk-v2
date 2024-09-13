#pragma once

//#include "describer.h"
#include "string.h"
#include "math.h"

#include "LibLinear.hpp"

using namespace std;


class Cverifier
{
public:
	Cverifier();
	~Cverifier();

	int init(char *model_path, char *mean_path, char *std_path);
	float do_one_liveness_det(float *fea, float &conf, int fead);
    int get_nr_feature();
	float *get_weights();
private:
	LibLinear *linear;
	//Cdescriber *pCdescriber;

	bool _gotModels;
	float *_pFeat, *_pMean, *_pStd;
	int _dim_feat;

	parameter param;
};
