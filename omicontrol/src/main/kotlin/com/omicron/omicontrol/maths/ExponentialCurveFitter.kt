package com.omicron.omicontrol.maths

import org.apache.commons.math3.analysis.function.Multiply
import org.apache.commons.math3.analysis.function.Power
import org.apache.commons.math3.fitting.AbstractCurveFitter
import org.apache.commons.math3.fitting.WeightedObservedPoint
import org.apache.commons.math3.fitting.leastsquares.LeastSquaresBuilder
import org.apache.commons.math3.fitting.leastsquares.LeastSquaresProblem

class ExponentialCurveFitter : AbstractCurveFitter() {
    override fun getProblem(points: MutableCollection<WeightedObservedPoint>): LeastSquaresProblem {
        // initialise points to supply to least squares regression
        val len = points.size
        val target = DoubleArray(len)
        val weights = DoubleArray(len)

        for ((i, obs) in points.withIndex()) {
            target[i] = obs.y
            weights[i] = obs.weight
        }

        // set up the value function
        //val model = AbstractCurveFitter.TheoreticalValuesFunction()

        // TODO finish this

        return LeastSquaresBuilder().build()
    }
}