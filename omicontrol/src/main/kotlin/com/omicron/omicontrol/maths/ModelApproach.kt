package com.omicron.omicontrol.maths

import org.apache.commons.math3.fitting.WeightedObservedPoint

/** Encapsulates an approach to calculating a mirror model, for example by using a polynomial or by an exponential. */
interface ModelApproach {
    /**
     * Performs a least squares regression to optimise the coefficients for the specified function.
     * @return the list of coefficients for the specified model
     */
    fun calculateModel(dataPoints: Collection<WeightedObservedPoint>): DoubleArray

    /**
     * Formats the list of coefficients for the function in a mathematically proper format
     */
    fun formatFunction(coefficients: DoubleArray): String

    /**
     * Evaluates the model function at the specified point
     */
    fun evaluate(x: Double, coefficients: DoubleArray): Double
}