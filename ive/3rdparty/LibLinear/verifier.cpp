
#include "verifier.h"

Cverifier::Cverifier()
{
	//pCdescriber = nullptr;
	_pFeat = nullptr;
	_pMean = nullptr;
	_pStd = nullptr;
	_gotModels = false;

	linear = nullptr;// = new LibLinear;
}
Cverifier::~Cverifier()
{
	//if (pCdescriber!=nullptr) { delete pCdescriber; pCdescriber = nullptr;}
	if (_pFeat != nullptr) { delete _pFeat; _pFeat = nullptr; }
	if (_pMean != nullptr) { delete _pMean; _pMean = nullptr; }
	if (_pStd != nullptr) { delete _pStd; _pStd = nullptr; }

	if (linear != nullptr) { delete linear; linear = nullptr; }
}
int Cverifier::get_nr_feature()
{
	return linear->_nr_feature;
}

float *Cverifier::get_weights()
{
	return linear->_model_weight;
}
int Cverifier::init(char *model_path, char *mean_path, char *std_path)
{
	if (!_gotModels) {
		//pCdescriber = new Cdescriber();
		//int radius = 9;
		//int neighbors = 8;
		//int channels = 3; // ori ch=3
		//_dim_feat = pCdescriber->get_dim_feat(radius, neighbors, channels);

		//_dim_feat *= 2;

		_dim_feat = 3780;
		//svm = SVM::load(model_path);
		linear = new LibLinear;
		linear->load_model(model_path);
#if 0
		_pMean = new float[444];
		_pStd = new float[444];

		FILE *fp = fopen("../train_mean.txt", "r");
		for (int i = 0; i < 444; i++) {
			fscanf(fp, "%f", &_pMean[i]);
		}
		fclose(fp);
		fp = fopen("../train_mean.dat", "wb");
		fwrite(&_dim_feat, sizeof(int), 1, fp);
		fwrite(_pMean, sizeof(float), _dim_feat, fp);
		fclose(fp);

		fp = fopen("../train_var.txt", "r");
		for (int i = 0; i < 444; i++) {
			fscanf(fp, "%f", &_pStd[i]);
			_pStd[i] = sqrt(_pStd[i]);
		}
		fclose(fp);
		fp = fopen("../train_std.dat", "wb");
		fwrite(&_dim_feat, sizeof(int), 1, fp);
		fwrite(_pStd, sizeof(float), _dim_feat, fp);
		fclose(fp);
#else


		//int t;
#if 0
		FILE *fp = fopen(mean_path, "rb");
		fread(&t, sizeof(int), 1, fp);

		if (t != _dim_feat) {
			fclose(fp);
			printf("Feature dimension is incorrect.");
			return (-4);
		}
		_pMean = new float[_dim_feat];
		fread(_pMean, sizeof(float), _dim_feat, fp);
		fclose(fp);
		fp = fopen(std_path, "rb");
		fread(&t, sizeof(int), 1, fp);
		if (t != _dim_feat) {
			fclose(fp);
			printf("Feature dimension is incorrect.");
			return (-5);
		}
		_pStd = new float[_dim_feat];
		fread(_pStd, sizeof(float), _dim_feat, fp);
		fclose(fp);
#endif
        _pMean = new float[_dim_feat];
		_pStd = new float[_dim_feat];
		memset(_pMean, 0, sizeof(float)*_dim_feat );
        memset(_pStd, 0, sizeof(float)*_dim_feat );
#endif

		_gotModels = true;
	}
	else {
		return (-6);
	}
	return 0;
}


float Cverifier::do_one_liveness_det(float *fea, float &conf, int fead)
{
	//double response = 0.0;//linear->predict(test_feat);
	double tt = linear->predict_values( fea, fead, &conf );
	//printf("%lf, %lf\n", tt, conf);

	return (float)tt;//response;
}

int quality_checker(unsigned char *pgray, int w, int h) {

	int sum_values = 0;
	//int nbr_pix = 0;
	int wxh = w*h;

	for (int i = 0; i < wxh; i++) {
		if (pgray[i] < 30) {
			sum_values++;
		}
	}
	return (100 * sum_values / (wxh));
}

