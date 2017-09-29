package com.example.myb;

import android.app.Activity;
import android.app.NativeActivity;
import android.os.Bundle;
import android.content.Intent;

public class StartActivity extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.start_activity);

        Intent intent = new Intent(StartActivity.this, NativeActivity.class);
        startActivity(intent);
    }
}
