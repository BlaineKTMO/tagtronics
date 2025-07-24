#ifndef DualHWPwm_HPP
#define DualHWPwm_HPP

#include <Arduino.h>

class DualHardwarePWM {
public:
    DualHardwarePWM(uint8_t pin1, uint8_t pin2);
    
    void begin(uint32_t frequency = 60);
    void setDutyCycle1(float percent);
    void setDutyCycle2(float percent);

private:
    uint8_t _pin1, _pin2;
    uint32_t _frequency;
    Tcc* _timer1;
    Tcc* _timer2;
    uint8_t _cc_channel1;
    uint8_t _cc_channel2;
    
    void configureTimer(Tcc* timer, uint8_t gclk_id);
    void setDutyCycle(Tcc* timer, uint8_t cc_channel, float percent);
};

#endif
