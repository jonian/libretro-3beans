/*
    Copyright 2023-2026 Hydr8gon

    This file is part of 3Beans.

    3Beans is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    3Beans is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with 3Beans. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../core.h"

I2c::I2c(Core &core): core(core) {
    // Try to load MCU RAM data from a file
    if (FILE *file = fopen((Settings::basePath + "/mcu_ram.bin").c_str(), "rb")) {
        fread(mcuRamData, sizeof(uint8_t), sizeof(mcuRamData), file);
        fclose(file);
    }
}

I2c::~I2c() {
    // Ensure MCU RAM is written
    updateMcuRam();
}

void I2c::updateMcuRam() {
    // Update the MCU RAM file if its data changed
    if (!ramDirty) return;
    if (FILE *file = fopen((Settings::basePath + "/mcu_ram.bin").c_str(), "wb")) {
        LOG_INFO("Writing updated MCU RAM file to disk\n");
        fwrite(mcuRamData, sizeof(uint8_t), sizeof(mcuRamData), file);
        fclose(file);
        ramDirty = false;
    }
}

void I2c::mcuInterrupt(uint32_t mask) {
    // Set MCU interrupt flags and trigger if a set flag is enabled
    if ((mcuIrqFlags |= mask) & ~mcuIrqMask)
        core.interrupts.sendInterrupt(ARM11, 0x71);
}

uint8_t I2c::readMcu() {
    // Get the MCU address and increment if enabled
    uint8_t address = regAddrs[1];
    regAddrs[1] += mcuInc;

    // Read from an MCU register at the address
    switch (address) {
        case 0x00: return 0x13; // Version high
        case 0x01: return 0x41; // Version low
        case 0x0B: return 0x64; // Battery percent
        case 0x0F: return 0x02; // Power flags
        case 0x10: return readMcuIrqFlags(0);
        case 0x11: return readMcuIrqFlags(1);
        case 0x12: return readMcuIrqFlags(2);
        case 0x13: return readMcuIrqFlags(3);
        case 0x18: return readMcuIrqMask(0);
        case 0x19: return readMcuIrqMask(1);
        case 0x1A: return readMcuIrqMask(2);
        case 0x1B: return readMcuIrqMask(3);
        case 0x30: return readMcuRtcVal(0);
        case 0x31: return readMcuRtcVal(1);
        case 0x32: return readMcuRtcVal(2);
        case 0x33: return readMcuRtcVal(3);
        case 0x34: return readMcuRtcVal(4);
        case 0x35: return readMcuRtcVal(5);
        case 0x36: return readMcuRtcVal(6);
        case 0x60: return mcuRamIdx;
        case 0x61: return readMcuRamData();

    default:
        // Catch reads from unknown MCU registers
        LOG_WARN("Unknown I2C MCU read from register 0x%X\n", address);
        return 0;
    }
}

void I2c::writeMcu(uint8_t value) {
    // Set the 8-bit MCU address and disable increment for certain values
    if (writeCounts[1] == 2) {
        regAddrs[1] = value;
        mcuInc = (value != 0x29 && value != 0x2D && value != 0x4F && value != 0x61 && value != 0x7F);
        return;
    }

    // Increment the MCU address if enabled
    uint8_t address = regAddrs[1];
    regAddrs[1] += mcuInc;

    // Write to an MCU register at the address
    switch (address) {
        case 0x18: return writeMcuIrqMask(0, value);
        case 0x19: return writeMcuIrqMask(1, value);
        case 0x1A: return writeMcuIrqMask(2, value);
        case 0x1B: return writeMcuIrqMask(3, value);
        case 0x22: return writeMcuLcdPower(value);
        case 0x60: return writeMcuRamIdx(value);
        case 0x61: return writeMcuRamData(value);

    default:
        // Catch writes to unknown MCU registers
        LOG_WARN("Unknown I2C MCU write to register 0x%X\n", address);
        return;
    }
}

uint8_t I2c::readMcuIrqFlags(int i) {
    // Read MCU interrupt flags and clear them
    uint8_t value = mcuIrqFlags >> (i << 3);
    mcuIrqFlags &= ~(0xFF << (i << 3));
    return value;
}

uint8_t I2c::readMcuIrqMask(int i) {
    // Read part of the MCU interrupt mask
    return mcuIrqMask >> (i << 3);
}

uint8_t I2c::readMcuRtcVal(int i) {
    // Get the local time and adjust values for the DS
    std::time_t t = std::time(nullptr);
    std::tm *time = std::localtime(&t);
    time->tm_year %= 100; // 2000-2099
    time->tm_mon++; // Starts at 1

    // Read the requested value converted to BCD format
    switch (i) {
        case 0: return ((time->tm_sec / 10) << 4) | (time->tm_sec % 10);
        case 1: return ((time->tm_min / 10) << 4) | (time->tm_min % 10);
        case 2: return ((time->tm_hour / 10) << 4) | (time->tm_hour % 10);
        case 3: return 0; // TODO: day of week
        case 4: return ((time->tm_mday / 10) << 4) | (time->tm_mday % 10);
        case 5: return ((time->tm_mon / 10) << 4) | (time->tm_mon % 10);
        default: return ((time->tm_year / 10) << 4) | (time->tm_year % 10);
    }
}

uint8_t I2c::readMcuRamData() {
    // Read from indexed MCU RAM data and increment always
    if (mcuRamIdx++ < 0xC8)
        return mcuRamData[mcuRamIdx - 1];
    return 0;
}

void I2c::writeMcuIrqMask(int i, uint8_t value) {
    // Write part of the MCU interrupt mask
    mcuIrqMask = (mcuIrqMask & ~(0xFF << (i << 3))) | (value << (i << 3));
    mcuInterrupt(0);
}

void I2c::writeMcuLcdPower(uint8_t value) {
    // Fake LCD power control by simply firing interrupts
    if (value & BIT(0)) mcuInterrupt(BIT(24) | BIT(26) | BIT(28)); // Power off
    if (value & BIT(1)) mcuInterrupt(BIT(25)); // LCD power on
    if (value & BIT(2)) mcuInterrupt(BIT(26)); // Bottom backlight off
    if (value & BIT(3)) mcuInterrupt(BIT(27)); // Bottom backlight on
    if (value & BIT(4)) mcuInterrupt(BIT(28)); // Top backlight off
    if (value & BIT(5)) mcuInterrupt(BIT(29)); // Top backlight on
}

void I2c::writeMcuRamIdx(uint8_t value) {
    // Write to the MCU RAM index register
    mcuRamIdx = value;
}

void I2c::writeMcuRamData(uint8_t value) {
    // Write to indexed MCU RAM data and increment if within bounds
    if (mcuRamIdx >= 0xC8) return;
    mcuRamData[mcuRamIdx++] = value;
    ramDirty = true;
}

uint8_t I2c::readCam(int i) {
    // Increment the camera address and stub reads for now
    uint16_t address = regAddrs[i / 2]++;
    LOG_WARN("Unknown I2C camera %d read from register 0x%X\n", i, address);
    return 0xFF;
}

void I2c::writeCam(int i, uint8_t value) {
    // Set the 16-bit camera address across two writes
    int j = i / 2;
    if (writeCounts[j] == 2) {
        regAddrs[j] = value;
        return;
    }
    else if (writeCounts[j] == 3) {
        regAddrs[j] = (regAddrs[j] << 8) | value;
        return;
    }

    // Increment the camera address and stub writes for now
    uint16_t address = regAddrs[j]++;
    LOG_WARN("Unknown I2C camera %d write to register 0x%X\n", i, address);
}

void I2c::writeBusData(int i, uint8_t value) {
    // Write to one of the I2C_BUS_DATA registers
    i2cBusData[i] = value;
}

void I2c::writeBusCnt(int i, uint8_t value) {
    // Write to one of the I2C_BUS_CNT registers and check some bits
    i2cBusCnt[i] = value & 0x7F;
    if (~value & BIT(7)) return; // Enable
    if (value & BIT(1)) writeCounts[i] = 0; // Start

    // Trigger an interrupt for the current bus if enabled
    if (value & BIT(6)) {
        uint8_t types[] = { 0x54, 0x55, 0x5C };
        core.interrupts.sendInterrupt(ARM11, types[i]);
    }

    // Forward I2C reads to the addressed device
    if (value & BIT(5)) { // Direction
        switch ((i << 8) | devAddrs[i]) {
            case 0x079: i2cBusData[i] = readCam(0); return;
            case 0x07B: i2cBusData[i] = readCam(1); return;
            case 0x14B: i2cBusData[i] = readMcu(); return;
            case 0x179: i2cBusData[i] = readCam(2); return;

        default:
            // Catch reads from unknown devices
            LOG_WARN("Unknown I2C bus %d read from device 0x%X\n", i, devAddrs[i]);
            i2cBusData[i] = 0;
            return;
        }
    }

    // Set the device address on first write
    i2cBusCnt[i] |= BIT(4); // Acknowledge
    if (++writeCounts[i] == 1) {
        devAddrs[i] = i2cBusData[i];
        return;
    }

    // Forward I2C writes to the addressed device
    switch ((i << 8) | devAddrs[i]) {
        case 0x078: return writeCam(0, i2cBusData[i]);
        case 0x07A: return writeCam(1, i2cBusData[i]);
        case 0x14A: return writeMcu(i2cBusData[i]);
        case 0x178: return writeCam(2, i2cBusData[i]);

    default:
        // Catch writes to unknown devices
        LOG_WARN("Unknown I2C bus %d write to device 0x%X\n", i, devAddrs[i]);
        return;
    }
}
