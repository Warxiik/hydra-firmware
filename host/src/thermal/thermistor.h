#pragma once

#include <cstdint>

namespace hydra::thermal {

/**
 * Thermistor ADC-to-temperature conversion.
 * Uses the Steinhart-Hart equation with NTC 100k thermistor coefficients.
 */
class Thermistor {
public:
    struct Coefficients {
        double a = 0.0007343140544;   /* Steinhart-Hart A */
        double b = 0.0002157437229;   /* Steinhart-Hart B */
        double c = 0.0000000951568577; /* Steinhart-Hart C */
        double pullup_resistance = 4700.0; /* Pull-up resistor (ohms) */
        double adc_max = 4095.0;      /* 12-bit ADC max value */
    };

    Thermistor();
    explicit Thermistor(const Coefficients& coeff);

    /** Convert raw 12-bit ADC reading to temperature in Celsius */
    double adc_to_celsius(uint16_t adc_raw) const;

    /** Check if reading is valid (not open circuit or short) */
    bool is_valid(uint16_t adc_raw) const;

private:
    Coefficients coeff_;
};

} // namespace hydra::thermal
