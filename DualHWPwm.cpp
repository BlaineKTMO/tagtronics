#include "DualHWPwm.hpp"
#include <Arduino.h>

DualHardwarePWM::DualHardwarePWM(uint8_t pin1, uint8_t pin2) 
  : _pin1(pin1), _pin2(pin2), _timer1(nullptr), _timer2(nullptr) {}

void DualHardwarePWM::begin(uint32_t frequency) {
    _frequency = frequency;

    // Configure Pin 10 (PA18, TCC0/WO[2])
    if (_pin1 == 10) {
        PM->APBCMASK.reg |= PM_APBCMASK_TCC0;  // Enable TCC0 clock
        _timer1 = TCC0;
        PORT->Group[PORTA].PINCFG[18].bit.PMUXEN = 1;
        PORT->Group[PORTA].PMUX[9].bit.PMUXE = PORT_PMUX_PMUXE_E_Val;  // Function E
        configureTimer(_timer1, TCC0_GCLK_ID);
    }

    // Configure Pin 11 (PA16, TCC1/WO[0])
    if (_pin2 == 11) {
        PM->APBCMASK.reg |= PM_APBCMASK_TCC1;  // Enable TCC1 clock
        _timer2 = TCC1;
        PORT->Group[PORTA].PINCFG[16].bit.PMUXEN = 1;
        PORT->Group[PORTA].PMUX[8].bit.PMUXE = PORT_PMUX_PMUXE_E_Val;  // Function E
        configureTimer(_timer2, TCC1_GCLK_ID);
    }
}

void DualHardwarePWM::configureTimer(Tcc* timer, uint8_t gclk_id) {
    // Enable clock
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | 
                        GCLK_CLKCTRL_GEN_GCLK0 | 
                        GCLK_CLKCTRL_ID(gclk_id);
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Reset timer
    timer->CTRLA.bit.ENABLE = 0;
    while (timer->SYNCBUSY.bit.ENABLE);
    timer->CTRLA.bit.SWRST = 1;
    while (timer->SYNCBUSY.bit.SWRST);

    // Waveform setup
    timer->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
    while (timer->SYNCBUSY.bit.WAVE);

    // Set frequency
    const uint32_t prescaler = 1024;
    timer->CTRLA.bit.PRESCALER = 0x07;  // Prescaler 1024
    timer->PER.reg = (48000000UL / (prescaler * _frequency)) - 1;
    while (timer->SYNCBUSY.bit.PER);

    // Enable timer
    timer->CTRLA.bit.ENABLE = 1;
    while (timer->SYNCBUSY.bit.ENABLE);
}

void DualHardwarePWM::setDutyCycle(Tcc* timer, uint8_t cc_channel, float percent) {
    if (!timer) return;
    
    const uint32_t per = timer->PER.reg;
    timer->CC[cc_channel].reg = (per * constrain(percent, 0, 100)) / 100;
    
    // Wait for CC channel sync
    switch (cc_channel) {
        case 0: while (timer->SYNCBUSY.bit.CC0); break;
        case 1: while (timer->SYNCBUSY.bit.CC1); break;
        case 2: while (timer->SYNCBUSY.bit.CC2); break;
        case 3: while (timer->SYNCBUSY.bit.CC3); break;
    }
}

void DualHardwarePWM::setDutyCycle1(float percent) {
    if (_timer1) setDutyCycle(_timer1, 2, percent);  // Channel 2 for TCC0 (WO[2])
}

void DualHardwarePWM::setDutyCycle2(float percent) {
    if (_timer2) setDutyCycle(_timer2, 0, percent);  // Channel 0 for TCC1 (WO[0])
}