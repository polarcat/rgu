package lib3d;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;

public class scene {
	private static final String TAG = "lib3d: scene";

	static {
		System.loadLibrary("3d");
	}

	/* scene interfaces */

	public static native int open(AssetManager assetManager);
	public static native void render();
	public static native void resize(int w, int h);
	public static native void rotate(int r);
	public static native void resume(Context context, Activity activity);
	public static native void pause();
	public static native void close();
	public static native void input(float x, float y);
}
