package com.omicron.omicontrol.maths

import org.apache.commons.math3.analysis.function.Exp
import org.apache.commons.math3.fitting.WeightedObservedPoint

/**
 * Calculates a mirror model by doing a least-squares regression to fit an exponential function, using
 * Apache Commons Math.
 */
class ExponentialFitApproach : ModelApproach {
    override fun calculateModel(dataPoints: Collection<WeightedObservedPoint>): DoubleArray {
        // TODO add model calculations here
        return doubleArrayOf()
    }

    override fun formatFunction(coefficients: DoubleArray): String {
        return "NOT YET IMPLEMENTED"
    }
}