#include "ADS1015.h"

//Write a 16-bits value to the specified destination register
void ADS1015::I2CWriteRegister(uint8_t reg, uint16_t value) {
    char buf[3];
    buf[0] = reg;
    buf[1] = (char) (value >> 8);
    buf[2] = (char) (value & 0xFF);
    bcm2835_i2c_write(buf, 3);
}

//Read 16-bits value to the specified register
uint16_t ADS1015::I2CReadReg16(uint8_t reg) {
    char buf[3];
    char current = reg;
    bcm2835_i2c_write(&current, 1);
    usleep(10000);
    uint8_t r = bcm2835_i2c_read(buf, 2);
    uint16_t result = (buf[0] << 8 | buf[1]);
    //printf("Resp: 0x%02X - 0x%02X%02X", r, buf[0], buf[1]);
    return result;
}

//ADS1015 constructor
ADS1015::ADS1015(uint8_t i2cAddress) {
    m_i2cAddress = i2cAddress;
    m_conversionDelay = ADS1015_CONVERSION_DELAY;
    m_bitShift = 4;
    m_gain = GAIN_TWOTHIRDS;
}

//ADS1115 constructor
ADS1115::ADS1115(uint8_t i2cAddress) {
    m_i2cAddress = i2cAddress;
    m_conversionDelay = ADS1115_CONVERSION_DELAY;
    m_bitShift = 0;
    m_gain = GAIN_TWOTHIRDS;
}

//Set ADC the gain
void ADS1015::setGain(adsGain_t gain) {
    m_gain = gain;
}

//Get the ADC gain
adsGain_t ADS1015::getGain() {
    return m_gain;
}

int16_t ADS1015::getSignedValue(uint16_t value) const {
    if (m_bitShift == 0) {
        return (int16_t) value;
    } else {
        // Shift 12-bit results right 4 bits for the ADS1015,
        // making sure we keep the sign bit intact
        if (value > 0x07FF) {
            // negative number - extend the sign to 16th bit
            value |= 0xF000;
        }
        return (int16_t) value;
    }
}

//Gets a single-ended ADC reading from the specified input channel
uint16_t ADS1015::readADC_SingleEnded(uint8_t channel) {
    if (channel > 3) {
        return 0;
    }

    uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE    |
                      ADS1015_REG_CONFIG_CLAT_NONLAT  |
                      ADS1015_REG_CONFIG_CPOL_ACTVLOW |
                      ADS1015_REG_CONFIG_CMODE_TRAD   |
                      ADS1015_REG_CONFIG_DR_1600SPS   |
                      ADS1015_REG_CONFIG_MODE_SINGLE  |
					  m_gain;   						

    switch (channel) {
        case (0):
            config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
            break;
        case (1):
            config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
            break;
        case (2):
            config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
            break;
        case (3):
            config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
            break;
    }

    config |= ADS1015_REG_CONFIG_OS_SINGLE;

    bcm2835_i2c_begin();
    bcm2835_i2c_set_baudrate(100000);
    bcm2835_i2c_setSlaveAddress(m_i2cAddress);

    I2CWriteRegister(ADS1015_REG_POINTER_CONFIG, config);

    delay(m_conversionDelay); // Wait for the conversion delay to complete

    // Read the conversion results and shift 12-bit results right 4 bits for the ADS1015
    uint16_t adcValue = I2CReadReg16(ADS1015_REG_POINTER_CONVERT) >> m_bitShift;

    bcm2835_i2c_end();

    return adcValue;
}

/*
    Reads the difference between the P (AIN0) and N (AIN1) input.
	Generates a signed value since the difference can be either
	positive or negative.
*/
int16_t ADS1015::readADC_Differential_0_1() {
    uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE		|
                      ADS1015_REG_CONFIG_CLAT_NONLAT	|
                      ADS1015_REG_CONFIG_CPOL_ACTVLOW	|
                      ADS1015_REG_CONFIG_CMODE_TRAD		|
                      ADS1015_REG_CONFIG_DR_1600SPS		|
                      ADS1015_REG_CONFIG_MODE_SINGLE;    

    
    config |= m_gain; 							// Set the PGA gain
    config |= ADS1015_REG_CONFIG_MUX_DIFF_0_1;  // Set channels AIN0 = P, AIN1 = N
    config |= ADS1015_REG_CONFIG_OS_SINGLE;

    I2CWriteRegister(ADS1015_REG_POINTER_CONFIG, config);

    delay(m_conversionDelay);

    uint16_t res = I2CReadReg16(ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
	
    return getSignedValue(res);
}

/*
    Reads the difference between the P (AIN2) and N (AIN3) input.
	Generates a signed value since the difference can be either
	positive or negative.
*/
int16_t ADS1015::readADC_Differential_2_3() {
    uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE		|
                      ADS1015_REG_CONFIG_CLAT_NONLAT	|
                      ADS1015_REG_CONFIG_CPOL_ACTVLOW	|
                      ADS1015_REG_CONFIG_CMODE_TRAD		|
                      ADS1015_REG_CONFIG_DR_1600SPS		|
                      ADS1015_REG_CONFIG_MODE_SINGLE;    

    
    config |= m_gain; 							// Set the PGA gain
    config |= ADS1015_REG_CONFIG_MUX_DIFF_2_3;  // Set channels AIN2 = P, AIN3 = N
    config |= ADS1015_REG_CONFIG_OS_SINGLE;

    I2CWriteRegister(ADS1015_REG_POINTER_CONFIG, config);
	
    delay(m_conversionDelay);

    uint16_t res = I2CReadReg16(ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
	
    return getSignedValue(res);
}

/*
    Sets up the comparator to operate in basic mode, causing the
	ALERT/RDY pin to assert (go from high to low) when the ADC
	value exceeds the specified threshold.
	This will also set the ADC in continuous conversion mode.
*/
void ADS1015::startComparator_SingleEnded(uint8_t channel, int16_t threshold) {
    if (channel > 3) {
        return;
    }
	
	uint16_t config = ADS1015_REG_CONFIG_CQUE_1CONV		|
                      ADS1015_REG_CONFIG_CLAT_LATCH		|
                      ADS1015_REG_CONFIG_CPOL_ACTVLOW	|
                      ADS1015_REG_CONFIG_CMODE_TRAD		|
                      ADS1015_REG_CONFIG_DR_1600SPS		|
                      ADS1015_REG_CONFIG_MODE_CONTIN	|
                      ADS1015_REG_CONFIG_MODE_CONTIN	|
					  m_gain;

    switch (channel) {
        case (0):
            config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
            break;
        case (1):
            config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
            break;
        case (2):
            config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
            break;
        case (3):
            config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
            break;
    }

    I2CWriteRegister(ADS1015_REG_POINTER_HITHRESH, threshold << m_bitShift);
    I2CWriteRegister(ADS1015_REG_POINTER_CONFIG, config);
}

/*
    In order to clear the comparator, we need to read the
	conversion results.  This function reads the last conversion
	results without changing the config value.
*/
int16_t ADS1015::getLastConversionResults() {
    delay(m_conversionDelay);

    uint16_t res = I2CReadReg16(ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
    return getSignedValue(res);
}