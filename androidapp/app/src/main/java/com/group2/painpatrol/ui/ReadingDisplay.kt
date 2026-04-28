package com.group2.painpatrol.ui


import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

// Boxes with sensor readings
@Composable
fun ReadingDisplay(label: String, modifier: Modifier = Modifier, /*bgColor: Color*/) {
    Box(
        contentAlignment = Alignment.Center,
        modifier =
            modifier
                .fillMaxSize()
                .clip(shape = CardDefaults.shape)
                .background(Color(0xFFD0BCFF)) // Temp, color of box
    )
    {
        Column(modifier = Modifier.fillMaxSize(), verticalArrangement = Arrangement.Center, horizontalAlignment = Alignment.CenterHorizontally) {
            Text(text = label, fontSize = 20.sp, modifier = Modifier.padding(8.dp))
            val uiState by AppViewModel.uiState.collectAsState()    // Sensor readings from broker
            Text(text = uiState.tempPayload, fontSize = 20.sp, modifier = Modifier.padding(8.dp))

        }


    }
}
