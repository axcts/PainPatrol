package com.group2.painpatrol.ui

import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier

// Home Screen with sensor data
@Composable
fun HomeScreen(modifier: Modifier = Modifier) {
    val uiState by AppViewModel.uiState.collectAsState()
    Text(text = uiState.tempPayload, modifier = modifier)
}