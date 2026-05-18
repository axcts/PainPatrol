@file:OptIn(ExperimentalGridApi::class)

package com.group2.painpatrol.ui

import com.group2.painpatrol.ui.theme.*
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalGridApi
import androidx.compose.foundation.layout.Grid
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.compose.material3.MaterialTheme
import androidx.compose.ui.graphics.Color.Companion.Green
import androidx.compose.ui.graphics.Color.Companion.Red

// Home Screen with sensor data
@Composable
fun HomeScreen(modifier: Modifier = Modifier) {
    val uiState by AppViewModel.uiState.collectAsState()
    var popupState by remember { mutableStateOf(false) }
    val sensorStates = mapOf(
        "Temperature" to listOf("Okay", "Too hot", "Too cold"),
        "Humidity" to listOf("Okay", "Too humid", "Too dry"),
        "Lighting" to listOf("Okay", "Too bright", "Too dim"),
        "Sound" to listOf("Okay", "Too loud", "Too quiet")
    )

    if (popupState) {
        Popup(dismiss = { popupState = false }, sensorStates)
    }

    Column(
        modifier = modifier.fillMaxSize().padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Grid(config = {
            repeat(2) { column(160.dp) }
            repeat(2) { row(160.dp) }
            rowGap(16.dp)
            columnGap(13.dp)
        }) {
            ReadingDisplay("Temperature", uiState.readings["temperature"]?.let { "${it}C" } ?: "Fetching...")
            ReadingDisplay("Humidity", uiState.readings["humidity"]?.let { "$it%" } ?: "Fetching...")
            ReadingDisplay("Lighting", uiState.readings["lighting"]?.let { "$it%" } ?: "Fetching...")
            ReadingDisplay("Sound", uiState.readings["sound"]?.let { "$it%" } ?: "Fetching...")
        }

        Button(
            onClick = { popupState = true },
            modifier = Modifier.padding(16.dp).align(Alignment.Start),
            shape = RoundedCornerShape(12.dp),
            colors = ButtonDefaults.buttonColors(
                containerColor = MaterialTheme.colorScheme.primary,
                contentColor = MaterialTheme.colorScheme.onPrimary
            ),
        ) {
            Text(text = "Mark readings as uncomfortable")
        }
    }
}

@Composable
fun Popup(dismiss: () -> Unit, sensorStates: Map<String, List<String>>) {
    val uiState by AppViewModel.uiState.collectAsState()
    var thresholds: Map<String, ClosedFloatingPointRange<Float>> = uiState.thresholds
    var readings: Map<String, String> = uiState.readings
    // parallel of sensorStates - this one stores sensor and an integer that is either 0, 1, 2
    // to represent the ["okay", "too hot", "too cold"] from the list in sensorStates. sorry not very modular
    val states = remember {
        mutableStateMapOf(
            "temperature" to 0,
            "humidity" to 0,
            "lighting" to 0,
            "sound" to 0
        )
    }

    Dialog(onDismissRequest = dismiss) {
        Card(shape = RoundedCornerShape(16.dp)) {
            Column(
                modifier = Modifier.padding(24.dp),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(modifier = Modifier.padding(bottom = 12.dp),
                     text = "Toggle uncomfortable readings:",
                     color = MaterialTheme.colorScheme.secondary
                )

                // for every sensor create a button in the popup with the sensor name and state
                // (the state that is currently in the states map not the actual state of the sensor that is displayed)
                for ((sensor, options) in sensorStates) {
                    val state: Int = states[sensor.lowercase()] ?: 0
                    var color: Color = if (state % 3 == 0) Green10 else Red10

                    Button(
                        // on click increase the int in states by 1 then modulo 3 so it stays either 0 1 or 3
                        // since states is mutableStateOfMap it recompiles all composables that access it when the value of it changes
                        // so that makes the button rotate the okay -> too [smth] -> too [smth] values
                        onClick = { states[sensor.lowercase()] = (state + 1) % 3 },
                        shape = RoundedCornerShape(8.dp),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = Color(0x00FFFFFF), // transparent
                            contentColor = MaterialTheme.colorScheme.secondary
                        ),
                        modifier = Modifier.fillMaxWidth().padding(0.dp),
                        contentPadding = PaddingValues(vertical = 8.dp)
                    ) {
                        Text(text = "> $sensor: ", textAlign = TextAlign.Start, fontSize = 16.sp)
                        Text(modifier = Modifier.fillMaxWidth(),
                             text = options[state],
                             fontSize = 16.sp,
                             color = color)
                    }
                }

                Button(
                    onClick = {
                        adjustThresholds(states, thresholds, readings) // on click first adjust thresholds
                        dismiss() // then collapse the popup
                              },
                    modifier = Modifier.fillMaxWidth().padding(top = 12.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = MaterialTheme.colorScheme.primary,
                        contentColor = MaterialTheme.colorScheme.onPrimary
                    ),
                ) {
                    Text("Confirm")
                }
            }
        }
    }
}

fun adjustThresholds(states: Map<String, Int>, thresholds: Map<String, ClosedFloatingPointRange<Float>>, readings: Map<String, String>) {
    for ((sensor, range) in thresholds) {
        var state: Int = states[sensor] ?: 0

        if (readings[sensor] !== null) {
            var read: Float = readings[sensor]!!.toFloat()

            // only change ranges if the value thats being read is actually in the range
            if (range.start <= read && read <= range.endInclusive) {
                if (state == 1) {
                    AppViewModel.updateThreshold(sensor, range.start..read)

                } else if (state == 2) {
                    AppViewModel.updateThreshold(sensor, read..range.endInclusive)
                }
            }
        }
    }
}