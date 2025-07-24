#include "DualHWPwm.hpp"
#include <Arduino.h>

DualHardwarePWM::DualHardwarePWM(uint8_t pin1, uint8_t pin2) 
  : _pin1(pin1), _pin2(pin2), _timer1(nullptr), _timer2(nullptr), 
    _cc_channel1(0), _cc_channel2(0) {}

void DualHardwarePWM::begin(uint32_t frequency) {
    _frequency = frequency;
    bool timer1_configured = false;

    // Structure for pin mapping
    struct PinInfo {
        uint8_t pin;
        Tcc* timer;
        uint8_t cc_channel;
        uint8_t gclk_id;
        uint8_t port_pin;
        uint8_t pmux_index;
        uint8_t pmux_position;
        uint8_t mux_val;
    };

    // Pin mapping table for Feather M0
    const PinInfo pinMap[] = {
        // D5: PA15 - TCC0/WO[1]
        {5, TCC0, 1, 0x1A, 15, 7, 1, 0x05},  // Function F
        
        // D6: PA20 - TCC0/WO[0]
        {6, TCC0, 0, 0x1A, 20, 10, 0, 0x05}, // Function F
        
        // D9: PA07 - TCC1/WO[1]
        {9, TCC1, 1, 0x1A, 7, 3, 1, 0x04},   // Function E
        
        // D10: PA18 - TCC1/WO[2]
        {10, TCC1, 2, 0x1A, 18, 9, 0, 0x04}, // Function E
        
        // D11: PA16 - TCC1/WO[0]
        {11, TCC1, 0, 0x1A, 16, 8, 0, 0x04}, // Function E
        
        // D12: PA19 - TCC1/WO[3]
        {12, TCC1, 3, 0x1A, 19, 9, 1, 0x04}, // Function E
        
        // D13: PA17 - TCC2/WO[1]
        {13, TCC2, 1, 0x1C, 17, 8, 1, 0x04}  // Function E
    };
    const uint8_t numPins = sizeof(pinMap) / sizeof(pinMap[0]);

    // Configure first pin
    for (uint8_t i = 0; i < numPins; i++) {
        if (pinMap[i].pin == _pin1) {
            // Enable timer clock
            if (pinMap[i].timer == TCC0) PM->APBCMASK.reg |= PM_APBCMASK_TCC0;
            else if (pinMap[i].timer == TCC1) PM->APBCMASK.reg |= PM_APBCMASK_TCC1;
            else if (pinMap[i].timer == TCC2) PM->APBCMASK.reg |= PM_APBCMASK_TCC2;

            // Configure pin multiplexing
            PORT->Group[0].PINCFG[pinMap[i].port_pin].bit.PMUXEN = 1;
            if (pinMap[i].pmux_position == 0) {
                PORT->Group[0].PMUX[pinMap[i].pmux_index].bit.PMUXE = pinMap[i].mux_val;
            } else {
                PORT->Group[0].PMUX[pinMap[i].pmux_index].bit.PMUXO = pinMap[i].mux_val;
            }

            _timer1 = pinMap[i].timer;
            _cc_channel1 = pinMap[i].cc_channel;
            timer1_configured = true;
            configureTimer(_timer1, pinMap[i].gclk_id);
            break;
        }
    }

    // Configure second pin
    for (uint8_t i = 0; i < numPins; i++) {
        if (pinMap[i].pin == _pin2) {
            // Skip if same timer already configured
            bool same_timer = (_timer1 == pinMap[i].timer);
            
            // Enable timer clock if different
            if (!same_timer) {
                if (pinMap[i].timer == TCC0) PM->APBCMASK.reg |= PM_APBCMASK_TCC0;
                else if (pinMap[i].timer == TCC1) PM->APBCMASK.reg |= PM_APBCMASK_TCC1;
                else if (pinMap[i].timer == TCC2) PM->APBCMASK.reg |= PM_APBCMASK_TCC2;
            }

            // Configure pin multiplexing
            PORT->Group[0].PINCFG[pinMap[i].port_pin].bit.PMUXEN = 1;
            if (pinMap[i].pmux_position == 0) {
                PORT->Group[0].PMUX[pinMap[i].pmux_index].bit.PMUXE = pinMap[i].mux_val;
            } else {
                PORT->Group[0].PMUX[pinMap[i].pmux_index].bit.PMUXO = pinMap[i].mux_val;
            }

            _timer2 = pinMap[i].timer;
            _cc_channel2 = pinMap[i].cc_channel;
            
            // Configure timer if not already done
            if (!same_timer || !timer1_configured) {
                configureTimer(_timer2, pinMap[i].gclk_id);
            }
            break;
        }
    }
}

void DualHardwarePWM::configureTimer(Tcc* timer, uint8_t gclk_id) {
    // Enable GCLK for timer
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | 
                        GCLK_CLKCTRL_GEN_GCLK0 | 
                        GCLK_CLKCTRL_ID(gclk_id);
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Reset timer
    timer->CTRLA.bit.ENABLE = 0;
    while (timer->SYNCBUSY.bit.ENABLE);
    timer->CTRLA.bit.SWRST = 1;
    while (timer->SYNCBUSY.bit.SWRST);

    // Set waveform generation mode
    timer->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (timer->SYNCBUSY.bit.WAVE);

    // Set frequency (prescaler 1024)
    const uint32_t prescaler = 1024;
    timer->CTRLA.bit.PRESCALER = TCC_CTRLA_PRESCALER_DIV1024;
    timer->PER.reg = (48000000UL / (prescaler * _frequency)) - 1;
    while (timer->SYNCBUSY.bit.PER);

    // Enable timer
    timer->CTRLA.bit.ENABLE = 1;
    while (timer->SYNCBUSY.bit.ENABLE);
}

void DualHardwarePWM::setDutyCycle(Tcc* timer, uint8_t cc_channel, float percent) {
    if (!timer) return;
    
    percent = constrain(percent, 0.0f, 100.0f);
    const uint32_t per = timer->PER.reg;
    timer->CC[cc_channel].reg = (per * percent) / 100;
    
    // Wait for synchronization
    switch (cc_channel) {
        case 0: while (timer->SYNCBUSY.bit.CC0); break;
        case 1: while (timer->SYNCBUSY.bit.CC1); break;
        case 2: while (timer->SYNCBUSY.bit.CC2); break;
        case 3: while (timer->SYNCBUSY.bit.CC3); break;
    }
}

void DualHardwarePWM::setDutyCycle1(float percent) {
    setDutyCycle(_timer1, _cc_channel1, percent);
}

void DualHardwarePWM::setDutyCycle2(float percent) {
    setDutyCycle(_timer2, _cc_channel2, percent);
}
