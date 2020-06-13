package com.omicron.omicontrol.maths

import org.apache.commons.math3.analysis.function.Exp
import org.apache.commons.math3.fitting.WeightedObservedPoint
import org.tinylog.kotlin.Logger

/**
 * Calculates a mirror model by doing a least-squares regression to fit an [ExpFunction], using
 * Apache Commons Math.
 *
 * The form of an exp function is: f(x) = ae^(bx)
 */
class ExponentialFitApproach : ModelApproach {
    private val fitter = ExponentialCurveFitter()

    override fun calculateModel(dataPoints: Collection<WeightedObservedPoint>): DoubleArray {
        Logger.debug("Calculating an exponential model for ${dataPoints.size} points")
        return fitter.fit(dataPoints)
    }

    override fun formatFunction(coefficients: DoubleArray): String {
        return "${coefficients[0]} * exp(${coefficients[1]} * x)"
    }

    override fun evaluate(x: Double, coefficients: DoubleArray): Double {
        return ExpFunction().value(x, *coefficients)
    }
}