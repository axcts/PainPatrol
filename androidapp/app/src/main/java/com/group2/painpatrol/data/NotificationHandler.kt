package com.group2.painpatrol.data

import android.Manifest
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.BitmapFactory
import android.os.Build
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import com.group2.painpatrol.R
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

// conditions is list of Strings with values "warm/cold, loud/quiet etc"
fun sendNotification(context: Context, conditions: MutableList<String>){

    createNotificationChannel(context)

    // continues only if push notification permissions are granted
    if (ActivityCompat.checkSelfPermission(
        context, Manifest.permission.POST_NOTIFICATIONS
    ) != PackageManager.PERMISSION_GRANTED) return

    // handles prettier display in the notification content text
    val condition: String = when (conditions.size) {
        0 -> return
        1 -> conditions[0]
        else -> conditions.dropLast(1).joinToString(", ") + " and " + conditions.last()
    }
    val bitmap = BitmapFactory.decodeResource(context.resources, R.mipmap.ic_launcher)

    val notif = NotificationCompat.Builder(context, "painpatrol_discomfort_alerts")
        .setSmallIcon(R.drawable.ic_launcher_foreground)
        .setLargeIcon(bitmap)
        .setPriority(NotificationCompat.PRIORITY_DEFAULT)
        .setContentTitle("⚠\uFE0F Your environment is not ideal!")
        .setContentText("It's too "+ condition + "!")
        .build()

    // For now, id is random. Not optimal but it works.
    NotificationManagerCompat.from(context).notify(Random.nextInt(), notif)

    }
