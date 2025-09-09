/*
* mpg2km.h / Program in C
*
* Sooji Kim / CS5600 / Northeastern University
* Fall 2025 / Sep 8, 2025
*
* Header file for my conversion functions.
*/


/**
* @brief Checks whether a floating-point input is valid.
 *
 * @param input The floating-point number to validate.
 * @return int
 * - `1` if the input is valid (finite, not NaN, and non-negative).
 * - `0` otherwise.
 */
int isValid(float input);

/**
 * @brief Converts fuel efficiency from miles per gallon (MPG)
 *        to kilometers per liter (km/L).
 *
 * @param mpg Fuel efficiency in miles per gallon.
 * @return float
 * - Fuel efficiency in kilometers per liter.
 * - `-1.0` if input is invalid.
 */
float mpg2kml(float mpg);

/**
 * @brief Converts fuel efficiency from miles per gallon (MPG)
 *        to liters per 100 kilometers (L/100km).
 *
 * @param mpg Fuel efficiency in miles per gallon.
 * @return float
 * - Fuel consumption in liters per 100 kilometers.
 * - `-1.0` if input is invalid.
 */
float mpg2lphm(float mpg);

/**
 * @brief Converts fuel consumption from liters per 100 kilometers (L/100km)
 *        to miles per gallon (MPG).
 *
 * @param lphkm Fuel consumption in liters per 100 kilometers.
 * @return float
 * - Fuel efficiency in miles per gallon.
 * - `-1.0` if input is invalid.
 */
float lph2mpg(float lphkm);