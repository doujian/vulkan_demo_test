package com.vulkandemo

import android.content.Intent
import android.os.Bundle
import android.widget.ArrayAdapter
import android.widget.ListView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        val demoNames = VulkanDemo.nativeGetDemoNames()
        val listView = findViewById<ListView>(R.id.demoListView)
        
        val adapter = ArrayAdapter(this, android.R.layout.simple_list_item_1, demoNames)
        listView.adapter = adapter
        
        listView.setOnItemClickListener { _, _, position, _ ->
            val demoName = demoNames[position]
            val intent = Intent(this, DemoActivity::class.java)
            intent.putExtra("demo_name", demoName)
            startActivity(intent)
        }
    }
}