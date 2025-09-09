/*
* SoojiK.CS5600.LearnC.c / Program in C
*
* Sooji Kim / CS5600 / Northeastern University
* Fall 2025 / Sep 8, 2025
*
* Program for some conversions between miles and gallons to kilometers
* and liters for fuel efficiency and fuel consumption.
*/

#include <stdio.h>   /**< Standard I/O operations. */
#include <stdlib.h>  /**< Standard library functions (e.g., exit). */
#include <math.h> /**< Mathematical functions (e.g., isnan, isinf). */
#include "mpg2km.h"

/**
 * @brief Program entry point.
 *
 * Runs test cases for both valid and invalid inputs
 * using the fuel efficiency conversion functions.
 *
 * @return int
 * Returns 0 upon successful execution.
 */
int main() {

    // test cases for "good" inputs
    printf("Test cases for \"good\" inputs:\n");
    printf("24 miles per gallon is %.2f kilometers per liter. (Expected output: 10.20) \n", mpg2kml(24.0));
    printf("15 miles per gallon is %.2f liters per 100 kilometers. (Expected output: 15.68) \n", mpg2lphm(15.0));
    printf("19 liters per 100 kilometers is %.2f miles per gallon. (Expected output: 12.38) \n", lph2mpg(19.0));

    printf("\n");

    // test cases for "bad" inputs
    printf("Test cases for \"bad\" inputs:\n");
    printf("mpg2kml(INF) -> %.2f (Expected output: -1.00)\n", mpg2kml(INFINITY));
    printf("mpg2lphm(NAN) -> %.2f (Expected output: -1.00)\n", mpg2lphm(NAN));
    printf("lph2mpg(-7.5) -> %.2f (Expected output: -1.00)\n", lph2mpg(-7.5f));
    printf("mpg2kml(-0.0) -> %.2f (Expected output: -1.00)\n", mpg2kml(-0.0f));
    printf("lph2mpg(-INFINITY) -> %.2f (Expected output: -1.00)\n", lph2mpg(-INFINITY));

    return 0;
}