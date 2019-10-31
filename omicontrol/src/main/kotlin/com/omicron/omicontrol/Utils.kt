package com.omicron.omicontrol

import javafx.util.Duration
import tornadofx.View
import tornadofx.ViewTransition

object Utils {
    fun transitionMetro(from: View, to: View){
        from.replaceWith(to, transition = ViewTransition.Metro(Duration.seconds(1.5)))
    }
}