package com.group2.painpatrol.data

import android.Manifest
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import kotlin.random.Random

// As we will only have one type of notification,
// this function works specifically for creating the Notif Channel
// for Discomfort Alerts
private fun createNotificationChannel(context: Context) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        val channel = NotificationChannel(
            "painpatrol_discomfort_alerts",
            "Discomfort alerts",
            NotificationManager.IMPORTANCE_DEFAULT
        ).apply { description = "Alerts when values are outside of comfort range." }


        val notificationManager: NotificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.createNotificationChannel(channel)
    }
}

//TODO
// - cooldown


// conditions is an Array of Strings with values "warm/cold, loud/quiet etc"
fun sendNotification(context: Context, conditions: Array<String>){

    val condition : String = conditions.joinToString { ", " }

    val notif = NotificationCompat.Builder(context, "painpatrol_discomfort_alerts")
        .setPriority(NotificationCompat.PRIORITY_DEFAULT)
        .setContentTitle("⚠\uFE0F Your environment is not ideal!")
        .setContentText("It's too "+ condition + "!")
        .build()

    with(NotificationManagerCompat.from(context)) {
        if (ActivityCompat.checkSelfPermission(
                context,
                Manifest.permission.POST_NOTIFICATIONS
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            // TODO: Consider calling ActivityCompat#requestPermissions here
            // to request the missing permissions, and then overriding
            // public fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>,
            //                                        grantResults: IntArray)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.

            return@with
        }
        val id = Random.nextInt()       // For now, id is random. Not optimal but it works.
        notify(id, notif)
    }
}