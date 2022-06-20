#include <jni.h>
#include <opencv2/opencv.hpp>
#include <jni.h>

using namespace cv;

static Mat downscaled;
static Mat laplacian;
static float eps = 0.00000001;

Mat ND(double original_row, double original_col, jdouble lambda, jint itr, jdouble g, Mat L0,
       jdouble k);

extern "C"
JNIEXPORT void JNICALL
Java_com_smj_mobile_1lpnd_1native_MainActivity_LPND(JNIEnv *env, jobject thiz, jlong mat_addr_input, jlong mat_addr_result,
                                                    jint itr, jdouble lambda, jdouble g,
                                                    jdoubleArray k) {
    Mat I2, I3, I4, I5;
    Mat I2_re, I3_re, I4_re, I5_re;
    Mat L1, L2, L3, L4;
    Mat L1_d, L2_d, L3_d, L4_d;
    Mat L2_d_re, L3_d_re, L4_d_re;

    Mat &matInput = *(Mat*)mat_addr_input;
    jdouble* kArray  = env->GetDoubleArrayElements(k, nullptr);
    Size original_size = Size(matInput.cols, matInput.rows);

    Mat result = Mat(original_size, matInput.type());

    Mat ones = Mat::ones(original_size, matInput.type());
    add(matInput, ones, result);
    log(result, result);

    // first pyramid layer
    pyrDown(result, I2);
    pyrUp(I2, I2_re, original_size);
    subtract(result, I2_re, L1);

    // second pyramid layer
    pyrDown(I2, I3);
    pyrUp(I3, I3_re, Size(I2.cols, I2.rows));
    subtract(I2, I3_re, L2);

    // third pyramid layer
    pyrDown(I3, I4);
    pyrUp(I4, I4_re, Size(I3.cols, I3.rows));
    subtract(I3, I4_re, L3);

    // last pyramid layer
    pyrDown(I4, I5);
    pyrUp(I5, I5_re, Size(I4.cols, I4.rows));
    subtract(I4, I5_re, L4);

    // diffusion thread
    L1_d = ND(L1.rows, L1.cols, lambda, itr, g, L1, kArray[0]);

    L2_d = ND(L2.rows, L2.cols, lambda, itr, g, L2, kArray[1]);
    resize(L2_d, L2_d_re, original_size);
    add(L1_d, L2_d_re, result);

    L3_d = ND(L3.rows, L3.cols, lambda, itr, g, L3, kArray[2]);
    resize(L3_d, L3_d_re, original_size);
    add(result, L3_d_re, result);

    L4_d = ND(L4.rows, L4.cols, lambda, itr, g, L4, kArray[3]);
    env->ReleaseDoubleArrayElements(k, kArray, 0);
    add(L4_d, I4, L4_d);
    resize(L4_d, L4_d_re, original_size);
    add(result, L4_d_re, result);
    exp(result, result);
    subtract(result, ones, result);

    *(Mat*)mat_addr_result = result;

}


Mat In, pow_In, cn, n;
Mat Is, pow_Is, cs, s;
Mat Iw, pow_Iw, cw, w;
Mat Ie, pow_Ie, ce, e;

Mat ND(double original_row, double original_col, jdouble lambda, jint itr, jdouble g, Mat L0,
       jdouble k) {

    int Gau_size = (int) (2*ceil(3*g) + 1);
    Mat filtered = Mat(Size(original_col, original_row), CV_32FC1);
    Mat L0_g_n = Mat(Size(original_col, original_row), CV_32FC1);
    Mat L0_g_s = Mat(Size(original_col, original_row), CV_32FC1);
    Mat L0_g_w = Mat(Size(original_col, original_row), CV_32FC1);
    Mat L0_g_e = Mat(Size(original_col, original_row), CV_32FC1);
    Mat k_mat = Mat(Size(original_col, original_row), CV_32FC1, Scalar(2*k*k));
    Mat lambda_mat = Mat(Size(original_col, original_row), CV_32FC1);
    Mat eps_mat = Mat(Size(original_col, original_row), CV_32FC1, Scalar(eps));
    Mat L0_out = L0.clone();

    for (int t = 0; t < itr; t++) {

        GaussianBlur(L0_out, filtered, Size(Gau_size, Gau_size), g);

        vconcat(filtered.row(0), filtered(Rect(0, 0, original_col, original_row-1)), L0_g_n);
        vconcat(filtered.row(original_row-1), filtered(Rect(0, 1, original_col, original_row-1)), L0_g_s);
        hconcat(filtered.col(0), filtered(Rect(0, 0, original_col-1, original_row)), L0_g_w);
        hconcat(filtered.col(original_col-1), filtered(Rect(1, 0, original_col-1, original_row)), L0_g_e);

        subtract(L0_g_n, filtered, In);
        subtract(L0_g_s, filtered, Is);
        subtract(L0_g_w, filtered, Iw);
        subtract(L0_g_e, filtered, Ie);

        pow(In, 2, pow_In);
        divide(pow_In, k_mat, cn);
        multiply(cn, -1, cn);
        exp(cn, cn);

        pow(Is, 2, pow_Is);
        divide(pow_Is, k_mat, cs);
        multiply(cs, -1, cs);
        exp(cs, cs);

        pow(Iw, 2, pow_Iw);
        divide(pow_Iw, k_mat, cw);
        multiply(cw, -1, cw);
        exp(cw, cw);

        pow(Ie, 2, pow_Ie);
        divide(pow_Ie, k_mat, ce);
        multiply(ce, -1, ce);
        exp(ce, ce);

        multiply(cn, In, n);
        multiply(cs, Is, s);
        multiply(cw, Iw, w);
        multiply(ce, Ie, e);

        addWeighted(L0_out, 1.0, (n+s+w+e), lambda, 0, L0_out);
    }
    return L0_out;
}

Mat InphaseMag;
Mat QuadratureMag;
Mat BmodeMag;
Mat BmodeMaglog;
Mat finalData;

int dynamic_range = 60;
double logMinValue, logMaxValue, magMinValue, magMaxValue;

//typedef double jdouble;

double test_data[1024][32];

extern "C"
JNIEXPORT void JNICALL Java_com_smj_mobile_1lpnd_1native_MainActivity_nativeImaging
        (JNIEnv *env, jobject thiz, jdoubleArray doubleInphaseArray, jdoubleArray doubleQuadratureArray, jlong result_data) {

    jdouble * i_doubleBuffer;
    i_doubleBuffer = env -> GetDoubleArrayElements(doubleInphaseArray, NULL);
    jsize i_arrayLength = env ->GetArrayLength(doubleInphaseArray);

    jdouble * q_doubleBuffer;
    q_doubleBuffer = env -> GetDoubleArrayElements(doubleQuadratureArray, NULL);
    jsize q_arrayLength = env ->GetArrayLength(doubleQuadratureArray);

    Mat InphaseArrayToMat(32, 1024, CV_64F, i_doubleBuffer);
    transpose(InphaseArrayToMat, InphaseArrayToMat);

    Mat QuadratureArrayToMat(32, 1024, CV_64F, q_doubleBuffer);
    transpose(QuadratureArrayToMat, QuadratureArrayToMat);

    pow( InphaseArrayToMat, 2, InphaseMag);
    pow(QuadratureArrayToMat, 2, QuadratureMag);
    add(InphaseMag, QuadratureMag, BmodeMag);
    sqrt(BmodeMag, BmodeMag);

    minMaxLoc(BmodeMag, &magMinValue, &magMaxValue);
    divide(BmodeMag, magMaxValue, BmodeMag);
    log(BmodeMag, BmodeMaglog);
    multiply(BmodeMaglog, 10, BmodeMaglog) ;
    add(BmodeMaglog, dynamic_range, BmodeMaglog);

    max(BmodeMaglog, 0, finalData);
    minMaxLoc(finalData, &logMinValue, &logMaxValue);
    divide(finalData, logMaxValue, finalData);
    multiply(finalData, 255, finalData);

    *(Mat*)result_data = finalData;

    env->ReleaseDoubleArrayElements(doubleInphaseArray, i_doubleBuffer, 0);
    env->ReleaseDoubleArrayElements(doubleQuadratureArray, q_doubleBuffer, 0);
}
