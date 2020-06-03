package com.omicron.omicontrol.maths

import org.apache.commons.math3.analysis.polynomials.PolynomialFunction
import org.apache.commons.math3.fitting.PolynomialCurveFitter
import org.apache.commons.math3.fitting.WeightedObservedPoint
import org.tinylog.kotlin.Logger

/**
 * Calculates a mirror model by doing a least-squares curve fit on an N-th order polynomial using Apache Commons Math
 * @param order the order of the polynomial (number of coefficients)
 */
class PolynomialFitApproach(private val order: Int) : ModelApproach {
    private val fitter = PolynomialCurveFitter.create(order)

    override fun calculateModel(dataPoints: Collection<WeightedObservedPoint>): DoubleArray {
        Logger.debug("Calculating a model for ${dataPoints.size} data points with an order of $order")
        return fitter.fit(dataPoints)
    }

    override fun formatFunction(coefficients: DoubleArray): String {
        return PolynomialFunction(coefficients).toString()
    }
}