package com.smj.mobile_lpnd_native;

import static android.content.ContentValues.TAG;
import static org.opencv.core.CvType.CV_32FC1;
import static org.opencv.core.CvType.CV_32FC3;
import static org.opencv.core.CvType.CV_64F;
import static org.opencv.core.CvType.CV_8UC1;
import static org.opencv.imgcodecs.Imgcodecs.imwrite;
import static org.opencv.imgproc.Imgproc.COLOR_BGR2GRAY;

import android.Manifest;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.ImageView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import com.gun0912.tedpermission.*;
import com.gun0912.tedpermission.normal.TedPermission;
import com.smj.mobile_lpnd_native.databinding.ActivityMainBinding;

import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.Mat;
import org.opencv.imgproc.Imgproc;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'mobile_lpnd_native' library on application startup.
    static {
        System.loadLibrary("mobile_lpnd_native");
    }

    private ActivityMainBinding binding;

    /*TEST Dpp Data*/
    //GetData;
    public InputStream inputStreamInphase, inputStreamQuadrature;
    public byte[] getByteDataInphase = new byte[0];
    public byte[] getByteDataQuadrature = new byte[0];

    public String[] getLinesInphase, getLinesQuadrature;

    public int[] convertIntArrayInphase= new int[32768];
    public int[] convertIntArrayQuadrature = new int[32768];

    public static double[] convertDoubleArrayInphase= new double[32768];
    public static double[] convertDoubleArrayQuadrature = new double[32768];

    Mat result_data = new Mat();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ImageView img = findViewById(R.id.LPND_result_img);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        try {
            //*/ TEST DATA
            inputStreamInphase = getResources().openRawResource(R.raw.inpa);
            inputStreamQuadrature = getResources().openRawResource(R.raw.quad);
            getByteDataInphase = new byte[inputStreamInphase.available()];
            getByteDataQuadrature = new byte[inputStreamQuadrature.available()];
            inputStreamInphase.read(getByteDataInphase);
            inputStreamQuadrature.read(getByteDataQuadrature);
            getLinesInphase = (new String(getByteDataInphase)).split("\\r?\\n");
            getLinesQuadrature = (new String(getByteDataQuadrature)).split("\\r?\\n");

            for (int i = 0; i < getLinesInphase.length; i++) {
                //System.out.println("getLinesInphase " + i + " : " + getLinesInphase[i]);
                convertIntArrayInphase[i] = (int) Double.parseDouble(getLinesInphase[i]);
                convertDoubleArrayInphase[i] = Double.parseDouble(getLinesInphase[i]);
            }

            for (int i = 0; i < getLinesQuadrature.length; i++) {
                //System.out.println("getLinesQuadrature " + i + " : " + getLinesQuadrature[i]);
                convertIntArrayQuadrature[i] = (int) Double.parseDouble(getLinesQuadrature[i]);
                convertDoubleArrayQuadrature[i] = Double.parseDouble(getLinesQuadrature[i]);
            }
            nativeImaging(convertDoubleArrayInphase, convertDoubleArrayQuadrature, result_data.getNativeObjAddr());
            result_data.convertTo(result_data, CV_8UC1);
            Bitmap result_data_bitmap = Bitmap.createBitmap(result_data.cols(), result_data.rows(), Bitmap.Config.ARGB_8888);
            Utils.matToBitmap(result_data, result_data_bitmap);
            img.setImageBitmap(result_data_bitmap);
        } catch (IOException e) {
            e.printStackTrace();
        }

//        OpenCVLoader.initDebug();
//
//        new Thread(new Runnable() {
//
//            Mat original_mat = new Mat();
//            Bitmap result_bitmap;
//            ImageView original_img = findViewById(R.id.original_img);
//            ImageView LPND_result_img = findViewById(R.id.LPND_result_img);
//            BitmapDrawable drawable = (BitmapDrawable) original_img.getDrawable();
//            Bitmap originImg = drawable.getBitmap();
//            double[] k = {0.2101, 0.1007, 0.0766, 0.0626};
//
//            @Override
//            public void run() {
//                while (true) {
//                    try {
//                        Utils.bitmapToMat(originImg, original_mat);
//                        Imgproc.cvtColor(original_mat, original_mat, COLOR_BGR2GRAY);
//                        original_mat.convertTo(original_mat, CV_32FC1);
//
//                        Mat result_mat = Mat.zeros(original_mat.rows(), original_mat.cols(), original_mat.type());
//
//                        LPND(original_mat.getNativeObjAddr(), result_mat.getNativeObjAddr(),5, 0.1, 1.0, k);
//
//                        // 일단 실행시킬 때마다 Save되니까 unable
//                        // SaveImage(result_mat);
//
//                        result_mat.convertTo(result_mat, CV_8UC1);
//                        Bitmap result_bitmap = Bitmap.createBitmap(result_mat.cols(), result_mat.rows(), Bitmap.Config.ARGB_8888);
//                        Utils.matToBitmap(result_mat, result_bitmap);
//                        FPS();
//
//                        runOnUiThread(new Runnable() {
//                            public void run() {
//                                LPND_result_img.setImageBitmap(result_bitmap);
//
//                            }
//                        });
//
//                    } catch (Exception e) {
//                        Log.d(TAG, "Thread Read Exception : " + e);
//                    }
//
//                }
//            }
//        }).start();
    }

    public void SaveImage (Mat mat) {

        File file = new File(getFilesDir(), "mobile_LPND_5_01_10.tif");

        mat.convertTo(mat, CV_32FC3);

        String filename = file.toString();
        Boolean bool = imwrite(filename, mat);

        if (bool)
            Log.i(TAG, "SUCCESS writing image to external storage");
        else
            Log.i(TAG, "Fail writing image to external storage");
    }

    long fpsStartTime = 0L;             // Frame 시작 시간

    int frameCnt = 0;                   // 돌아간 Frame 갯수
    double timeElapsed = 0.0f;          // 그 동안 쌓인 시간 차이

    int result_frame_cnt = 0;

    private void FPS() {

        //시간 차이 구하기
        long fpsEndTime = System.currentTimeMillis();
        float timeDelta = (fpsEndTime - fpsStartTime) * 0.001f;

        // Frame 증가 셋팅
        frameCnt++;
        timeElapsed += timeDelta;

        // FPS를 구해서 로그로 표시
        if(frameCnt == 200){
            float fps = (float)(frameCnt/timeElapsed);
            Log.d("fps","fps : "+fps+", frameCnt : "+frameCnt);
            frameCnt = 0;
            timeElapsed = 0.0f;
        }

        // Frame 시작 시간 다시 셋팅
        fpsStartTime = System.currentTimeMillis();

    }

    /**
     * A native method that is implemented by the 'mobile_lpnd_native' native library,
     * which is packaged with this application.
     */
    public native void LPND(long mat_addr_input, long mat_addr_result, int itr, double lambda, double g, double[] k);
    public native void nativeImaging(double[] doubleInphaseArray, double[] doubleQuadratureArray, long result_data);

}