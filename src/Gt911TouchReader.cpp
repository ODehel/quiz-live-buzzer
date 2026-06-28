#include "Gt911TouchReader.h"
#include <Arduino.h>
#include <Wire.h>

static constexpr uint8_t REG_OUTPUT = 0x01;
static constexpr uint8_t REG_CONFIG = 0x03;
static constexpr uint8_t PCA9557_I2C_ADDR = 0x18;

void Gt911TouchReader::WriteReg(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(PCA9557_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t Gt911TouchReader::ReadReg(uint8_t reg)
{
    Wire.beginTransmission(PCA9557_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(PCA9557_I2C_ADDR, (uint8_t)1);
    return Wire.read();
}

void Gt911TouchReader::SetMode(uint8_t pin, PinMode mode)
{
    uint8_t current = ReadReg(REG_CONFIG);
    uint8_t updated = mode == PinMode::Input
                          ? (current | (1 << pin))
                          : current & ~(1 << pin);
    WriteReg(REG_CONFIG, updated);
}

void Gt911TouchReader::SetState(uint8_t pin, PinState state)
{
    uint8_t current = ReadReg(REG_OUTPUT);
    uint8_t updated = state == PinState::High
                          ? (current | (1 << pin))
                          : current & ~(1 << pin);
    WriteReg(REG_OUTPUT, updated);
}

void Gt911TouchReader::ResetTouchSequence()
{
    constexpr uint8_t ALL_PINS = 0xFF;
    constexpr uint8_t IO0 = 0x00;
    constexpr uint8_t IO1 = 0x01;

    // Neutralise l'expander : toutes les broches en entrée (0xFF = convention CONFIG 1→input)
    // avant de piloter finement IO0/IO1 pour la séquence de reset du GT911
    WriteReg(REG_CONFIG, ALL_PINS);
    SetMode(IO0, PinMode::Output);
    SetMode(IO1, PinMode::Output);
    SetState(IO0, PinState::Low);
    SetState(IO1, PinState::Low);
    delay(20);
    SetState(IO0, PinState::High);
    delay(100);
    SetMode(IO1, PinMode::Input);
}

TouchPoint Gt911TouchReader::Read()
{
    ts.read();
    if (!ts.isTouched)
        return TouchPoint::NoContact();

    return projection.Project({ts.points[0].x, ts.points[0].y});
}

void Gt911TouchReader::Begin()
{
    Wire.begin(19, 20);
    ResetTouchSequence();
    ts.begin();
    // INVERTED = identité dans TAMC_GT911 v1.0.2 ;
    // neutralise la rotation lib pour que TouchProjection soit seule autorité du repérage.
    // Réserve : sens final des axes NON validé matériellement ; divergence du repère factory ;
    // correction au smoke en point unique (rotation lib OU bornes TouchProjection).
    ts.setRotation(ROTATION_INVERTED);
}