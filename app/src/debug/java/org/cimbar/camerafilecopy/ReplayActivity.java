package org.cimbar.camerafilecopy;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import org.opencv.android.OpenCVLoader;
import org.opencv.core.Mat;
import org.opencv.imgcodecs.Imgcodecs;
import org.opencv.imgproc.Imgproc;

import java.io.File;
import java.util.Arrays;
import java.util.Comparator;

/** Debug-only activity for replaying software-camera frames through the JNI decoder. */
public class ReplayActivity extends MainActivity {
    private static final String TAG = "CfcReplay";
    private TextView statusView;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        statusView = new TextView(this);
        statusView.setText("Waiting for replay frames...");
        setContentView(statusView);

        if (!OpenCVLoader.initLocal()) {
            statusView.setText("OpenCV initialization failed");
            return;
        }
        System.loadLibrary("cfc-cpp");
        File replayDirectory = getExternalFilesDir("camera");
        if (replayDirectory != null) {
            replayDirectory.mkdirs();
            Log.i(TAG, "replay directory=" + replayDirectory);
        }

        String framePath = getIntent().getStringExtra("frames");
        int mode = getIntent().getIntExtra("mode", 68);
        int fps = Math.max(1, getIntent().getIntExtra("fps", 15));
        new Thread(() -> replay(framePath, mode, fps), "cfc-replay").start();
    }

    private void replay(String framePath, int mode, int fps) {
        File directory;
        if (framePath != null && framePath.startsWith("external")) {
            String suffix = framePath.length() == "external".length()
                    ? ""
                    : framePath.substring("external".length()).replaceFirst("^[/\\\\]+", "");
            directory = suffix.isEmpty()
                    ? getExternalFilesDir(null)
                    : new File(getExternalFilesDir(null), suffix);
        } else {
            directory = framePath == null ? null : new File(framePath);
        }
        File[] frames = directory == null ? null : directory.listFiles(file -> {
            String name = file.getName().toLowerCase();
            return name.startsWith("frame_") &&
                    (name.endsWith(".png") || name.endsWith(".jpg") || name.endsWith(".jpeg"));
        });
        Log.i(TAG, "directory=" + directory + " exists=" + (directory != null && directory.exists()) +
                " readable=" + (directory != null && directory.canRead()) +
                " frames=" + (frames == null ? -1 : frames.length));
        if (frames == null || frames.length == 0) {
            updateStatus("No replay frames in " + framePath);
            return;
        }
        Arrays.sort(frames, Comparator.comparingInt(ReplayActivity::frameNumber));

        shutdownJNI();
        String lastResult = "";
        int processed = 0;
        long delay = Math.max(1, 1000L / fps);
        for (File frame : frames) {
            Mat bgr = Imgcodecs.imread(frame.getAbsolutePath(), Imgcodecs.IMREAD_COLOR);
            Mat rgba = new Mat();
            if (bgr.empty()) {
                bgr.release();
                continue;
            }
            Imgproc.cvtColor(bgr, rgba, Imgproc.COLOR_BGR2RGBA);
            long started = System.nanoTime();
            String result = processImageJNI(rgba.getNativeObjAddr(), getFilesDir().getPath(), mode);
            long elapsedMs = (System.nanoTime() - started) / 1_000_000L;
            if (!result.isEmpty()) {
                lastResult = result;
            }
            ++processed;
            final String text = "frames=" + processed + "/" + frames.length +
                    " decode_ms=" + elapsedMs + " result=" + lastResult;
            updateStatus(text);
            rgba.release();
            bgr.release();
            try {
                Thread.sleep(delay);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                break;
            }
        }
        updateStatus("replay complete: frames=" + processed + " result=" + lastResult);
    }

    private static int frameNumber(File file) {
        String name = file.getName();
        int begin = name.indexOf('_') + 1;
        int end = name.lastIndexOf('.');
        try {
            return Integer.parseInt(name.substring(begin, end));
        } catch (RuntimeException ignored) {
            return Integer.MAX_VALUE;
        }
    }

    private void updateStatus(String text) {
        runOnUiThread(() -> statusView.setText(text));
    }
}
