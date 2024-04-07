#include "ImageViewer.hpp"

#if defined(ARDUINO_M5STACK_DIAL)
#include "M5DialEncoder.hpp"

static M5DialEncoder encoder;
static int16_t prev_dial_pos = 0;

inline void M5_BEGIN(m5::M5Unified::config_t cfg) {
    M5.begin();
    encoder.begin();
    M5.Lcd.setCursor(56, 60);
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
    if (abs(prev_dial_pos - pos) >= 4) {
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
#if defined(ARDUINO_M5STACK_COREINK) || defined(ARDUINO_M5STACK_PAPER)
    M5.Lcd.invertDisplay(false);
    M5.Lcd.setEpdMode(epd_mode_t::epd_quality);
#endif
}

inline void M5_BEGIN(void) {
    auto cfg = M5.config();
    M5.begin(cfg);
}

inline void M5_UPDATE(void) {
    M5.update();
}

inline int32_t getDirection(void) {
    if (M5.BtnA.wasClicked()) {
        return 1;
    } else if (M5.BtnC.wasClicked()) {
        return -1;
    } else {
        return 0;
    }
}
#endif

#include <Arduino_JSON.h>
#include <string.h>

const char* ImageViewer::VERSION = "v1.0.0";

const char* ImageViewer::DEFAULT_CONFIG_NAME = "image-viewer.json";
const char* ImageViewer::KEY_AUTO_MODE = "AutoMode";
const char* ImageViewer::KEY_AUTO_MODE_INTERVAL = "AutoModeInterval";
const char* ImageViewer::KEY_AUTO_MODE_RANDOMIZED = "AutoModeRandomized";
const char* ImageViewer::KEY_AUTO_ROTATION = "AutoRotation";

const float ImageViewer::GRAVITY_THRESHOLD = 0.9F;
const String ImageViewer::ROOT_DIR("/");

static const bool FORMAT_FS_IF_FAILED = true;
static const char* EXT_JPG = ".jpg";
static const char* EXT_JPEG = ".jpeg";
static const char* EXT_BMP = ".bmp";
static const char* EXT_PNG = ".png";

ImageViewer::ImageViewer(Orientation orientation, bool isAutoMode,
                         uint32_t autoModeInterval, bool isAutoModeRandomized,
                         bool isAutoRotation)
    : _orientation(orientation),
      _isAutoMode(isAutoMode),
      _autoModeInterval(autoModeInterval),
      _isAutoModeRandomized(isAutoModeRandomized),
      _isAutoRotation(isAutoRotation),
      _imageFiles{""},
      _nImageFiles(0),
      _pos(0),
      _prevUpdate(0),
      _interval(autoModeInterval) {
    randomSeed(analogRead(0));
}

ImageViewer::~ImageViewer(void) {
}

bool ImageViewer::begin(int bgColor) {
    M5_BEGIN();

    M5.Lcd.setRotation(this->_orientation);

    if (!IV_FS.begin(FORMAT_FS_IF_FAILED)) {
        M5.Lcd.println("Failed to mount File System");
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
        if (!M5.Imu.isEnabled()) {
            this->_isAutoRotation = false;
            M5.Lcd.println(" No(IMU disabled)");
        } else {
            M5.Lcd.println(" Auto");
        }
    } else {
        M5.Lcd.println(" No");
    }
    if (this->_isAutoRotation) {
        if (M5.getBoard() == m5::board_t::board_M5Stack ||
            M5.getBoard() == m5::board_t::board_M5StackCoreS3 ||
            M5.getBoard() == m5::board_t::board_M5StackCore2) {
            M5.Imu.setAxisOrder(m5::IMU_Class::axis_y_pos,
                                m5::IMU_Class::axis_x_neg,
                                m5::IMU_Class::axis_z_pos);
        }
        updateOrientation();
    }

    delay(DEFAULT_START_INTERVAL_MS);
    if (!setImageFileList()) {
        return false;
    }

    delay(DEFAULT_START_INTERVAL_MS);
    M5.Lcd.clear();
    M5.Lcd.fillScreen(bgColor);

    if (!this->_isAutoMode) {
        showImage(this->_imageFiles, this->_pos);
    }
    return true;
}

bool ImageViewer::update(void) {
    M5_UPDATE();

    if (this->_isAutoRotation && updateOrientation(GRAVITY_THRESHOLD)) {
        showImage(this->_imageFiles, this->_pos);
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
        showImage(this->_imageFiles, this->_pos);
        if (this->_isAutoMode && this->_isAutoModeRandomized) {
            this->_interval = random(this->_autoModeInterval);
        }
    }
    return direction != 0;
}

bool ImageViewer::setImageFileList(const String& path) {
    File root = IV_FS.open(path.c_str(), "r");
    if (!root and !root.isDirectory()) {
        M5.Lcd.printf("Failed to open \"%s\"", ROOT_DIR);
        M5.Lcd.println();
        return false;
    }
    File f = root.openNextFile();
    while (f && this->_nImageFiles < MAX_IMAGE_FILES) {
        if (!f.isDirectory() && isImageFile(f)) {
            this->_imageFiles[this->_nImageFiles] = ROOT_DIR + f.name();
            ++this->_nImageFiles;
        }
        f = root.openNextFile();
    }
    if (this->_nImageFiles == 0) {
        M5.Lcd.println("No image files found");
        return false;
    }
    M5.Lcd.println("Image Files:");
    for (size_t c = 0; c < this->_nImageFiles; ++c) {
        M5.Lcd.print(" ");
        M5.Lcd.println(this->_imageFiles[c]);
    }
    return true;
}

bool ImageViewer::updateOrientation(float threshold) {
    const Orientation o = detectOrientation(threshold);
    if (this->_orientation != o) {
        M5_LOGD("Change Orientation: %d -> %d", this->_orientation, o);
        this->_orientation = o;
        M5.Lcd.setRotation(this->_orientation);
        return true;
    }
    return false;
}

void ImageViewer::showImage(const String images[], size_t p) {
    const char* filename = images[p].c_str();
    M5.Lcd.startWrite();
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

ImageViewer::Orientation ImageViewer::detectOrientation(float threshold) {
    if (M5.Imu.isEnabled()) {
        float ax, ay, az;
        M5.Imu.getAccel(&ax, &ay, &az);
        M5_LOGV("Accel: ax: %f, ay: %f, az: %f", ax, ay, az);
        if (ay >= threshold) {
            return OrientationNormal;
        } else if (ax >= threshold) {
            return OrientationRight;
        } else if (ax <= -threshold) {
            return OrientationLeft;
        } else if (ay <= -threshold) {
            return OrientationUpsideDown;
        }
    }
    return DEFAULT_ORIENTATION;
}

bool ImageViewer::parse(const char* config) {
    if (config == nullptr) {
        M5_LOGE("config is null");
        return false;
    }
    const String filename = ROOT_DIR + config;
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
    return true;
}
