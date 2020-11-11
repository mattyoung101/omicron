/*
 * This file is part of the Omicontrol project.
 * Copyright (c) 2019-2020 Team Omicron. All rights reserved.
 *
 * Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
 * James Talkington, Matt Young.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package com.omicron.omicontrol.maths

import org.apache.commons.math3.analysis.function.Multiply
import org.apache.commons.math3.analysis.function.Power
import org.apache.commons.math3.fitting.AbstractCurveFitter
import org.apache.commons.math3.fitting.WeightedObservedPoint
import org.apache.commons.math3.fitting.leastsquares.LeastSquaresBuilder
import org.apache.commons.math3.fitting.leastsquares.LeastSquaresProblem
import org.apache.commons.math3.linear.DiagonalMatrix

/**
 * This is a curve fitter implementation for an exponential model given by f(x) = a * exp(b * x)
 *
 * Based on the PolynomialCurveFitter from Apache Commons Math
 */
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
        val function = ExpFunction()
        val initialGuess = doubleArrayOf(0.0, 0.0)
        val model = TheoreticalValuesFunction(function, points)

        // and build the least squares problem
        return LeastSquaresBuilder()
            .maxEvaluations(Integer.MAX_VALUE)
            .maxIterations(Integer.MAX_VALUE)
            .start(initialGuess)
            .target(target)
            .weight(DiagonalMatrix(weights))
            .model(model.modelFunction, model.modelFunctionJacobian)
            .build()
    }
}