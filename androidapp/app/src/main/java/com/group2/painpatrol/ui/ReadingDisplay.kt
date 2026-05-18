package com.group2.painpatrol.ui


import android.graphics.Color.alpha
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.group2.painpatrol.ui.theme.*
import androidx.compose.material3.MaterialTheme

// Boxes with sensor readings
//TODO Evaluate background color based on sensor value (depending on Comfort range)
@Composable
fun ReadingDisplay(label: String, value: String, modifier: Modifier = Modifier) {
    Box(
        contentAlignment = Alignment.Center,
        modifier =
            modifier
                .fillMaxSize()
                .clip(shape = CardDefaults.shape)
                .background(MaterialTheme.colorScheme.primary)
    )
    {
        Column(modifier = Modifier.fillMaxSize(), verticalArrangement = Arrangement.Center, horizontalAlignment = Alignment.CenterHorizontally) {
            Text(text = label, fontSize = 20.sp, modifier = Modifier.padding(8.dp), color = MaterialTheme.colorScheme.onPrimary)
            Text(text = value, fontSize = 20.sp, modifier = Modifier.padding(8.dp), color = MaterialTheme.colorScheme.onPrimary)



        }


    }
}