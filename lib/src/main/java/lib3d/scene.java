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
	public static native int render();
	public static native void resize(int w, int h);
	public static native void rotate(int r);
	public static native void resume(Context context, Activity activity);
	public static native void pause();
	public static native void close();
	public static native void input(float x, float y);

	/* device rotation and inclination matrices */

	public static native void rmatrix(
	  float a0, float a1, float a2,
	  float a3, float a4, float a5,
	  float a6, float a7, float a8);

	public static native void imatrix(
	  float a0, float a1, float a2,
	  float a3, float a4, float a5,
	  float a6, float a7, float a8);

	public static native void orientation(float azimuth, float pitch,
	  float roll);
}
