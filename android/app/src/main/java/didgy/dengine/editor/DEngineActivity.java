package didgy.dengine.editor;

import android.app.NativeActivity;

public class DEngineActivity extends NativeActivity {


    public int getCurrentOrientation()
    {
        return getResources().getConfiguration().orientation;
    }

}
