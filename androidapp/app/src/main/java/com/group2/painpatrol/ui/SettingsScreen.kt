package com.group2.painpatrol.ui

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.group2.painpatrol.ui.theme.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.Alignment


// Settings screen where the range sliders are displayed
// uses a Column and loops over the map with thresholds to create one SensorRangeSlider per sensor
@Composable
fun SettingsScreen(modifier: Modifier = Modifier) {
    val uiState by AppViewModel.uiState.collectAsState()

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(24.dp),
        verticalArrangement = Arrangement.spacedBy(50.dp)
    ) {
        Text(text = "Sensor Thresholds", fontSize = 24.sp)

        uiState.thresholds.forEach { (sensor, range) ->
            SensorRangeSlider(
                label = sensor.replaceFirstChar { it.uppercase() },
                range = range,
                valueRange = when (sensor) {
                    "temperature" -> 0f..50f // so that temp is not up till 100 degrees
                    else -> 0f..100f // other values are in %
                },
                unit = when (sensor) {
                    "temperature" -> "°C"
                    else -> "%"
                },
                // save the slider position to viewmodel when user updates the range
                onRangeChange = { newRange ->
                    AppViewModel.updateThreshold(sensor, newRange)
                },
                onReset = {
                    AppViewModel.resetThreshold(sensor)
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
    onRangeChange: (ClosedFloatingPointRange<Float>) -> Unit,
    onReset: () -> Unit
) {
    var sliderPosition by remember { mutableStateOf(range) }

    // change slider position when viewmodel resets the threshold
    LaunchedEffect(range) {
        sliderPosition = range
    }

    Column {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = "$label: ${sliderPosition.start.toInt()} - ${sliderPosition.endInclusive.toInt()}$unit",
                fontSize = 18.sp,
                fontWeight = FontWeight.Bold,
                color = PurpleGrey40
            )
            Button(
                onClick = { onReset() },
                colors = ButtonDefaults.buttonColors(containerColor = Purple40),
                shape = RoundedCornerShape(10.dp)
            ) {
                Text("Reset")
            }
        }
        RangeSlider(
            value = sliderPosition,
            onValueChange = { range -> sliderPosition = range },
            valueRange = valueRange,
            onValueChangeFinished = {
                onRangeChange(sliderPosition) // only update when user lifts their finger, not on every drag
            },
            colors = SliderDefaults.colors(
                thumbColor = Purple80,
                activeTrackColor = Purple80,
                inactiveTrackColor = Purple80.copy(alpha = 0.3f)
            )
        )
    }
}