#include "thermistor.h"
#include <cmath>

namespace hydra::thermal {

Thermistor::Thermistor() : coeff_{} {}
Thermistor::Thermistor(const Coefficients& coeff) : coeff_(coeff) {}

double Thermistor::adc_to_celsius(uint16_t adc_raw) const {
    if (!is_valid(adc_raw)) return -999.0;

    /* Voltage divider: R_therm = R_pullup * adc_raw / (adc_max - adc_raw) */
    double r = coeff_.pullup_resistance * static_cast<double>(adc_raw)
             / (coeff_.adc_max - static_cast<double>(adc_raw));

    /* Steinhart-Hart equation: 1/T = A + B*ln(R) + C*ln(R)^3 */
    double ln_r = std::log(r);
    double inv_t = coeff_.a + coeff_.b * ln_r + coeff_.c * ln_r * ln_r * ln_r;

    /* Convert Kelvin to Celsius */
    return (1.0 / inv_t) - 273.15;
}

bool Thermistor::is_valid(uint16_t adc_raw) const {
    /* Open circuit (thermistor disconnected) or short circuit */
    return adc_raw > 10 && adc_raw < static_cast<uint16_t>(coeff_.adc_max - 10);
}

} // namespace hydra::thermal
