#pragma once

// clang-format off
#include <LittleFS.h>
#define IV_FS LittleFS
#include <M5Unified.h>
// clang-format on

class ImageViewer {
public:
    enum Orientation
    {
        OrientationNormal = 0,
        OrientationRight,
        OrientationUpsideDown,
        OrientationLeft,
    };

    static const char* VERSION;

    static const char* DEFAULT_CONFIG_NAME;
    static const char* KEY_AUTO_MODE;
    static const char* KEY_AUTO_MODE_INTERVAL;
    static const char* KEY_AUTO_MODE_RANDOMIZED;
    static const char* KEY_AUTO_ROTATION;

    static const size_t MAX_IMAGE_FILES = 50;
    static const bool DEFAULT_AUTO_MODE = false;
    static const uint32_t DEFAULT_START_INTERVAL_MS = 3000;
    static const uint32_t DEFAULT_AUTO_MODE_INTERVAL_MS = 3000;
    static const bool DEFAULT_AUTO_MODE_RANDOMIZED = false;
    static const bool DEFAULT_AUTO_ROTATION = true;

#if defined(ARDUINO_M5STACK_CARDPUTER)
    static const Orientation DEFAULT_ORIENTATION = OrientationRight;
#else
    static const Orientation DEFAULT_ORIENTATION = OrientationNormal;
#endif

    static const float GRAVITY_THRESHOLD;
    static const String ROOT_DIR;

    ImageViewer(Orientation orientation = DEFAULT_ORIENTATION,
                bool isAutoMode = DEFAULT_AUTO_MODE,
                uint32_t autoModeInterval = DEFAULT_AUTO_MODE_INTERVAL_MS,
                bool isAutoModeRandomize = DEFAULT_AUTO_MODE_RANDOMIZED,
                bool isAutoRotation = DEFAULT_AUTO_ROTATION);
    virtual ~ImageViewer(void);

    virtual bool begin(const int bgColor = TFT_WHITE);
    virtual bool update(void);

    virtual bool updateOrientation(float threshold = GRAVITY_THRESHOLD);

protected:
    virtual bool setImageFileList(const String& path = ROOT_DIR);
    virtual void showImage(const String images[], size_t p);
    virtual bool hasExt(const char* filename, const char* ext) const;
    virtual bool isJpeg(const char* filename) const;
    virtual bool isPng(const char* filename) const;
    virtual bool isBmp(const char* filename) const;
    virtual bool isImageFile(const File& f) const;
    virtual Orientation detectOrientation(float threshold);
    virtual bool parse(const char* config = DEFAULT_CONFIG_NAME);

private:
    Orientation _orientation;
    bool _isAutoMode;
    uint32_t _autoModeInterval;
    bool _isAutoModeRandomized;
    bool _isAutoRotation;

    String _imageFiles[MAX_IMAGE_FILES];
    size_t _nImageFiles;
    size_t _pos;
    uint32_t _prevUpdate;
    uint32_t _interval;
};