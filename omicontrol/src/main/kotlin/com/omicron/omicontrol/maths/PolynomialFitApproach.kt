package com.omicron.omicontrol.maths

import org.apache.commons.math3.fitting.PolynomialCurveFitter
import org.apache.commons.math3.fitting.WeightedObservedPoint

/**
 * Calculates a mirror model by doing a least-squares curve fit on an N-th order polynomial using Apache Commons Math
 */
class PolynomialFitApproach(order: Int) : ModelApproach {
    private val fitter = PolynomialCurveFitter.create(order)

    override fun calculateModel(dataPoints: Collection<WeightedObservedPoint>): DoubleArray {
        return fitter.fit(dataPoints)
    }
}