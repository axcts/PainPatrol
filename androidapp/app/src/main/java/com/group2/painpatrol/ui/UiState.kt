package com.group2.painpatrol.ui


// constant for the default acceptable ranges for each sensor (same as in wio)
val DEFAULT_THRESHOLDS = mapOf(
    "temperature" to 18f..25f,
    "humidity" to 30f..60f,
    "light" to 10f..60f,
    "sound" to 0f..50f
)
data class UiState(
    val readings: Map<String, String> = mapOf(),

    // maps a sensor name to a range of floats where both ends are included
    val thresholds: Map<String, ClosedFloatingPointRange<Float>> = DEFAULT_THRESHOLDS
)
