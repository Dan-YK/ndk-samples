/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.sample.camera.basic;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.NativeActivity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import static android.hardware.camera2.CameraMetadata.LENS_FACING_BACK;

public class CameraActivity extends NativeActivity
        implements ActivityCompat.OnRequestPermissionsResultCallback {

    private final String DBG_TAG = "NDK-CAMERA-BASIC";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(DBG_TAG, "OnCreate()");
        RequestCamera();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    private static final int PERMISSION_REQUEST_CODE_CAMERA = 1;
    public void RequestCamera() {
        String[] accessPermissions = new String[] {
            Manifest.permission.CAMERA
        };
        boolean needRequire  = false;
        for(String access : accessPermissions) {
           int curPermission = ActivityCompat.checkSelfPermission(this, access);
           if(curPermission != PackageManager.PERMISSION_GRANTED) {
               needRequire = true;
               break;
           }
        }
        if (needRequire) {
            ActivityCompat.requestPermissions(
                    this,
                    accessPermissions,
                    PERMISSION_REQUEST_CODE_CAMERA);
            return;
        }
        notifyCameraPermission(true);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        /*
         * if any permission failed, the sample could not play
         */
        if (PERMISSION_REQUEST_CODE_CAMERA != requestCode) {
            super.onRequestPermissionsResult(requestCode,
                                             permissions,
                                             grantResults);
            return;
        }

        if(grantResults.length == 1) {
            notifyCameraPermission(grantResults[0] == PackageManager.PERMISSION_GRANTED);
        }
    }

    /**
     * params[] exposure and sensitivity init values in (min, max, curVa) tuple
     *   0: exposure min
     *   1: exposure max
     *   2: exposure val
     *   3: sensitivity min
     *   4: sensitivity max
     *   5: sensitivity val
     */
    @SuppressLint("InflateParams")
    public void EnableUI(final long[] params)
    {
        TakePhoto();
    }
    /**
      Called from Native side to notify that a photo is taken
     */
    public void OnPhotoTaken(String fileName) {
        final String name = fileName;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(getApplicationContext(),
                        "Photo saved to " + name, Toast.LENGTH_SHORT).show();
            }
        });
    }

    native static void notifyCameraPermission(boolean granted);
    native static void TakePhoto();

    static {
        System.loadLibrary("ndk_camera");
    }
}

