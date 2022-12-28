/**
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

/** Description
 *   Demonstrate NDK Camera interface added to android-24
 */

#include "camera_engine.h"

#include <cstdio>

#include "utils/native_debug.h"

/**
 * constructor and destructor for main application class
 * @param app native_app_glue environment
 * @return none
 */
CameraEngine::CameraEngine(android_app* app)
    : app_(app),
      cameraGranted_(false),
      rotation_(0),
      cameraReady_(false),
      camera_(nullptr),
      yuvReader_(nullptr),
      jpgReader_(nullptr) {
  memset(&savedNativeWinRes_, 0, sizeof(savedNativeWinRes_));
}

CameraEngine::~CameraEngine() {
  DeleteCamera();
}

struct android_app* CameraEngine::AndroidApp(void) const {
  return app_;
}

/**
 * Create a camera object for onboard BACK_FACING camera
 */
void CameraEngine::CreateCamera(void) {
  // Camera needed to be requested at the run-time from Java SDK
  // if Not granted, do nothing.
//  if (!cameraGranted_ || !app_->window) {
//    LOGW("Camera Sample requires Full Camera access");
//    return;
//  }

  camera_ = new NDKCamera();
  ASSERT(camera_, "Failed to Create CameraObject");

  jpgReader_ = new ImageReader(nullptr, AIMAGE_FORMAT_JPEG);
  jpgReader_->RegisterCallback(
      this, [this](void* ctx, const char* str) -> void {
        reinterpret_cast<CameraEngine*>(ctx)->OnPhotoTaken(str);
      });

  // now we could create session
  camera_->CreateSession(nullptr,
                         jpgReader_->GetNativeWindow(), 0);
}

void CameraEngine::DeleteCamera(void) {
  cameraReady_ = false;
  if (camera_) {
    delete camera_;
    camera_ = nullptr;
  }
  if (yuvReader_) {
    delete yuvReader_;
    yuvReader_ = nullptr;
  }
  if (jpgReader_) {
    delete jpgReader_;
    jpgReader_ = nullptr;
  }
}

/**
 * Initiate a Camera Run-time usage request to Java side implementation
 *  [ The request result will be passed back in function
 *    notifyCameraPermission()]
 */
void CameraEngine::RequestCameraPermission() {
  if (!app_) return;

  JNIEnv* env;
  ANativeActivity* activity = app_->activity;
  activity->vm->GetEnv((void**)&env, JNI_VERSION_1_6);

  activity->vm->AttachCurrentThread(&env, NULL);

  jobject activityObj = env->NewGlobalRef(activity->clazz);
  jclass clz = env->GetObjectClass(activityObj);
  env->CallVoidMethod(activityObj,
                      env->GetMethodID(clz, "RequestCamera", "()V"));
  env->DeleteGlobalRef(activityObj);

  activity->vm->DetachCurrentThread();
}