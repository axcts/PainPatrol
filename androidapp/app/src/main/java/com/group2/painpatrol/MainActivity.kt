@file:OptIn(ExperimentalGridApi::class) // not sure whether placeholder or not, but without this it likes to throw an error

package com.group2.painpatrol

import android.Manifest
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.compose.foundation.layout.ExperimentalGridApi
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
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.ui.res.painterResource
import com.group2.painpatrol.data.MQTTSubscriber
import com.group2.painpatrol.ui.AppViewModel
import com.group2.painpatrol.ui.HomeScreen
import com.group2.painpatrol.ui.StatisticScreen
import com.group2.painpatrol.ui.SettingsScreen
import androidx.compose.runtime.collectAsState
import androidx.compose.foundation.isSystemInDarkTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        MQTTSubscriber.connect()
        AppViewModel.init(this)
        enableEdgeToEdge()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 0)
        }
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
    STATS("Statistics",R.drawable.ic_stats),
    SETTINGS("Settings", R.drawable.ic_settings)
}


@Preview(showSystemUi = true)
@Composable
fun PainPatrolApp() {
    val themeMode = AppViewModel.themeMode.collectAsState()
    val darkTheme = when (themeMode.value) {
        1 -> false
        2 -> true
        else -> isSystemInDarkTheme()
    }
    var currentDestination by rememberSaveable { mutableStateOf(AppDestinations.HOME) }

    PainpatrolTheme(darkTheme = darkTheme) {
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
                AppDestinations.SETTINGS -> SettingsScreen(modifier = Modifier.padding(innerPadding))
            }
        }
    }
}