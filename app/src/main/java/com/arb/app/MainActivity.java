package com.arb.app;

import lib3d.scene;

import android.os.Bundle;
import android.app.Activity;
import android.hardware.Camera;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.graphics.SurfaceTexture;
import android.view.SurfaceView;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import java.io.IOException;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends Activity implements GLSurfaceView.Renderer,
  SensorEventListener
{
	private static final String TAG = "arb: ";
	private Camera camera_;
	private GLSurfaceView surface_;
	private SurfaceTexture texture_;

	private SensorManager sensors_;
	private Sensor accelerometer_;
	private Sensor magnetometer_;

	private void setFullscreen()
	{
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
		  WindowManager.LayoutParams.FLAG_FULLSCREEN);

		final int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
		  | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
		  | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
		  | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
		  | View.SYSTEM_UI_FLAG_FULLSCREEN
		  | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

		getWindow().getDecorView().setSystemUiVisibility(flags);
	}

	private void initSensors()
	{
		sensors_ = (SensorManager) getSystemService(SENSOR_SERVICE);
		accelerometer_ = sensors_.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		magnetometer_ = sensors_.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);

		Log.d(TAG, "init sensors ...");
	}

	public void initListeners()
	{
		sensors_.registerListener(this, accelerometer_,
		  SensorManager.SENSOR_DELAY_FASTEST);
		sensors_.registerListener(this, magnetometer_,
		  SensorManager.SENSOR_DELAY_FASTEST);

		Log.d(TAG, "init listeners ...");
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		setFullscreen();

		surface_ = new GLSurfaceView(this);
		surface_.setPreserveEGLContextOnPause(true);
		surface_.setEGLContextClientVersion(2);
		surface_.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
		surface_.setRenderer(this);
		surface_.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

		setContentView(surface_);

//		Log.i(TAG, "external storage: " + android.os.Environment.getExternalStorageDirectory());

		initSensors();
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		scene.resume(getApplicationContext(), this);
		surface_.onResume();
		startCamera();
		initListeners();
	}

	@Override
	protected void onPause()
	{
		super.onPause();
		camera_.stopPreview();
		camera_.release();
		scene.pause();
		surface_.onPause();
		sensors_.unregisterListener(this);
	}

	@Override
	protected void onDestroy()
	{
		super.onDestroy();
		sensors_.unregisterListener(this);
		camera_.stopPreview();
		camera_.release();
		scene.close();
	}

	private void startCamera()
	{
		synchronized(this) {
			camera_ = Camera.open();
			Camera.Parameters p = camera_.getParameters();
			p.setRotation(90);
			camera_.setParameters(p);

			try {
				camera_.setPreviewTexture(texture_);
			} catch (IOException e) {
				e.printStackTrace();
			}

			camera_.startPreview();
		}
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig cfg)
	{
		texture_ = new SurfaceTexture(scene.open(getAssets()));
		startCamera();
	}

	@Override
	public void onSurfaceChanged(GL10 gl, int w, int h)
	{
		scene.resize(w, h);
		scene.rotate(getWindowManager().getDefaultDisplay().getRotation());
	}

	@Override
	public void onDrawFrame(GL10 gl)
	{
		texture_.updateTexImage();
		scene.render();
	}

	@Override
	public boolean onTouchEvent(MotionEvent e)
	{
		switch (e.getAction()) {
		case MotionEvent.ACTION_DOWN:
			scene.input(e.getX(), e.getY());
			break;
		}

		return true;
	}

	float[] gravity_;
	float[] geomagnetic_;
	float orientation_[] = new float[3];
	float R_[] = new float[9];
	float I_[] = new float[9];

	@Override
	public void onSensorChanged(SensorEvent event)
	{
		if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
			gravity_ = event.values;

		if (event.sensor.getType() == Sensor.TYPE_MAGNETIC_FIELD)
			geomagnetic_ = event.values;

		if (gravity_ == null || geomagnetic_ == null) {
			return;
		} else if (!SensorManager.getRotationMatrix(R_, I_, gravity_,
		  geomagnetic_)) {
			return;
		}

		SensorManager.getOrientation(R_, orientation_);

		scene.rmatrix(R_[0], R_[1], R_[2], R_[3], R_[4], R_[5], R_[6], R_[7], R_[8]);
		scene.imatrix(I_[0], I_[1], I_[2], I_[3], I_[4], I_[5], I_[6], I_[7], I_[8]);
		scene.orientation(orientation_[0], orientation_[1], orientation_[2]);
	}

	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy)
	{
	}
}
