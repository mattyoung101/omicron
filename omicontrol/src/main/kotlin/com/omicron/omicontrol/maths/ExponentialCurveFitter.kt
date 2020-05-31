package com.omicron.omicontrol.maths

import org.apache.commons.math3.fitting.AbstractCurveFitter
import org.apache.commons.math3.fitting.WeightedObservedPoint
import org.apache.commons.math3.fitting.leastsquares.LeastSquaresBuilder
import org.apache.commons.math3.fitting.leastsquares.LeastSquaresFactory
import org.apache.commons.math3.fitting.leastsquares.LeastSquaresProblem

class ExponentialCurveFitter : AbstractCurveFitter() {
    override fun getProblem(points: MutableCollection<WeightedObservedPoint>): LeastSquaresProblem {
        // TODO copy approach used by PolynomialCurveFitter
        return LeastSquaresBuilder().build()
    }
}