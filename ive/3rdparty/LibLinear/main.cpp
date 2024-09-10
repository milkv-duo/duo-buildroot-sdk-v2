// Train and test liblinear model in C++ format.
//
#include <iostream>
#include "LibLinear.hpp"
using namespace std;

int main(int argc, char* argv[])
{
    // Set up training data
    float labels[12] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    -1.0, -1.0, -1.0, -1.0, -1.0, -1.0};
    Mat labelsMat(12, 1, CV_32FC1, labels);

    float trainingData[12][2] = { { 1, 1 }, { 3, 1 }, { 1, 3 },
        { 1, -1 }, { 3, -1 }, { 1, -3 }, { -1, 1 }, { -3, 1 }, { -1, 3 },
        { -1, -1 }, { -3, -1 }, { -1, -3 }};
    Mat trainingDataMat(12, 2, CV_32FC1, trainingData);

    float testData[4][2] = { { 4, 4 }, {4, -4}, {-4, 4}, {-4, -4} };
    Mat testDataMat(4, 2, CV_32FC1, testData);

    // train model
    // there are 8 solvers(0~7) to choose in paramter.
    // choose
    LibLinear *linear = new LibLinear;
    parameter param = LinearParam::construct_param(0);
    // data as Mat
    linear->train(trainingDataMat, labelsMat, param);
    // data as float*
    linear->train(trainingDataMat.ptr<float>(), labelsMat.ptr<float>(),
                  12, 2, param);
    // user should take care of the paramter deconstructor.
    LinearParam::destroy_param(&param);

    // save and load model
    string model_file = "train.model";
    linear->save_model(model_file);
    linear->load_model(model_file);

    // predict one sample each call
    for (int i = 0; i < testDataMat.rows; i++){
        Mat sampleMat = testDataMat.row(i);
        double out = linear->predict(sampleMat);
        cout<<out<<" ";
    }
    cout<<endl;

    // predict all sample in one call
    Mat outputMat;
    linear->predict(testDataMat, outputMat);
    cout<<outputMat<<endl;

    // predict Sigma(W*x) or the distance from hyperplane.
    for (int i = 0; i < testDataMat.rows; i++){
        Mat sampleMat = testDataMat.row(i);
        Mat valueMat;
        double out = linear->predict_values(sampleMat, valueMat);
        cout<<out<<" "<<endl;
        cout<<valueMat<<endl;
        cout<<"<<<<<<<<<<<<<<<<<\n";
        float value = 0;
        out = linear->predict_values(sampleMat.ptr<float>(), testDataMat.cols,
                              &value);
        cout<<out<<" "<<endl;
        cout<<value<<endl;
        cout<<">>>>>>>>>>>>>>>>>\n";
    }

    // predict probability of one sample
    for (int i = 0; i < testDataMat.rows; i++){
        Mat sampleMat = testDataMat.row(i);
        Mat probMat;
        double out = linear->predict_probabilities(sampleMat, probMat);
        cout<<out<<" "<<endl;
        cout<<probMat<<endl;
    }

    linear->release();
    delete linear;

    return 0;
}