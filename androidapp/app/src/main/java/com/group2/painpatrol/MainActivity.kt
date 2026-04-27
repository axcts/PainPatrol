package com.group2.painpatrol

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import com.group2.painpatrol.ui.theme.PainpatrolTheme
import com.group2.painpatrol.ui.AppScreen
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.ui.res.painterResource

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            PainpatrolTheme {
                PainPatrolApp()
            }
        }
    }
}


enum class AppDestinations(
    val label: String,
    val icon: Int
) {
    HOME("Home", R.drawable.ic_home),
    STATS("Statistics",R.drawable.ic_stats)
}


@Preview(showSystemUi = true)
@Composable
fun PainPatrolApp() {
    var currentDestination by rememberSaveable { mutableStateOf(AppDestinations.HOME) }

    Scaffold(
        modifier = Modifier.fillMaxSize(),
        bottomBar = {
            NavigationBar {
                AppDestinations.entries.forEach {
                    destination -> NavigationBarItem(
                        selected = currentDestination == destination,
                        onClick = { currentDestination = destination },
                        icon = {
                            Icon(
                                painter = painterResource(id = destination.icon),
                                contentDescription = destination.label
                            )
                        },
                        label = { Text(destination.label) }
                    )
                }
            }
        }
    ) { innerPadding ->
        when (currentDestination) {
            AppDestinations.HOME -> HomeScreen(modifier = Modifier.padding(innerPadding))
            AppDestinations.STATS -> StatisticScreen(modifier = Modifier.padding(innerPadding))
        }
    }
}

// Home screen with the sensor data
@Composable
fun HomeScreen(modifier: Modifier = Modifier) {
    Text(text = "Placeholder", modifier = modifier)
}


// Screen of the Statistics, graphs etc will go here
@Composable
fun StatisticScreen(modifier: Modifier = Modifier) {
    Text(text = "Another placeholder", modifier = modifier)
}