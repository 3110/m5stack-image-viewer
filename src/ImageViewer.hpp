#pragma once

#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)

// clang-format off
#if defined(IV_FS_SD)
#if defined(ARDUINO_M5STACK_TAB5)
#include <SD_MMC.h>
#define IV_FS SD_MMC
#else
#include <SD.h>
#define IV_FS SD
#endif
#else
#include <LittleFS.h>
#define IV_FS LittleFS
#endif
#include <M5Unified.h>
// clang-format on

#if defined(IV_FS_SD)
#if defined(ARDUINO_M5STACK_TAB5)
inline bool FS_BEGIN() {
    if (!IV_FS.setPins(GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_39, GPIO_NUM_40,
                       GPIO_NUM_41, GPIO_NUM_42)) {
        return false;
    }
    return IV_FS.begin();
}
#else
#define SD_FREQUENCY 15000000
inline bool FS_BEGIN() {
    SPI.begin(M5.getPin(m5::pin_name_t::sd_spi_sclk),
              M5.getPin(m5::pin_name_t::sd_spi_miso),
              M5.getPin(m5::pin_name_t::sd_spi_mosi),
              M5.getPin(m5::pin_name_t::sd_spi_ss));
    return IV_FS.begin(M5.getPin(m5::pin_name_t::sd_spi_ss), SPI, SD_FREQUENCY);
}
#endif
#else
#define FORMAT_FS_IF_FAILED true
inline bool FS_BEGIN() {
    return IV_FS.begin(FORMAT_FS_IF_FAILED);
}
#endif

#if defined(IV_FS_SD)
#define SD_MOUNT_RETRY    5
#define SD_MOUNT_DELAY_MS 500
inline bool IVS_FS_BEGIN() {
    M5.Lcd.print("Mounting SD Card ...");
    uint8_t retry = 0;
    for (retry = 0; retry < SD_MOUNT_RETRY; ++retry) {
        if (FS_BEGIN()) {
            break;
        }
        delay(SD_MOUNT_DELAY_MS);
        M5.Lcd.print(".");
    }
    const bool succeeded = (retry < SD_MOUNT_RETRY);
    M5.Lcd.println(succeeded ? " Done." : " Failed.");
    delay(500);
    M5.Lcd.clear();
    return succeeded;
}
#else
inline bool IVS_FS_BEGIN() {
    return FS_BEGIN();
}
#endif

class ImageViewer {
public:
    inline const char* getOrientationString(uint8_t rotation) {
        switch (rotation) {
            case 0:
                return "CW_0";
            case 1:
                return "CW_90";
            case 2:
                return "CW_180";
            case 3:
                return "CW_270";
            case 4:
                return "CCW_0";
            case 5:
                return "CCW_90";
            case 6:
                return "CCW_180";
            case 7:
                return "CCW_270";
            default:
                return "Unknown";
        }
    }

    static const char* VERSION;

    static const char* PATH_SEP;

    static const char* DEFAULT_CONFIG_NAME;
    static const char* KEY_AUTO_MODE;
    static const char* KEY_AUTO_MODE_INTERVAL;
    static const char* KEY_AUTO_MODE_RANDOMIZED;
    static const char* KEY_AUTO_ROTATION;
    static const char* KEY_ORIENTATION;
    static const char* KEY_CLEAR_BEFORE_DISPLAY;

    static const size_t MAX_IMAGE_FILES = 50;
    static const bool DEFAULT_AUTO_MODE = false;
    static const uint32_t DEFAULT_START_INTERVAL_MS = 3000;
    static const uint32_t DEFAULT_AUTO_MODE_INTERVAL_MS = 3000;
    static const bool DEFAULT_AUTO_MODE_RANDOMIZED = false;
    static const bool DEFAULT_AUTO_ROTATION = true;
    static const bool DEFAULT_CLEAR_BEFORE_DISPLAY = false;
    static const uint32_t FILE_LIST_DISPLAY_INTERVAL_MS = 100;
    static const int DEFAULT_BG_COLOR = TFT_WHITE;

    static const float GRAVITY_THRESHOLD;
    static const String ROOT_DIR;

    ImageViewer(const String& rootDir = ROOT_DIR,
                bool isAutoMode = DEFAULT_AUTO_MODE,
                uint32_t autoModeInterval = DEFAULT_AUTO_MODE_INTERVAL_MS,
                bool isAutoModeRandomize = DEFAULT_AUTO_MODE_RANDOMIZED,
                bool isAutoRotation = DEFAULT_AUTO_ROTATION,
                bool isClearBeforeDisplay = DEFAULT_CLEAR_BEFORE_DISPLAY);
    virtual ~ImageViewer(void);

    virtual bool begin(const int bgColor = DEFAULT_BG_COLOR);
    virtual bool update(void);

    virtual bool updateOrientation(float threshold = GRAVITY_THRESHOLD);

protected:
    virtual bool setImageFileList(void);
    virtual void showImage(void);
    virtual void clear(void);
    virtual bool hasExt(const char* filename, const char* ext) const;
    virtual bool isJpeg(const char* filename) const;
    virtual bool isPng(const char* filename) const;
    virtual bool isBmp(const char* filename) const;
    virtual bool isImageFile(const File& f) const;
    virtual uint8_t detectOrientation(float threshold);
    virtual bool parse(const char* config = DEFAULT_CONFIG_NAME);

private:
    uint8_t _orientation;
    bool _isAutoMode;
    uint32_t _autoModeInterval;
    bool _isAutoModeRandomized;
    bool _isAutoRotation;
    bool _isClearBeforeDisplay;

    String _rootDir;
    String _imageFiles[MAX_IMAGE_FILES];
    size_t _nImageFiles;
    size_t _pos;
    uint32_t _prevUpdate;
    uint32_t _interval;
    int _bgColor;
};