package com.omicron.omicontrol

import javafx.scene.paint.Color
import javafx.scene.text.FontWeight
import tornadofx.Stylesheet
import tornadofx.box
import tornadofx.cssclass
import tornadofx.px

class Styles : Stylesheet() {
    companion object {
        val appMenuButton by cssclass()
        val titleLabel by cssclass()
        val rect by cssclass()
        val paddedBox by cssclass()
        val bigLabel by cssclass()
    }

    init {
        label {
            fontSize = 20.px
        }

        bigLabel {
            fontSize = 22.px
        }

        button {
            fontSize = 20.px
            padding = box(12.px)
            minWidth = 128.px
        }

        paddedBox {
            padding = box(5.px)
        }

        titleLabel{
            fontSize = 36.px
            fontWeight = FontWeight.EXTRA_BOLD
        }

        progressIndicator {
            // TODO doesn't work, want to fill it in white
            fill = Color.WHITE
        }

        rect {
            stroke = Color.BLACK
            strokeWidth = 5.px
            fill = Color.TRANSPARENT
        }

        appMenuButton {
            fontSize = 25.px
            padding = box(40.px)
            minWidth = 256.px
            tabMinWidth = 256.px
        }
    }
}