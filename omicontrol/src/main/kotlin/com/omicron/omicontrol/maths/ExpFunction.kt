package com.omicron.omicontrol.maths

import org.apache.commons.math3.analysis.ParametricUnivariateFunction
import org.apache.commons.math3.util.FastMath

/**
 * Implementation of a standard exponential function:
 * f(x) = a * exp(b * x)
 */
class ExpFunction : ParametricUnivariateFunction {
    override fun value(x: Double, vararg parameters: Double): Double {
        // most other apache implementations of functions use FastMath, so we will as well
        val a = parameters[0]
        val b = parameters[1]
        return a * FastMath.exp(b * x)
    }

    override fun gradient(x: Double, vararg parameters: Double): DoubleArray {
        // derivative is in the form: abe^(bx), or ab * exp(b * x) in code form
        val a = parameters[0]
        val b = parameters[1]
        // TODO FIND OUT WHY IT WANTS A VECTOR DERIVATIVE
        return doubleArrayOf()
        //return a * b * FastMath.exp(b * x)
    }
}