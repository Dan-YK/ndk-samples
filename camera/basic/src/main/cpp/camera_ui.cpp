
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

#include <utils/native_debug.h>

#include "camera_engine.h"

const int kInitDataLen = 6;
void CameraEngine::EnableUI(void) {
  JNIEnv *jni;
  app_->activity->vm->AttachCurrentThread(&jni, NULL);
  int64_t range[3];

  // Default class retrieval
  jclass clazz = jni->GetObjectClass(app_->activity->clazz);
  jmethodID methodID = jni->GetMethodID(clazz, "EnableUI", "([J)V");
  jlongArray initData = jni->NewLongArray(kInitDataLen);

  ASSERT(initData && methodID, "JavaUI interface Object failed(%p, %p)",
         methodID, initData);

  jni->SetLongArrayRegion(initData, 3, 3, range);
  jni->CallVoidMethod(app_->activity->clazz, methodID, initData);
  app_->activity->vm->DetachCurrentThread();
}

/**
 * Handles UI request to take a photo into
 *   /sdcard/DCIM/Camera
 */
void CameraEngine::OnTakePhoto() {
  if (camera_) {
    camera_->TakePhoto();
  }
}

void CameraEngine::OnPhotoTaken(const char *fileName) {
  JNIEnv *jni;
  app_->activity->vm->AttachCurrentThread(&jni, NULL);

  // Default class retrieval
  jclass clazz = jni->GetObjectClass(app_->activity->clazz);
  jmethodID methodID =
      jni->GetMethodID(clazz, "OnPhotoTaken", "(Ljava/lang/String;)V");
  jstring javaName = jni->NewStringUTF(fileName);

  jni->CallVoidMethod(app_->activity->clazz, methodID, javaName);
  app_->activity->vm->DetachCurrentThread();
}
/**
 * Process user camera and disk writing permission
 * Resume application initialization after user granted camera and disk usage
 * If user denied permission, do nothing: no camera
 *
 * @param granted user's authorization for camera and disk usage.
 * @return none
 */
void CameraEngine::OnCameraPermission(jboolean granted) {
  cameraGranted_ = (granted != JNI_FALSE);

  if (cameraGranted_) {
    OnAppInitWindow();
  }
}

/**
 *  A couple UI handles ( from UI )
 *      user camera and disk permission
 *      exposure and sensitivity SeekBars
 *      takePhoto button
 */
extern "C" JNIEXPORT void JNICALL
Java_com_sample_camera_basic_CameraActivity_notifyCameraPermission(
    JNIEnv *env, jclass type, jboolean permission) {
  std::thread permissionHandler(&CameraEngine::OnCameraPermission,
                                GetAppEngine(), permission);
  permissionHandler.detach();
}

extern "C" JNIEXPORT void JNICALL
Java_com_sample_camera_basic_CameraActivity_TakePhoto(JNIEnv *env,
                                                      jclass type) {
  std::thread takePhotoHandler(&CameraEngine::OnTakePhoto, GetAppEngine());
  takePhotoHandler.detach();
}