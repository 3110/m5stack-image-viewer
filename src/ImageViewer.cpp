#include "ImageViewer.hpp"

inline bool isEPaperTarget(void) {
#if defined(ARDUINO_M5STACK_COREINK) || defined(ARDUINO_M5STACK_PAPER) || \
    defined(ARDUINO_M5STACK_PAPERS3) || defined(ARDUINO_M5STACK_PAPERCOLOR)
    return true;
#else
    return false;
#endif
}

inline bool hasEPaperDisplay(void) {
    switch (M5.getBoard()) {
        case m5::board_t::board_M5StackCoreInk:
        case m5::board_t::board_M5Paper:
        case m5::board_t::board_M5PaperS3:
        case m5::board_t::board_M5PaperColor:
            return true;
        default:
            return false;
    }
}

#if defined(ARDUINO_M5STACK_DIAL) || defined(ARDUINO_M5STACK_DIN_METER)
#include "M5Encoder.hpp"

static M5Encoder encoder;
static int16_t prev_dial_pos = 0;

inline int16_t getEncoderOffset(void) {
    switch (M5.getBoard()) {
        case m5::board_t::board_M5Dial:
            return 4;
        case m5::board_t::board_M5DinMeter:
            return 2;
        default:
            return 0;
    }
}
#endif

inline void M5_BEGIN(m5::M5Unified::config_t& cfg) {
    M5.begin(cfg);
#if defined(ARDUINO_M5STACK_DIAL) || defined(ARDUINO_M5STACK_DIN_METER)
    encoder.begin();
#endif
#if defined(ENABLE_M5STICK_S3_SPEAKER_NOISE_WORKAROUND)
    M5.Power.setExtOutput(false);
    M5_LOGW("Work around for M5StickS3 is enabled.");
#endif
}

inline void M5_BEGIN(void) {
    auto cfg = M5.config();
    M5_BEGIN(cfg);
}

inline void M5_UPDATE(void) {
    M5.update();
}

inline int16_t getDirection(void) {
    if (M5.BtnA.wasClicked()) {
        return 1;
    } else if (M5.getBoard() == m5::board_t::board_M5PaperColor ||
               M5.getBoard() == m5::board_t::board_M5StopWatch) {
        if (M5.BtnB.wasClicked()) {
            return -1;
        }
    } else if (M5.BtnC.wasClicked()) {
        return -1;
    }

    if (M5.Touch.isEnabled()) {
        const auto detail = M5.Touch.getDetail();
        if (detail.wasFlicked()) {
            const int32_t dx = detail.distanceX();
            const int32_t dy = detail.distanceY();

            if (abs(dx) >= abs(dy)) {
                return dx > 0 ? 1 : -1;  // right: next, left: previous
            } else {
                return dy < 0 ? 1 : -1;  // up: next, down: previous
            }
        }
    }

#if defined(ARDUINO_M5STACK_DIAL) || defined(ARDUINO_M5STACK_DIN_METER)
    if (M5.getBoard() == m5::board_t::board_M5Dial ||
        M5.getBoard() == m5::board_t::board_M5DinMeter) {
        const int16_t pos = encoder.read();  // or M5Dial.Encoder.read()
        const int16_t delta = pos - prev_dial_pos;

        M5_LOGV("Encoder: %d -> %d", prev_dial_pos, pos);

        if (abs(delta) >= getEncoderOffset()) {
            const int16_t direction = delta > 0 ? 1 : -1;
            prev_dial_pos = pos;
            return direction;
        }
    }
#endif
    return 0;
}

inline int32_t getTextAreaX(void) {
    switch (M5.getBoard()) {
        case m5::board_t::board_M5Dial:
            return 35;
        case m5::board_t::board_M5PaperColor:
            return 10;
        case m5::board_t::board_M5StopWatch:
            return 80;
        default:
            return 0;
    }
}

inline int32_t getTextAreaY(void) {
    switch (M5.getBoard()) {
        case m5::board_t::board_M5Dial:
            return 35;
        case m5::board_t::board_M5PaperColor:
            return 10;
        case m5::board_t::board_M5StopWatch:
            return 80;
        default:
            return 0;
    }
}

inline int32_t getTextAreaWidth(void) {
    switch (M5.getBoard()) {
        case m5::board_t::board_M5Dial:
            return 170;
        case m5::board_t::board_M5PaperColor:
            return M5.Lcd.width() - getTextAreaX() * 2;
        case m5::board_t::board_M5StopWatch:
            return M5.Lcd.width() - getTextAreaX() * 2;
        default:
            return M5.Lcd.width();
    }
}

inline int32_t getTextAreaHeight(void) {
    switch (M5.getBoard()) {
        case m5::board_t::board_M5Dial:
            return 170;
        case m5::board_t::board_M5PaperColor:
            return M5.Lcd.height() - getTextAreaY() * 2;
        case m5::board_t::board_M5StopWatch:
            return M5.Lcd.height() - getTextAreaY() * 2;
        default:
            return M5.Lcd.height();
    }
}

#include <Arduino_JSON.h>
#include <string.h>

const char* const ImageViewer::VERSION = "v1.0.13";

const char* const ImageViewer::PATH_SEP = "/";

const char* const ImageViewer::DEFAULT_CONFIG_NAME = "image-viewer.json";
const char* const ImageViewer::KEY_AUTO_MODE = "AutoMode";
const char* const ImageViewer::KEY_AUTO_MODE_INTERVAL = "AutoModeInterval";
const char* const ImageViewer::KEY_AUTO_MODE_RANDOMIZED = "AutoModeRandomized";
const char* const ImageViewer::KEY_AUTO_ROTATION = "AutoRotation";
const char* const ImageViewer::KEY_ORIENTATION = "Orientation";
const char* const ImageViewer::KEY_CLEAR_BEFORE_DISPLAY = "ClearBeforeDisplay";

const float ImageViewer::GRAVITY_THRESHOLD = 0.9F;
const String ImageViewer::ROOT_DIR(ImageViewer::PATH_SEP);

namespace {
constexpr const char* const EXT_JPG = ".jpg";
constexpr const char* const EXT_JPEG = ".jpeg";
constexpr const char* const EXT_BMP = ".bmp";
constexpr const char* const EXT_PNG = ".png";

class LcdWriteGuard {
public:
    explicit LcdWriteGuard(bool enabled = true) : _enabled(enabled) {
        if (_enabled) {
            M5.Lcd.startWrite();
        }
    }

    ~LcdWriteGuard() {
        if (_enabled) {
            M5.Lcd.endWrite();
        }
    }

    LcdWriteGuard(const LcdWriteGuard&) = delete;
    LcdWriteGuard& operator=(const LcdWriteGuard&) = delete;

private:
    bool _enabled;
};
}  // namespace

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
    auto cfg = M5.config();
    cfg.clear_display = !isEPaperTarget();
    M5_BEGIN(cfg);

    this->_orientation = M5.Lcd.getRotation();
    M5.Lcd.setRotation(this->_orientation);

    if (hasEPaperDisplay()) {
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

    {
        LcdWriteGuard guard(hasEPaperDisplay());

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
                    M5.getBoard() == m5::board_t::board_M5StackCoreS3 ||
                    M5.getBoard() == m5::board_t::board_M5StickS3 ||
                    M5.getBoard() == m5::board_t::board_M5StopWatch) {
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
    }

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

    if (this->_nImageFiles == 0) {
        return false;
    }

    if (this->_isAutoRotation && updateOrientation(GRAVITY_THRESHOLD)) {
        showImage();
    }

    const uint32_t t = millis();
    int16_t direction = getDirection();
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
    this->_nImageFiles = 0;
    this->_pos = 0;

    File root = IV_FS.open(this->_rootDir, "r");
    if (!root || !root.isDirectory()) {
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
        f.close();
        f = root.openNextFile();
    }
    root.close();
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
    if (this->_nImageFiles == 0) {
        return;
    }

    const char* filename = this->_imageFiles[this->_pos].c_str();

    LcdWriteGuard guard;

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
    if (!M5.Imu.isEnabled()) {
        return this->_orientation;
    }

    float ax, ay, az;
    M5.Imu.getAccel(&ax, &ay, &az);
    M5_LOGV("Accel: ax: %f, ay: %f, az: %f", ax, ay, az);

    float mag_xy = sqrtf(ax * ax + ay * ay);
    if (mag_xy < 0.3f)
        return this->_orientation;

    float nx = ax / mag_xy;
    float ny = ay / mag_xy;

    if (ny >= threshold) {
        return 0;
    } else if (nx >= threshold) {
        return 1;
    } else if (nx <= -threshold) {
        return 3;
    } else if (ny <= -threshold) {
        return 2;
    }
    return this->_orientation;
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

    const size_t size = f.size();
    if (size == 0) {
        M5.Lcd.println(" E: empty config");
        f.close();
        return false;
    }
    if (size > MAX_CONFIG_SIZE) {
        M5.Lcd.println(" E: config too large");
        f.close();
        return false;
    }

    String json;
    json.reserve(size + 1);
    while (f.available()) {
        json += static_cast<char>(f.read());
    }
    f.close();

    JSONVar o = JSON.parse(json);
    if (JSON.typeof(o) == "undefined") {
        M5.Lcd.println(" E: parse");
        return false;
    }

    if (o.hasOwnProperty(KEY_AUTO_MODE)) {
        JSONVar v = o[KEY_AUTO_MODE];
        if (JSON.typeof(v) == "boolean") {
            this->_isAutoMode = static_cast<bool>(v);
        } else {
            M5_LOGE("Illegal AutoMode Type: %s", JSON.typeof(v).c_str());
        }
    }
    M5.Lcd.printf(" AutoMode: %s", this->_isAutoMode ? "true" : "false");
    M5.Lcd.println();

    if (o.hasOwnProperty(KEY_AUTO_MODE_INTERVAL)) {
        JSONVar v = o[KEY_AUTO_MODE_INTERVAL];
        if (JSON.typeof(v) == "number") {
            this->_autoModeInterval = static_cast<uint32_t>((int)v);
            this->_interval = this->_autoModeInterval;
        } else {
            M5_LOGE("Illegal AutoModeInterval Type: %s",
                    JSON.typeof(v).c_str());
        }
    }
    M5.Lcd.printf(" Interval: %dms", this->_autoModeInterval);
    M5.Lcd.println();

    if (o.hasOwnProperty(KEY_AUTO_MODE_RANDOMIZED)) {
        JSONVar v = o[KEY_AUTO_MODE_RANDOMIZED];
        if (JSON.typeof(v) == "boolean") {
            this->_isAutoModeRandomized = static_cast<bool>(v);
        } else {
            M5_LOGE("Illegal AutoModeRandomized Type: %s",
                    JSON.typeof(v).c_str());
        }
    }
    M5.Lcd.printf(" Randomized: %s",
                  this->_isAutoModeRandomized ? "true" : "false");
    M5.Lcd.println();

    if (o.hasOwnProperty(KEY_AUTO_ROTATION)) {
        JSONVar v = o[KEY_AUTO_ROTATION];
        if (JSON.typeof(v) == "boolean") {
            this->_isAutoRotation = static_cast<bool>(v);
        } else {
            M5_LOGE("Illegal AutoRotation Type: %s", JSON.typeof(v).c_str());
        }
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
        JSONVar v = o[KEY_CLEAR_BEFORE_DISPLAY];
        if (JSON.typeof(v) == "boolean") {
            this->_isClearBeforeDisplay = static_cast<bool>(v);
        } else {
            M5_LOGE("Illegal ClearBeforeDisplay Type: %s",
                    JSON.typeof(v).c_str());
        }
    }
    M5.Lcd.printf(" ClearBeforeDisplay: %s",
                  this->_isClearBeforeDisplay ? "true" : "false");
    M5.Lcd.println();

    return true;
}
