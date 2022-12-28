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

#include "camera_engine.h"
#include "utils/native_debug.h"

int test1 = 0;


/*
 * SampleEngine global object
 */
static CameraEngine* pEngineObj = nullptr;
CameraEngine* GetAppEngine(void) {
  ASSERT(pEngineObj, "AppEngine has not initialized");
  return pEngineObj;
}

/**
 * Teamplate function for NativeActivity derived applications
 *   Create/Delete camera object with
 *   INIT_WINDOW/TERM_WINDOW command, ignoring other event.
 */
static void ProcessAndroidCmd(struct android_app* app, int32_t cmd) {
  CameraEngine* engine = reinterpret_cast<CameraEngine*>(app->userData);
  switch (cmd) {
    case APP_CMD_INIT_WINDOW:
//      if (engine->AndroidApp()->window != NULL) {
//        engine->OnAppInitWindow();
//      }
      engine->OnAppInitWindow();
      break;
    case APP_CMD_TERM_WINDOW:
      engine->OnAppTermWindow();
      break;
    case APP_CMD_CONFIG_CHANGED:
      engine->OnAppConfigChange();
      break;
    case APP_CMD_LOST_FOCUS:
      break;
  }
}

extern "C" void android_main(struct android_app* state) {
  CameraEngine engine(state);
//  CameraEngine engine(nullptr);
  pEngineObj = &engine;

  state->userData = reinterpret_cast<void*>(&engine);
  state->onAppCmd = ProcessAndroidCmd;

  pEngineObj->OnAppInitWindow();

  while(1);

//  while(!test1)
//    pEngineObj->OnAppInitWindow();

  // loop waiting for stuff to do.
//  while (1) {
//    // Read all pending events.
//    int events;
//    struct android_poll_source* source;
//
//    while (ALooper_pollAll(0, NULL, &events, (void**)&source) >= 0) {
//      // Process this event.
//      if (source != NULL) {
//        source->process(state, source);
//      }
//
//      // Check if we are exiting.
//      if (state->destroyRequested != 0) {
//        LOGI("CameraEngine thread destroy requested!");
//        engine.DeleteCamera();
//        pEngineObj = nullptr;
//        return;
//      }
//    }
//  }
}

/**
 * Handle Android System APP_CMD_INIT_WINDOW message
 *   Request camera persmission from Java side
 *   Create camera object if camera has been granted
 */
void CameraEngine::OnAppInitWindow(void) {
//  if (!cameraGranted_) {
//    // Not permitted to use camera yet, ask(again) and defer other events
//    RequestCameraPermission();
//    return;
//  }

  CreateCamera();
  ASSERT(camera_, "CameraCreation Failed");

  EnableUI();
}

/**
 * Handle APP_CMD_TEMR_WINDOW
 */
void CameraEngine::OnAppTermWindow(void) {
  DeleteCamera();
}

/**
 * Handle APP_CMD_CONFIG_CHANGED
 */
void CameraEngine::OnAppConfigChange(void) {

}
