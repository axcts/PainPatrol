package com.group2.painpatrol.ui

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.DateRange
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowUp
import androidx.compose.material3.DatePickerDialog
import androidx.compose.material3.DateRangePicker
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.rememberDateRangePickerState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
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
import java.text.SimpleDateFormat
import java.util.Date

val channels = listOf("temperature", "humidity", "lighting", "sound")

@Preview(showSystemUi = true)
@Composable
fun StatisticScreen(modifier: Modifier = Modifier) {
    val uiState by AppViewModel.uiState.collectAsState()
    val modelProducer = remember { CartesianChartModelProducer() }
    var expanded by remember { mutableStateOf(false) }
    var selectedChannel by remember { mutableStateOf(channels[0]) }

    var readingHistory: Map<Long, Map<String, Float>> = AppViewModel.loadReadings()

    var showDatePicker by remember { mutableStateOf(false) }
    val datePickerState = rememberDateRangePickerState()

    // values after user confirms range
    var startUnix by remember { mutableStateOf<Long?>(null) }
    var endUnix   by remember { mutableStateOf<Long?>(null) }

    var filteredTimestamps by remember { mutableStateOf<List<Long>>(emptyList()) }

    // date formats for displaying
    val rangeDateFormat = remember { SimpleDateFormat("dd/MM/yyyy") }
    val axisDateFormat = remember { SimpleDateFormat("dd/MM") }


    // to make the datepicker box display chosen range
    val dateRangeLabel = when {
        startUnix != null && endUnix != null ->
            "${rangeDateFormat.format(Date(startUnix!! * 1000L))} – ${rangeDateFormat.format(Date(endUnix!! * 1000L))}"     // !! to force since it wont ever be null
        startUnix != null ->
            "${rangeDateFormat.format(Date(startUnix!! * 1000L))} – ?"
        else -> "Select range"
    }

    LaunchedEffect(selectedChannel, startUnix, endUnix, readingHistory) {

        val filteredReadings = readingHistory
            .entries
            .filter { (timestamp, _) ->     // get values within range
                val start = startUnix ?: Long.MIN_VALUE
                val end   = endUnix   ?: Long.MAX_VALUE
                timestamp in start..end
            }
            .sortedBy { (timestamp, _) -> timestamp }       // so timestamps are 100% chronological
        
        filteredTimestamps = filteredReadings.map { (timestamp, _) -> timestamp }  // so x-axis labels can display the dates of the readings

        val values = filteredReadings.mapNotNull { (_, readings) -> readings[selectedChannel] }

        modelProducer.runTransaction {
            if (values.isNotEmpty()){
                lineSeries { series(values) }
            }
            else {
                lineSeries {series(listOf(0f)) } // init empty value set, so it still renders with no values available
            }
        }
    }
    Column(modifier = modifier
        .fillMaxSize()
        .padding(16.dp)) {
        Row (modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp)){
            Box (modifier = Modifier.weight(1f)){   // Dropdown menus for sensor types
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
                        .clickable { expanded = !expanded }
                )
                DropdownMenu(
                    expanded = expanded,
                    onDismissRequest = { expanded = false },
                ) {
                    channels.forEach { channel ->
                        DropdownMenuItem(
                            text = { Text(channel.replaceFirstChar { it.uppercase() }) },
                            onClick = {
                                selectedChannel = channel
                                expanded = false
                            }
                        )
                    }
                }

            }
            Box (modifier = Modifier.weight(1f)) {   // Dropdown menus for date range
                OutlinedTextField(
                    value = dateRangeLabel,
                    onValueChange = {},
                    readOnly = true,
                    label = { Text("Date Range") },
                    trailingIcon = {
                        Icon(
                            imageVector = Icons.Default.DateRange,
                            contentDescription = "Choose date range"
                        )
                    },
                    modifier = Modifier.fillMaxWidth()
                )
                Box(
                    modifier = Modifier
                        .matchParentSize()
                        .clickable { showDatePicker = true }
                )


            }
        }
        // Date picker operation
        if (showDatePicker){
            DatePickerDialog(
                onDismissRequest = { showDatePicker = false },
                confirmButton = {
                    TextButton(
                        onClick = {
                            startUnix = datePickerState.selectedStartDateMillis?.let { it / 1000L }
                            endUnix = datePickerState.selectedEndDateMillis?.let { it / 1000L + 86399L }
                            // + 86399L since datepicker sets it to midnight & it would exclude readings from the chosen day [86400 is 1 day, so -1 for 23:59:59]
                            showDatePicker = false
                        }
                    ) { Text("OK") }

                },
                dismissButton = {
                    TextButton(onClick = {
                        showDatePicker = false
                    }) {
                        Text("Cancel")
                    }
                }
            ) {
                DateRangePicker(
                    state = datePickerState,
                    title = {
                        Text(
                            text = "Select date range"
                        )
                    },
                    //showModeToggle = false,
                    modifier = Modifier.weight(1f)
                )
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
                    bottomAxis = HorizontalAxis.rememberBottom(valueFormatter = { _, value, _ ->
                        val timestamp = filteredTimestamps.getOrNull(value.toInt())
                        if (timestamp != null) axisDateFormat.format(Date(timestamp * 1000L))
                        else value.toInt().toString()   // otherwise vico shouts at me :-(
                    },),
                ),
                modelProducer = modelProducer, modifier = Modifier.fillMaxSize()
            )
        }
    }
}