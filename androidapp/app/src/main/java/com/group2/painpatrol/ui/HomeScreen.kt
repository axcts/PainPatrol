@file:OptIn(ExperimentalGridApi::class)

package com.group2.painpatrol.ui

import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.unit.dp
import androidx.compose.material3.*
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextAlign

// Home Screen with sensor data
@Composable
fun HomeScreen(modifier: Modifier = Modifier) {
    val uiState by AppViewModel.uiState.collectAsState()
    var showPopup by remember { mutableStateOf(false) }
    val sensorStates = mapOf(
        "Temperature" to listOf("Okay", "Too hot", "Too cold"),
        "Humidity" to listOf("Okay", "Too humid", "Too dry"),
        "Lighting" to listOf("Okay", "Too bright", "Too dim"),
        "Sound" to listOf("Okay", "Too loud", "Too quiet")
    )

    if (showPopup) {
        ShowMenu(
            dismiss = { showPopup = false },
            confirm = { showPopup = false },
            sensorStates = sensorStates
        )
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
            onClick = { showPopup = true },
            modifier = Modifier.padding(16.dp).align(Alignment.Start),
            shape = RoundedCornerShape(12.dp)
        ) {
            Text(text = "Mark readings as uncomfortable")
        }
    }
}

@Composable
fun ShowMenu(dismiss: () -> Unit, confirm: () -> Unit, sensorStates: Map<String, List<String>>) {
    val states = remember {
        mutableStateMapOf(
            "Temperature" to 0,
            "Humidity" to 0,
            "Lighting" to 0,
            "Sound" to 0
        )
    }

    Dialog(onDismissRequest = dismiss) {
        Card(
            modifier = Modifier.fillMaxWidth().padding(12.dp),
            shape = RoundedCornerShape(16.dp),
        ) {
            Column(
                modifier = Modifier.padding(24.dp),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(modifier = Modifier.padding(vertical = 2.dp), text = "Toggle uncomfortable readings:")

                for ((sensor, options) in sensorStates) {
                    val state = states[sensor] ?: 0

                    Row(
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 2.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(text = "$sensor:", textAlign = TextAlign.Start, color = Color(0x80000000))
                        Button(
                            onClick = { states[sensor] = (state + 1) % 3 },
                            shape = RoundedCornerShape(8.dp),
                            colors = ButtonDefaults.buttonColors(
                                containerColor = Color(0x00000000),
                                contentColor = Color(0x80000000)
                            )
                        ) {
                            Text(text = options[state], textAlign = TextAlign.End)
                        }
                    }
                }

                Button(
                    onClick = confirm,
                    modifier = Modifier.fillMaxWidth().padding(top = 12.dp)
                ) {
                    Text("Confirm")
                }
            }
        }
    }
}