#include "ImageViewer.hpp"

#if defined(ARDUINO_M5STACK_DIAL) || defined(ARDUINO_M5STACK_DIN_METER)
#include "M5Encoder.hpp"

#if defined(ARDUINO_M5STACK_DIAL)
inline int16_t getEncoderOffset(void) {
    return 4;
}

inline int32_t getTextAreaX(void) {
    return 35;
}

inline int32_t getTextAreaY(void) {
    return 35;
}

inline int32_t getTextAreaWidth(void) {
    return 170;
}

inline int32_t getTextAreaHeight(void) {
    return 170;
}
#else
inline int16_t getEncoderOffset(void) {
    return 2;
}

inline int32_t getTextAreaX(void) {
    return 0;
}

inline int32_t getTextAreaY(void) {
    return 0;
}

inline int32_t getTextAreaWidth(void) {
    return M5.Lcd.width();
}

inline int32_t getTextAreaHeight(void) {
    return M5.Lcd.height();
}
#endif

static M5Encoder encoder;
static int16_t prev_dial_pos = 0;

inline void M5_BEGIN(m5::M5Unified::config_t cfg) {
    M5.begin(cfg);
    encoder.begin();
}

inline void M5_BEGIN(void) {
    auto cfg = M5.config();
    M5_BEGIN(cfg);
}

inline void M5_UPDATE(void) {
    M5.update();
}

inline int16_t getDirection(void) {
    // const long pos = M5Dial.Encoder.read();
    const int16_t pos = encoder.read();
    M5_LOGV("Dial: %d -> %d", prev_dial_pos, pos);
    if (abs(prev_dial_pos - pos) >= getEncoderOffset()) {
        const int16_t direction = pos - prev_dial_pos > 0 ? 1 : -1;
        prev_dial_pos = pos;
        return direction;
    } else {
        return 0;
    }
}
#else
inline void M5_BEGIN(m5::M5Unified::config_t cfg) {
    M5.begin(cfg);
}

inline void M5_BEGIN(void) {
    auto cfg = M5.config();
    M5.begin(cfg);
}

inline void M5_UPDATE(void) {
    M5.update();
}

inline int32_t getDirection(void) {
#if defined(ARDUINO_M5STACK_COREINK) || defined(ARDUINO_M5STACK_PAPER) || \
    defined(ARDUINO_M5STACK_PAPERS3) || defined(ARDUINO_M5STACK_TAB5)
    if (M5.Touch.getDetail().wasFlicked()) {
        return M5.Touch.getDetail().distanceX() > 0 ? 1 : -1;
    } else {
        return 0;
    }
#else
    if (M5.BtnA.wasClicked()) {
        return 1;
    } else if (M5.BtnC.wasClicked()) {
        return -1;
    } else {
        return 0;
    }
#endif
}

inline int32_t getTextAreaX(void) {
    return 0;
}

inline int32_t getTextAreaY(void) {
    return 0;
}

inline int32_t getTextAreaWidth(void) {
    return M5.Lcd.width();
}

inline int32_t getTextAreaHeight(void) {
    return M5.Lcd.height();
}
#endif

#include <Arduino_JSON.h>
#include <string.h>

const char* ImageViewer::VERSION = "v1.0.11";

const char* ImageViewer::PATH_SEP = "/";

const char* ImageViewer::DEFAULT_CONFIG_NAME = "image-viewer.json";
const char* ImageViewer::KEY_AUTO_MODE = "AutoMode";
const char* ImageViewer::KEY_AUTO_MODE_INTERVAL = "AutoModeInterval";
const char* ImageViewer::KEY_AUTO_MODE_RANDOMIZED = "AutoModeRandomized";
const char* ImageViewer::KEY_AUTO_ROTATION = "AutoRotation";
const char* ImageViewer::KEY_ORIENTATION = "Orientation";
const char* ImageViewer::KEY_CLEAR_BEFORE_DISPLAY = "ClearBeforeDisplay";

const float ImageViewer::GRAVITY_THRESHOLD = 0.9F;
const String ImageViewer::ROOT_DIR(ImageViewer::PATH_SEP);

static const char* EXT_JPG = ".jpg";
static const char* EXT_JPEG = ".jpeg";
static const char* EXT_BMP = ".bmp";
static const char* EXT_PNG = ".png";

ImageViewer::ImageViewer(const String& rootDir, bool isAutoMode,
                         uint32_t autoModeInterval, bool isAutoModeRandomized,
                         bool isAutoRotation, bool isClearBeforeDisplay)
    : _rootDir(rootDir.endsWith(PATH_SEP) ? rootDir : rootDir + PATH_SEP),
      _orientation(0),
      _isAutoMode(isAutoMode),
      _autoModeInterval(autoModeInterval),
      _isAutoModeRandomized(isAutoModeRandomized),
      _isAutoRotation(isAutoRotation),
      _isClearBeforeDisplay(isClearBeforeDisplay),
      _nImageFiles(0),
      _pos(0),
      _prevUpdate(0),
      _interval(autoModeInterval),
      _bgColor(DEFAULT_BG_COLOR) {
    randomSeed(analogRead(0));
    for (size_t i = 0; i < MAX_IMAGE_FILES; ++i) {
        this->_imageFiles[i] = "";
    }
}

ImageViewer::~ImageViewer(void) {
}

bool ImageViewer::begin(int bgColor) {
    M5_BEGIN();

    this->_orientation = M5.Lcd.getRotation();
    M5.Lcd.setRotation(this->_orientation);

    if (M5.getBoard() == m5::board_t::board_M5StackCoreInk ||
        M5.getBoard() == m5::board_t::board_M5Paper ||
        M5.getBoard() == m5::board_t::board_M5PaperS3) {
        M5.Lcd.invertDisplay(false);
        M5.Lcd.setEpdMode(epd_mode_t::epd_quality);
    }

    if (M5.getBoard() == m5::board_t::board_M5Paper ||
        M5.getBoard() == m5::board_t::board_M5PaperS3) {
        M5.Lcd.setTextSize(2);
    } else if (M5.getBoard() == m5::board_t::board_M5Tab5) {
        M5.Lcd.setTextSize(3);
    }

    M5.Lcd.setTextScroll(true);
    M5.Lcd.setCursor(getTextAreaX(), getTextAreaY());
    M5.Lcd.setScrollRect(getTextAreaX(), getTextAreaY(), getTextAreaWidth(),
                         getTextAreaHeight());

    if (!IVS_FS_BEGIN()) {
        M5.Lcd.printf("Failed to mount %s", TOSTRING(IV_FS));
        M5.Lcd.println();
        return false;
    }
    M5.Lcd.setFileStorage(IV_FS);

    M5.Lcd.printf("Image Viewer %s", VERSION);
    M5.Lcd.println();
    if (!parse()) {
        return false;
    }

    M5_UPDATE();
    M5.Lcd.println("Mode:");
    if (M5.BtnA.isPressed()) {
        this->_isAutoMode = true;  // overriding the setting
        M5.Lcd.println(" Auto(Forced)");
    } else {
        M5.Lcd.println(this->_isAutoMode ? " Auto" : " Manual");
    }

    M5.Lcd.println("Rotation:");
    if (this->_isAutoRotation) {
        if (M5.Imu.isEnabled()) {
            M5.Lcd.println(" Auto");
            if (M5.getBoard() == m5::board_t::board_M5Stack ||
                M5.getBoard() == m5::board_t::board_M5StackCore2 ||
                M5.getBoard() == m5::board_t::board_M5StackCoreS3) {
                M5.Imu.setAxisOrder(m5::IMU_Class::axis_y_pos,
                                    m5::IMU_Class::axis_x_neg,
                                    m5::IMU_Class::axis_z_pos);
            } else if (M5.getBoard() == m5::board_t::board_M5PaperS3) {
                M5.Imu.setAxisOrder(m5::IMU_Class::axis_y_pos,
                                    m5::IMU_Class::axis_x_pos,
                                    m5::IMU_Class::axis_z_pos);
            } else if (M5.getBoard() == m5::board_t::board_M5Tab5) {
                M5.Imu.setAxisOrder(m5::IMU_Class::axis_x_neg,
                                    m5::IMU_Class::axis_y_pos,
                                    m5::IMU_Class::axis_z_neg);
            }
        } else {
            this->_isAutoRotation = false;
            M5.Lcd.println(" No(IMU disabled)");
        }
    } else {
        M5.Lcd.println(" No");
    }

    delay(DEFAULT_START_INTERVAL_MS);
    if (!setImageFileList()) {
        return false;
    }

    M5.Lcd.clearScrollRect();
    M5.Lcd.setCursor(0, 0);

    delay(DEFAULT_START_INTERVAL_MS);
    this->_bgColor = bgColor;
    clear();
    if (this->_isAutoRotation) {
        updateOrientation();
    } else {
        M5.Lcd.setRotation(this->_orientation);
    }

    if (!this->_isAutoMode) {
        showImage();
    }

    return true;
}

bool ImageViewer::update(void) {
    M5_UPDATE();

    if (this->_isAutoRotation && updateOrientation(GRAVITY_THRESHOLD)) {
        showImage();
    }

    const uint32_t t = millis();
    int32_t direction = getDirection();
    if (direction == 0 && this->_isAutoMode &&
        t - this->_prevUpdate >= this->_interval) {
        direction = 1;
    }
    if (direction != 0) {
        this->_prevUpdate = t;
        if (direction < 0 && this->_pos == 0) {
            this->_pos = this->_nImageFiles - 1;
        } else if (direction > 0 && this->_pos == this->_nImageFiles - 1) {
            this->_pos = 0;
        } else {
            this->_pos += direction;
        }
        showImage();
        if (this->_isAutoMode && this->_isAutoModeRandomized) {
            this->_interval = random(this->_autoModeInterval);
        }
    }
    return direction != 0;
}

bool ImageViewer::setImageFileList(void) {
    File root = IV_FS.open(this->_rootDir, "r");
    if (!root and !root.isDirectory()) {
        M5.Lcd.printf("Failed to open \"%s\"", this->_rootDir.c_str());
        M5.Lcd.println();
        return false;
    }
    File f = root.openNextFile();
    while (f && this->_nImageFiles < MAX_IMAGE_FILES) {
        if (!f.isDirectory() && isImageFile(f)) {
            this->_imageFiles[this->_nImageFiles] = this->_rootDir + f.name();
            ++this->_nImageFiles;
        }
        f = root.openNextFile();
    }
    if (this->_nImageFiles == 0) {
        M5.Lcd.println("No image files found");
        return false;
    }
    std::sort(this->_imageFiles, this->_imageFiles + this->_nImageFiles);
    M5.Lcd.println("Image Files:");
    for (size_t c = 0; c < this->_nImageFiles; ++c) {
        M5.Lcd.print(" ");
        M5.Lcd.println(this->_imageFiles[c]);
        delay(FILE_LIST_DISPLAY_INTERVAL_MS);
    }
    return true;
}

bool ImageViewer::updateOrientation(float threshold) {
    const uint8_t o = detectOrientation(threshold);
    if (this->_orientation != o) {
        M5_LOGD("Change Orientation: %d -> %d", this->_orientation, o);
        this->_orientation = o;
        M5.Lcd.setRotation(this->_orientation);
        return true;
    }
    return false;
}

void ImageViewer::showImage(void) {
    const char* filename = this->_imageFiles[this->_pos].c_str();
    M5.Lcd.startWrite();
    if (this->_isClearBeforeDisplay) {
        clear();
    }
    if (isJpeg(filename)) {
        M5.Lcd.drawJpgFile(filename, 0, 0, M5.Display.width(),
                           M5.Display.height(), 0, 0, 0.0F, 0.0F,
                           middle_center);
    } else if (isPng(filename)) {
        M5.Lcd.drawPngFile(filename, 0, 0, M5.Display.width(),
                           M5.Display.height(), 0, 0, 0.0F, 0.0F,
                           middle_center);
    } else if (isBmp(filename)) {
        M5.Lcd.drawBmpFile(filename, 0, 0, M5.Display.width(),
                           M5.Display.height(), 0, 0, 0.0F, 0.0F,
                           middle_center);
    } else {
        M5.Lcd.printf("ignore: %s", filename);
        M5.Lcd.println();
    }
    M5.Lcd.endWrite();
}

void ImageViewer::clear(void) {
    M5.Lcd.clear(this->_bgColor);
}

bool ImageViewer::hasExt(const char* filename, const char* ext) const {
    if (filename == nullptr) {
        return false;
    }
    if (ext == nullptr) {
        return false;
    }
    const char* p = strrchr(filename, '.');
    return p != nullptr && strcasecmp(ext, p) == 0;
}

bool ImageViewer::isJpeg(const char* filename) const {
    if (filename == nullptr) {
        return false;
    }
    return hasExt(filename, EXT_JPG) || hasExt(filename, EXT_JPEG);
}

bool ImageViewer::isPng(const char* filename) const {
    if (filename == nullptr) {
        return false;
    }
    return hasExt(filename, EXT_PNG);
}

bool ImageViewer::isBmp(const char* filename) const {
    if (filename == nullptr) {
        return false;
    }
    return hasExt(filename, EXT_BMP);
}

bool ImageViewer::isImageFile(const File& f) const {
    const char* name = f.name();
    return isJpeg(name) || isPng(name) || isBmp(name);
}

uint8_t ImageViewer::detectOrientation(float threshold) {
    if (M5.Imu.isEnabled()) {
        float ax, ay, az;
        M5.Imu.getAccel(&ax, &ay, &az);
        M5_LOGV("Accel: ax: %f, ay: %f, az: %f", ax, ay, az);
        if (ay >= threshold) {
            return 0;
        } else if (ax >= threshold) {
            return 1;
        } else if (ax <= -threshold) {
            return 3;
        } else if (ay <= -threshold) {
            return 2;
        }
    }
    return 0;
}

bool ImageViewer::parse(const char* config) {
    if (config == nullptr) {
        M5_LOGE("config is null");
        return false;
    }
    const String filename = this->_rootDir + config;
    if (!IV_FS.exists(filename)) {
        M5_LOGW("%s is not found", filename.c_str());
        return true;  // use default
    }
    M5.Lcd.println("Config:");
    M5.Lcd.printf(" %s", filename.c_str());
    M5.Lcd.println();
    File f = IV_FS.open(filename, "r");
    if (!f) {
        M5.Lcd.println(" E: failed to open");
        return false;
    }
    uint8_t buf[f.size()] = {0};
    f.read(buf, sizeof(buf));
    f.close();

    JSONVar o = JSON.parse((const char*)buf);
    if (JSON.typeof(o) == "undefined") {
        M5.Lcd.println(" E: parse");
        return false;
    }
    if (o.hasOwnProperty(KEY_AUTO_MODE)) {
        this->_isAutoMode = (bool)o[KEY_AUTO_MODE];
    }
    M5.Lcd.printf(" AutoMode: %s", this->_isAutoMode ? "true" : "false");
    M5.Lcd.println();
    if (o.hasOwnProperty(KEY_AUTO_MODE_INTERVAL)) {
        this->_autoModeInterval = (uint32_t)o[KEY_AUTO_MODE_INTERVAL];
        this->_interval = this->_autoModeInterval;
    }
    M5.Lcd.printf(" Interval: %dms", this->_autoModeInterval);
    M5.Lcd.println();
    if (o.hasOwnProperty(KEY_AUTO_MODE_RANDOMIZED)) {
        this->_isAutoModeRandomized = (bool)o[KEY_AUTO_MODE_RANDOMIZED];
    }
    M5.Lcd.printf(" Randomized: %s",
                  this->_isAutoModeRandomized ? "true" : "false");
    M5.Lcd.println();
    if (o.hasOwnProperty(KEY_AUTO_ROTATION)) {
        this->_isAutoRotation = (bool)o[KEY_AUTO_ROTATION];
    }
    M5.Lcd.printf(" AutoRotation: %s",
                  this->_isAutoRotation ? "true" : "false");
    M5.Lcd.println();
    if (o.hasOwnProperty(KEY_ORIENTATION)) {
        JSONVar orientationVar = o[KEY_ORIENTATION];
        if (JSON.typeof(orientationVar) == "number") {
            int orientationInt = (int)orientationVar;
            if (0 <= orientationInt && orientationInt <= 7) {
                this->_orientation = orientationInt;
            } else {
                this->_orientation = M5.Lcd.getRotation();
                M5_LOGE("Invalid Orientation Value: %d", orientationInt);
            }
        } else {
            this->_orientation = M5.Lcd.getRotation();
            M5_LOGE("Illegal Orientation Type: %s, Value: %s",
                    JSON.typeof(orientationVar).c_str(),
                    JSONVar::stringify(orientationVar).c_str());
        }
    } else {
        this->_orientation = M5.Lcd.getRotation();
        M5_LOGW("Default Orientation is not found");
    }
    M5.Lcd.printf(" Orientation: %s", getOrientationString(this->_orientation));
    M5.Lcd.println();
    if (o.hasOwnProperty(KEY_CLEAR_BEFORE_DISPLAY)) {
        this->_isClearBeforeDisplay = (bool)o[KEY_CLEAR_BEFORE_DISPLAY];
    }
    M5.Lcd.printf(" ClearBeforeDisplay: %s",
                  this->_isClearBeforeDisplay ? "true" : "false");
    M5.Lcd.println();
    return true;
}
