package com.group2.painpatrol.ui

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.patrykandpatrick.vico.compose.cartesian.CartesianChartHost
import com.patrykandpatrick.vico.compose.cartesian.axis.HorizontalAxis
import com.patrykandpatrick.vico.compose.cartesian.axis.VerticalAxis
import com.patrykandpatrick.vico.compose.cartesian.data.CartesianChartModelProducer
import com.patrykandpatrick.vico.compose.cartesian.data.lineSeries
import com.patrykandpatrick.vico.compose.cartesian.layer.rememberLineCartesianLayer
import com.patrykandpatrick.vico.compose.cartesian.rememberCartesianChart


val channels = listOf("Temperature", "Humidity", "Light", "Sound")
@Preview(showSystemUi = true)
@Composable
fun StatisticScreen(modifier: Modifier = Modifier) {
    val uiState by AppViewModel.uiState.collectAsState()
    val modelProducer = remember { CartesianChartModelProducer() }
    var expanded by remember { mutableStateOf(false) }
    var selectedChannel by remember { mutableStateOf(channels[0]) }

    // Dummy data for now
    LaunchedEffect(Unit) {
        modelProducer.runTransaction {
            lineSeries { series(13, 8, 7, 12, 0, 1, 15, 14, 0, 11, 6, 12, 0, 11, 12, 11) }
        }
    }
    Column(modifier = modifier.fillMaxSize().padding(16.dp)) {
        Row (modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)){
            Box {   // Dropdown menus for sensor types
                OutlinedTextField(
                    value = selectedChannel,
                    onValueChange = {},
                    readOnly = true,
                    label = { Text("Sensor") },
                    trailingIcon = {
                        Icon(
                            imageVector = if (expanded) Icons.Default.KeyboardArrowUp else Icons.Default.KeyboardArrowDown,
                            contentDescription = "Toggle dropdown menu"
                        )
                    },
                    modifier = Modifier.fillMaxWidth()
                )
                Box(
                    modifier = Modifier
                        .matchParentSize()
                        .clickable { expanded = !expanded }     // Extra Box as otherwise it would register the click on the textfield not the dropdown...?
                )
                DropdownMenu(
                    expanded = expanded,
                    onDismissRequest = { expanded = false },
                ) {
                    channels.forEach { channel ->
                        DropdownMenuItem(
                            text = { Text(channel) },
                            onClick = {
                                selectedChannel = channel
                                expanded = false
                            }
                        )
                    }
                }

            }
        }
        Box(
            modifier = modifier
                .fillMaxSize()
                .padding(10.dp, 50.dp),
            //contentAlignment = Alignment.TopCenter
        ) {

            CartesianChartHost(
                rememberCartesianChart(
                    rememberLineCartesianLayer(),
                    startAxis = VerticalAxis.rememberStart(),
                    bottomAxis = HorizontalAxis.rememberBottom(),
                ),
                modelProducer = modelProducer, modifier = Modifier.fillMaxSize()
            )
        }
    }
}