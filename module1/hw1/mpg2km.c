/*
* mpg2km.c / Program in C
*
* Sooji Kim / CS5600 / Northeastern University
* Fall 2025 / Sep 8, 2025
*
* Library file for my conversion functions.
*/

#include <stdio.h>   /**< Standard I/O operations. */
#include <math.h> /**< Mathematical functions (e.g., isnan, isinf). */

/**
 * @name Fuel Conversion Constants
 * Conversion factors for unit conversions in fuel efficiency calculations.
 * @{
 */
const float MI_PER_KM = 0.621371; /**< Miles per kilometer. */
const float GAL_PER_L = 0.264172; /**< Gallons per liter. */
const float HUNDRED   = 100.0;    /**< Scaling factor for L/100km conversions. */
/** @} */

/**
 * @brief Checks whether a floating-point input is valid.
 *
 * This function determines if a given floating-point number is valid for use.
 * A value is considered invalid if it is:
 * - Infinite (`isinf(input)` returns true),
 * - NaN (`isnan(input)` returns true), or
 * - Negative (`input < 0.0`).
 *
 * @param input The floating-point number to validate.
 *
 * @return int
 * - `1` if the input is valid (finite, not NaN, and non-negative),
 * - `0` otherwise.
 *
 * @note This function uses `isinf()` and `isnan()` from `<math.h>`.
 *
 * @see isinf(), isnan()
 */
int isValid(float input) {
    if ( isinf(input) || isnan(input) || (input <= 0.0) ) {
        printf("Invalid input.\n");
        return 0;
    } else {
        return 1;
    }
}

/**
 * @brief Converts fuel efficiency from miles per gallon (MPG) to kilometers per liter (km/L).
 *
 * This function converts a given fuel efficiency in miles per gallon (MPG)
 * into kilometers per liter (km/L) using the conversion constants:
 * - MI_PER_KM: miles per kilometer
 * - GAL_PER_L: gallons per liter
 *
 * If the input value is invalid (negative, infinite, or NaN), the function
 * returns -1.0.
 *
 * @param mpg Fuel efficiency in miles per gallon.
 *
 * @return float
 * - Converted fuel efficiency in kilometers per liter.
 * - `-1.0` if input is invalid.
 *
 * @note Relies on the helper function isValid() for input validation.
 */
float mpg2kml(float mpg) {
    if (isValid(mpg)) {
        float res = mpg / MI_PER_KM;
        res = res * GAL_PER_L;
        return res;
    } else {
        return -1.0;
    }
}

/**
 * @brief Converts fuel efficiency from miles per gallon (MPG) to liters per 100 kilometers (L/100km).
 *
 * This function converts a given fuel efficiency in miles per gallon (MPG)
 * into liters per 100 kilometers (L/100km), a common European fuel efficiency unit.
 *
 * If the input value is invalid (negative, infinite, or NaN), the function
 * returns -1.0.
 *
 * @param mpg Fuel efficiency in miles per gallon.
 *
 * @return float
 * - Converted fuel efficiency in liters per 100 kilometers.
 * - `-1.0` if input is invalid.
 *
 * @note Uses mpg2kml() for conversion to intermediate km/L value.
 * @see mpg2kml()
 */
float mpg2lphm(float mpg) {
    if (isValid(mpg)) {
        float res = 1.0 / mpg2kml(mpg);
        res *= HUNDRED;
        return res;
    } else {
        return -1.0;
    }
}

/**
 * @brief Converts fuel efficiency from liters per 100 kilometers (L/100km) to miles per gallon (MPG).
 *
 * This function converts a given fuel consumption value in liters per 100 kilometers (L/100km)
 * into miles per gallon (MPG), commonly used in the U.S.
 *
 * If the input value is invalid (negative, infinite, or NaN), the function
 * returns -1.0.
 *
 * @param lphkm Fuel consumption in liters per 100 kilometers.
 *
 * @return float
 * - Converted fuel efficiency in miles per gallon.
 * - `-1.0` if input is invalid.
 *
 * @note Uses MI_PER_KM and GAL_PER_L for unit conversion.
 */
float lph2mpg(float lphkm) {
    if (isValid(lphkm)) {
        float res = lphkm / HUNDRED;
        res *= ( 1.0 / MI_PER_KM );
        res *= GAL_PER_L;
        res = 1.0 / res;
        return res;
    } else {
        return -1.0;
    }
}