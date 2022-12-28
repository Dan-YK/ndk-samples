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
 * See the License for the spRequestCameraPermissionecific language governing permissions and
 * limitations under the License.
 */
#include "camera_manager.h"

#include <camera/NdkCameraManager.h>
#include <unistd.h>

#include <cinttypes>
#include <queue>
#include <utility>

#include "utils/camera_utils.h"
#include "utils/native_debug.h"

/**
 * Range of Camera Exposure Time:
 *     Camera's capability range have a very long range which may be disturbing
 *     on camera. For this sample purpose, clamp to a range showing visible
 *     video on preview: 100000ns ~ 250000000ns
 */

NDKCamera::NDKCamera()
    : cameraMgr_(nullptr),
      activeCameraId_(""),
      cameraFacing_(ACAMERA_LENS_FACING_BACK),
      cameraOrientation_(0),
      outputContainer_(nullptr),
      captureSessionState_(CaptureSessionState::MAX_STATE),
      exposureTime_(static_cast<int64_t>(0)) {
  valid_ = false;
  requests_.resize(CAPTURE_REQUEST_COUNT);
  memset(requests_.data(), 0, requests_.size() * sizeof(requests_[0]));
  cameras_.clear();
  cameraMgr_ = ACameraManager_create();
  ASSERT(cameraMgr_, "Failed to create cameraManager");

  // Pick up a back-facing camera to preview
  EnumerateCamera();
  ASSERT(activeCameraId_.size(), "Unknown ActiveCameraIdx");

  // Create back facing camera device
  CALL_MGR(openCamera(cameraMgr_, activeCameraId_.c_str(), GetDeviceListener(),
                      &cameras_[activeCameraId_].device_));

  CALL_MGR(registerAvailabilityCallback(cameraMgr_, GetManagerListener()));

  valid_ = true;
}

void NDKCamera::CreateSession(ANativeWindow* previewWindow,
                              ANativeWindow* jpgWindow, int32_t imageRotation) {
  // Create output from this app's ANativeWindow, and add into output container
  requests_[JPG_CAPTURE_REQUEST_IDX].outputNativeWindow_ = jpgWindow;
  requests_[JPG_CAPTURE_REQUEST_IDX].template_ = TEMPLATE_STILL_CAPTURE;

  CALL_CONTAINER(create(&outputContainer_));
  for (auto& req : requests_) {
    ANativeWindow_acquire(req.outputNativeWindow_);
    CALL_OUTPUT(create(req.outputNativeWindow_, &req.sessionOutput_));
    CALL_CONTAINER(add(outputContainer_, req.sessionOutput_));
    CALL_TARGET(create(req.outputNativeWindow_, &req.target_));
    CALL_DEV(createCaptureRequest(cameras_[activeCameraId_].device_,
                                  req.template_, &req.request_));
    CALL_REQUEST(addTarget(req.request_, req.target_));
  }

  // Create a capture session for the given preview request
  captureSessionState_ = CaptureSessionState::READY;
  CALL_DEV(createCaptureSession(cameras_[activeCameraId_].device_,
                                outputContainer_, GetSessionListener(),
                                &captureSession_));

  ACaptureRequest_setEntry_i32(requests_[JPG_CAPTURE_REQUEST_IDX].request_,
                               ACAMERA_JPEG_ORIENTATION, 1, &imageRotation);
}

NDKCamera::~NDKCamera() {
  valid_ = false;
  // stop session if it is on:
  if (captureSessionState_ == CaptureSessionState::ACTIVE) {
    ACameraCaptureSession_stopRepeating(captureSession_);
  }
  ACameraCaptureSession_close(captureSession_);

  for (auto& req : requests_) {
    CALL_REQUEST(removeTarget(req.request_, req.target_));
    ACaptureRequest_free(req.request_);
    ACameraOutputTarget_free(req.target_);

    CALL_CONTAINER(remove(outputContainer_, req.sessionOutput_));
    ACaptureSessionOutput_free(req.sessionOutput_);

    ANativeWindow_release(req.outputNativeWindow_);
  }

  requests_.resize(0);
  ACaptureSessionOutputContainer_free(outputContainer_);

  for (auto& cam : cameras_) {
    if (cam.second.device_) {
      CALL_DEV(close(cam.second.device_));
    }
  }
  cameras_.clear();
  if (cameraMgr_) {
    CALL_MGR(unregisterAvailabilityCallback(cameraMgr_, GetManagerListener()));
    ACameraManager_delete(cameraMgr_);
    cameraMgr_ = nullptr;
  }
}

/**
 * EnumerateCamera()
 *     Loop through cameras on the system, pick up
 *     1) back facing one if available
 *     2) otherwise pick the first one reported to us
 */
void NDKCamera::EnumerateCamera() {
  ACameraIdList* cameraIds = nullptr;
  CALL_MGR(getCameraIdList(cameraMgr_, &cameraIds));

  for (int i = 0; i < cameraIds->numCameras; ++i) {
    const char* id = cameraIds->cameraIds[i];

    ACameraMetadata* metadataObj;
    CALL_MGR(getCameraCharacteristics(cameraMgr_, id, &metadataObj));

    int32_t count = 0;
    const uint32_t* tags = nullptr;
    ACameraMetadata_getAllTags(metadataObj, &count, &tags);
    for (int tagIdx = 0; tagIdx < count; ++tagIdx) {
      if (ACAMERA_LENS_FACING == tags[tagIdx]) {
        ACameraMetadata_const_entry lensInfo = {
            0,
        };
        CALL_METADATA(getConstEntry(metadataObj, tags[tagIdx], &lensInfo));
        CameraId cam(id);
        cam.facing_ = static_cast<acamera_metadata_enum_android_lens_facing_t>(
            lensInfo.data.u8[0]);
        cam.owner_ = false;
        cam.device_ = nullptr;
        cameras_[cam.id_] = cam;
        if (cam.facing_ == ACAMERA_LENS_FACING_BACK) {
          activeCameraId_ = cam.id_;
        }
        break;
      }
    }
    ACameraMetadata_free(metadataObj);
  }

  ASSERT(cameras_.size(), "No Camera Available on the device");
  if (activeCameraId_.length() == 0) {
    // if no back facing camera found, pick up the first one to use...
    activeCameraId_ = cameras_.begin()->second.id_;
  }
  ACameraManager_deleteCameraIdList(cameraIds);
}

bool NDKCamera::TakePhoto(void) {
  if (captureSessionState_ == CaptureSessionState::ACTIVE) {
    ACameraCaptureSession_stopRepeating(captureSession_);
  }

  CALL_SESSION(capture(captureSession_, GetCaptureCallback(), 1,
                       &requests_[JPG_CAPTURE_REQUEST_IDX].request_,
                       &requests_[JPG_CAPTURE_REQUEST_IDX].sessionSequenceId_));
  return true;
}