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

// Home Screen with sensor data
@Composable
fun HomeScreen(modifier: Modifier = Modifier) {
    val uiState by AppViewModel.uiState.collectAsState()
    var popupState by remember { mutableStateOf(false) }
    val sensorStates: Map<String, List<String>> = mapOf(
        "Temperature" to listOf("Okay", "Too hot", "Too cold"),
        "Humidity" to listOf("Okay", "Too humid", "Too dry"),
        "Lighting" to listOf("Okay", "Too bright", "Too dim"),
        "Sound" to listOf("Okay", "Too loud", "Too quiet")
    )

    if (popupState) {
        Popup(dismiss = { popupState = false }, confirm = { popupState = false }, sensorStates)
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
                containerColor = Purple80,
                contentColor = Black
            ),
        ) {
            Text(text = "Mark readings as uncomfortable")
        }
    }
}

@Composable
fun Popup(dismiss: () -> Unit, confirm: () -> Unit, sensorStates: Map<String, List<String>>) {
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
            shape = RoundedCornerShape(16.dp)
        ) {
            Column(
                modifier = Modifier.padding(24.dp),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(modifier = Modifier.padding(bottom = 12.dp),
                     text = "Toggle uncomfortable readings:",
                     color = Black
                )

                for ((sensor, options) in sensorStates) {
                    val state: Int = states[sensor] ?: 0
                    var color: Color = if (state % 3 == 0) Green else Red

                    Button(
                        onClick = { states[sensor] = (state + 1) % 3 },
                        shape = RoundedCornerShape(8.dp),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = Color(0x00FFFFFF), // transparent
                            contentColor = Grey
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
                    onClick = confirm,
                    modifier = Modifier.fillMaxWidth().padding(top = 12.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Purple80,
                        contentColor = Black
                    ),
                ) {
                    Text("Confirm")
                }
            }
        }
    }
}