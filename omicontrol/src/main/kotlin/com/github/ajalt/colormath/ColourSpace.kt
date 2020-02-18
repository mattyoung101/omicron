package com.github.ajalt.colormath

interface ColourSpace {
    /** the name of the colour space, should be also equal to the number of channels **/
    abstract val friendlyName: String
    /** the minimum values for each channel **/
    abstract val minRange: IntArray
    /** the maximum values for each channel **/
    abstract val maxRange: IntArray
}