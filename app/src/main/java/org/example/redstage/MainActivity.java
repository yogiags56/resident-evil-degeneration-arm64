package org.example.redstage;

import android.app.Activity;
import android.os.Bundle;
import android.widget.ScrollView;
import android.widget.TextView;

public final class MainActivity extends Activity {
    static { System.loadLibrary("re_asset_probe"); }
    private native String nativeProbe(android.content.res.AssetManager manager);

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        TextView output = new TextView(this);
        output.setTextIsSelectable(true);
        output.setPadding(24, 24, 24, 24);
        output.setText(nativeProbe(getAssets()));
        ScrollView scroll = new ScrollView(this);
        scroll.addView(output);
        setContentView(scroll);
    }
}
