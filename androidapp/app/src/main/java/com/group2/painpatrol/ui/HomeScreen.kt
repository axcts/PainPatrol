@file:OptIn(ExperimentalGridApi::class)

package com.group2.painpatrol.ui

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.ExperimentalGridApi
import androidx.compose.foundation.layout.Grid
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp


// Home Screen with sensor data
@Composable
fun HomeScreen(modifier: Modifier = Modifier) {
    val uiState by AppViewModel.uiState.collectAsState()

    Box(
        modifier = Modifier.fillMaxSize().padding(0.dp, 50.dp),
        contentAlignment = Alignment.TopCenter

    ){ // if we happen to have more than the sensors we have now, we can turn this into a lazy grid
        // and have a foreach on the map
        Grid(config = {
            repeat(2) {
                column(160.dp)
            }
            repeat(2){
                row(160.dp)
            }
            rowGap(16.dp)
            columnGap(13.dp)

        }) {

            // Init boxes for sensor readings
            ReadingDisplay(
                "Temperature",
                if (uiState.readings["temperature"] != null)
                            uiState.readings["temperature"] + "C"
                        else "Fetching..."
            )
            ReadingDisplay(
                "Humidity",
                if (uiState.readings["humidity"] != null)
                            uiState.readings["humidity"] + "%"
                        else "Fetching..."
            )
            ReadingDisplay(
                "Lighting",
                if (uiState.readings["lighting"] != null)
                            uiState.readings["lighting"] + "%"
                        else "Fetching..."
            )
            ReadingDisplay(
                "Sound",
                if (uiState.readings["sound"] != null)
                            uiState.readings["sound"] + "%"
                        else "Fetching..."
            )
        }
    }


}