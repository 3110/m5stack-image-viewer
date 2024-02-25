#pragma once

// taken from https://github.com/m5stack/M5Dial/issues/8#issuecomment-1887572556

#include <Arduino.h>

class M5DialEncoder {
public:
    M5DialEncoder(void);
    virtual ~M5DialEncoder(void);

    virtual void begin(void);
    virtual int16_t read(void);
};
