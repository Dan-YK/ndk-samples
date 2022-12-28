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
#include "image_reader.h"

#include <dirent.h>

#include <cstdlib>
#include <ctime>
#include <functional>
#include <string>
#include <thread>

#include "utils/native_debug.h"

/*
 * For JPEG capture, captured files are saved under
 *     DirName
 * File names are incrementally appended an index number as
 *     capture0.jpg, capture1.jpg, capture2.jpg
 */
static const char *kDirName = "/sdcard/DCIM/Camera/";
static const char *kFileName = "capture";

/**
 * MAX_BUF_COUNT:
 *   Max buffers in this ImageReader.
 */
#define MAX_BUF_COUNT 4

/**
 * ImageReader listener: called by AImageReader for every frame captured
 * We pass the event to ImageReader class, so it could do some housekeeping
 * about
 * the loaded queue. For example, we could keep a counter to track how many
 * buffers are full and idle in the queue. If camera almost has no buffer to
 * capture
 * we could release ( skip ) some frames by AImageReader_getNextImage() and
 * AImageReader_delete().
 */
void OnImageCallback(void *ctx, AImageReader *reader) {
  reinterpret_cast<ImageReader *>(ctx)->ImageCallback(reader);
}

/**
 * Constructor
 */
ImageReader::ImageReader(ImageFormat *res, enum AIMAGE_FORMATS format)
    : presentRotation_(0), reader_(nullptr) {
  callback_ = nullptr;
  callbackCtx_ = nullptr;

  media_status_t status = AImageReader_new(480, 640, format,
                                           MAX_BUF_COUNT, &reader_);
  ASSERT(reader_ && status == AMEDIA_OK, "Failed to create AImageReader");

  AImageReader_ImageListener listener{
      .context = this,
      .onImageAvailable = OnImageCallback,
  };
  AImageReader_setImageListener(reader_, &listener);
}

ImageReader::~ImageReader() {
  ASSERT(reader_, "NULL Pointer to %s", __FUNCTION__);
  AImageReader_delete(reader_);
}

void ImageReader::RegisterCallback(
    void *ctx, std::function<void(void *ctx, const char *fileName)> func) {
  callbackCtx_ = ctx;
  callback_ = func;
}

void ImageReader::ImageCallback(AImageReader *reader) {
  int32_t format;
  media_status_t status = AImageReader_getFormat(reader, &format);
  ASSERT(status == AMEDIA_OK, "Failed to get the media format");
  if (format == AIMAGE_FORMAT_JPEG) {
    AImage *image = nullptr;
    media_status_t status = AImageReader_acquireNextImage(reader, &image);
    ASSERT(status == AMEDIA_OK && image, "Image is not available");

    // Create a thread and write out the jpeg files
    std::thread writeFileHandler(&ImageReader::WriteFile, this, image);
    writeFileHandler.detach();
  }
}

ANativeWindow *ImageReader::GetNativeWindow(void) {
  if (!reader_) return nullptr;
  ANativeWindow *nativeWindow;
  media_status_t status = AImageReader_getWindow(reader_, &nativeWindow);
  ASSERT(status == AMEDIA_OK, "Could not get ANativeWindow");

  return nativeWindow;
}

/**
 * GetNextImage()
 *   Retrieve the next image in ImageReader's bufferQueue, NOT the last image so
 * no image is skipped. Recommended for batch/background processing.
 */
AImage *ImageReader::GetNextImage(void) {
  AImage *image;
  media_status_t status = AImageReader_acquireNextImage(reader_, &image);
  if (status != AMEDIA_OK) {
    return nullptr;
  }
  return image;
}

/**
 * GetLatestImage()
 *   Retrieve the last image in ImageReader's bufferQueue, deleting images in
 * in front of it on the queue. Recommended for real-time processing.
 */
AImage *ImageReader::GetLatestImage(void) {
  AImage *image;
  media_status_t status = AImageReader_acquireLatestImage(reader_, &image);
  if (status != AMEDIA_OK) {
    return nullptr;
  }
  return image;
}

/**
 * Delete Image
 * @param image {@link AImage} instance to be deleted
 */
void ImageReader::DeleteImage(AImage *image) {
  if (image) AImage_delete(image);
}

#ifndef MAX
#define MAX(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })
#define MIN(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })
#endif

/**
 * Write out jpeg files to kDirName directory
 * @param image point capture jpg image
 */
void ImageReader::WriteFile(AImage *image) {
  int planeCount;
  media_status_t status = AImage_getNumberOfPlanes(image, &planeCount);
  ASSERT(status == AMEDIA_OK && planeCount == 1,
         "Error: getNumberOfPlanes() planeCount = %d", planeCount);
  uint8_t *data = nullptr;
  int len = 0;
  AImage_getPlaneData(image, 0, &data, &len);

  DIR *dir = opendir(kDirName);
  if (dir) {
    closedir(dir);
  } else {
    std::string cmd = "mkdir -p ";
    cmd += kDirName;
    system(cmd.c_str());
  }

  struct timespec ts {
    0, 0
  };
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm localTime;
  localtime_r(&ts.tv_sec, &localTime);

  std::string fileName = kDirName;
  std::string dash("-");
  fileName += kFileName + std::to_string(localTime.tm_mon) +
              std::to_string(localTime.tm_mday) + dash +
              std::to_string(localTime.tm_hour) +
              std::to_string(localTime.tm_min) +
              std::to_string(localTime.tm_sec) + ".jpg";
  FILE *file = fopen(fileName.c_str(), "wb");
  if (file && data && len) {
    fwrite(data, 1, len, file);
    fclose(file);

    if (callback_) {
      callback_(callbackCtx_, fileName.c_str());
    }
  } else {
    if (file) fclose(file);
  }
  AImage_delete(image);
}
