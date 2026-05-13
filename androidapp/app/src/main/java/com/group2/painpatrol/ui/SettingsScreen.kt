package com.group2.painpatrol.ui

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.group2.painpatrol.ui.theme.Purple80


// Settings screen where the range sliders are displayed
// uses a Column and loops over the map with thresholds to create one SensorRangeSlider per sensor
@Composable
fun SettingsScreen(modifier: Modifier = Modifier) {
    val uiState by AppViewModel.uiState.collectAsState()

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(24.dp)
    ) {
        Text(text = "Sensor Thresholds", fontSize = 20.sp)

        uiState.thresholds.forEach { (sensor, range) ->
            SensorRangeSlider(
                label = sensor.replaceFirstChar { it.uppercase() },
                range = range,
                valueRange = when (sensor) {
                    "temperature" -> 0f..50f
                    else -> 0f..100f
                },
                unit = when (sensor) {
                    "temperature" -> "°C"
                    else -> "%"
                },
                // save the slider position to viewmodel when user updates the range
                onRangeChange = { newRange ->
                    AppViewModel.updateThreshold(sensor, newRange)
                }
            )
        }
    }
}


@Composable
fun SensorRangeSlider(
    label: String,
    range: ClosedFloatingPointRange<Float>,
    valueRange: ClosedFloatingPointRange<Float>,
    unit: String,
    onRangeChange: (ClosedFloatingPointRange<Float>) -> Unit
) {
    var sliderPosition by remember { mutableStateOf(range) }

    Column {
        Text(text = "$label: ${sliderPosition.start.toInt()} - ${sliderPosition.endInclusive.toInt()}$unit")
        RangeSlider(
            value = sliderPosition,
            onValueChange = { range -> sliderPosition = range },
            valueRange = valueRange,
            onValueChangeFinished = {
                onRangeChange(sliderPosition)
            },
            colors = SliderDefaults.colors(
                thumbColor = Purple80,
                activeTrackColor = Purple80,
                inactiveTrackColor = Purple80.copy(alpha = 0.3f)
            )
        )
    }
}