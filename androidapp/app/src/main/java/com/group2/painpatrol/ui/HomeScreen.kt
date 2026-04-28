@file:OptIn(ExperimentalGridApi::class)

package com.group2.painpatrol.ui

import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.ExperimentalGridApi
import androidx.compose.foundation.layout.Grid
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.group2.painpatrol.ui.theme.PainpatrolTheme


// Home Screen with sensor data
@Composable
fun HomeScreen(modifier: Modifier = Modifier) {
    val uiState by AppViewModel.uiState.collectAsState()
    //Text(text = uiState.tempPayload, modifier = modifier)

    Box(
        modifier = Modifier.fillMaxSize().padding(0.dp, 50.dp),
        contentAlignment = Alignment.TopCenter

    ){
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
            ReadingDisplay("Temperature", )
            ReadingDisplay("Humidity", )
            ReadingDisplay("Light", )
            ReadingDisplay("Sound", )
        }
    }


}