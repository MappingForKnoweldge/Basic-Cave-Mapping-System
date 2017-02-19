package cc.openframeworks.androidOFXspeleo;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.CompoundButton;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;
import java.io.*;

import cc.openframeworks.OFAndroid;

import static cc.openframeworks.androidOFXspeleo.RBLService.*;


public class OFActivity extends cc.openframeworks.OFActivity{

    // =============== Bluetooth ==================
    private final static String TAG = OFActivity.class.getSimpleName();

    private String rssiValue;
    private float yawValue;
    private float rollValue;
    private float pitchValue;
    private float distValue;
    private Button connectBtn = null;
    private ToggleButton laserOnOffBtn = null;
    private Button measureBtn = null;
    private Button sectionBtn = null;

    private BluetoothGattCharacteristic characteristicTx = null;
    private RBLService mBluetoothLeService;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothDevice mDevice = null;
    private String mDeviceAddress;

    private boolean flag = false;
    private boolean connState = false;
    private boolean scanFlag = false;

    private byte[] data = new byte[9];
    private static final int REQUEST_ENABLE_BT = 1;
    private static final long SCAN_PERIOD = 2000;

    final private static char[] hexArray = { '0', '1', '2', '3', '4', '5', '6',
            '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    private final ServiceConnection mServiceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName componentName,
                                       IBinder service) {
            mBluetoothLeService = ((RBLService.LocalBinder) service)
                    .getService();
            if (!mBluetoothLeService.initialize()) {
                Log.e(TAG, "Unable to initialize Bluetooth");
                finish();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            mBluetoothLeService = null;
        }
    };

    private final BroadcastReceiver mGattUpdateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();

            if (ACTION_GATT_DISCONNECTED.equals(action)) {
                Toast.makeText(getApplicationContext(), "Disconnected", Toast.LENGTH_SHORT).show();
                setButtonDisable();
            } else if (ACTION_GATT_SERVICES_DISCOVERED.equals(action)) {
                Toast.makeText(getApplicationContext(), "Connected", Toast.LENGTH_SHORT).show();

                getGattService(mBluetoothLeService.getSupportedGattService());
            } else if (ACTION_DATA_AVAILABLE.equals(action)) {
                data = intent.getByteArrayExtra(EXTRA_DATA);

                readIMUValue(data);
            } else if (ACTION_GATT_RSSI.equals(action)) {
                displayData(intent.getStringExtra(EXTRA_DATA));
            }
        }
    };
    // =============== Bluetooth ==================


    public File myFile = null;
    public FileOutputStream fOut = null;
    public OutputStreamWriter myOutWriter = null;
    public int numPoints = 0;


    public static float rssi = 0f;

    public native void callOF(float rs, float y, float r, float p, float d);
    public native void callOFRotScaTra(int r, int s, int t);
    public native void callOFmeasure(int m);
    public native void callOFsection(int s);
    public native void callOFclear(int c);

    public void connectOnClick(View v) {
        if (scanFlag == false) {
            scanLeDevice();

            Timer mTimer = new Timer();
            mTimer.schedule(new TimerTask() {

                @Override
                public void run() {
                    if (mDevice != null) {
                        mDeviceAddress = mDevice.getAddress();
                        mBluetoothLeService.connect(mDeviceAddress);
                        scanFlag = true;
                    } else {
                        runOnUiThread(new Runnable() {
                            public void run() {
                                Toast toast = Toast
                                        .makeText(
                                                OFActivity.this,
                                                "Couldn't search Ble Shiled device!",
                                                Toast.LENGTH_SHORT);
                                toast.setGravity(0, 0, Gravity.CENTER);
                                toast.show();
                            }
                        });
                    }
                }
            }, SCAN_PERIOD);
        }

        System.out.println(connState);
        if (connState == false) {
            mBluetoothLeService.connect(mDeviceAddress);
        } else {
            mBluetoothLeService.disconnect();
            mBluetoothLeService.close();
            setButtonDisable();
        }
    }

    public void laserOnCheckedChanged(View v) {
        byte[] buf = new byte[]{(byte) 0xA0, (byte) 0x00, (byte) 0x00};

        boolean on = ((ToggleButton) v).isChecked();

        if (on)
            buf[1] = 0x01;
        else
            buf[1] = 0x00;

        characteristicTx.setValue(buf);
        mBluetoothLeService.writeCharacteristic(characteristicTx);
    }

    public void measureOnClick(View v) {
        byte[] buf = new byte[]{(byte) 0xA1, (byte) 0x00, (byte) 0x00};

        buf[1] = 0x01;

        characteristicTx.setValue(buf);
        mBluetoothLeService.writeCharacteristic(characteristicTx);

        laserOnOffBtn = (ToggleButton) findViewById(R.id.laser);
        laserOnOffBtn.setChecked(false);

        this.callOFmeasure(1);
    }

    public void sectionOnClick(View v) {
        byte[] buf = new byte[]{(byte) 0xA1, (byte) 0x00, (byte) 0x00};

        buf[1] = 0x01;

        characteristicTx.setValue(buf);
        mBluetoothLeService.writeCharacteristic(characteristicTx);

        laserOnOffBtn = (ToggleButton) findViewById(R.id.laser);
        laserOnOffBtn.setChecked(false);

        this.callOFsection(1);
    }

    public void onRadioButtonClicked(View view) {
        // Is the button now checked?
        boolean checked = ((RadioButton) view).isChecked();

        // Check which radio button was clicked
        switch(view.getId()) {
            case R.id.radioButtonR:
                if (checked)
                    this.callOFRotScaTra(1, 0, 0);
                    break;
            case R.id.radioButtonS:
                if (checked)
                    this.callOFRotScaTra(0, 1, 0);
                    break;
            case R.id.radioButtonT:
                if (checked)
                    this.callOFRotScaTra(0, 0, 1);
                    break;
        }
    }

    public void clearOnClick(View v) {

        this.callOFclear(1);
    }

    /* Checks if external storage is available for read and write */
    public boolean isExternalStorageWritable() {
        String state = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(state)) {
            return true;
        }
        return false;
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    { 
        super.onCreate(savedInstanceState);
        String packageName = getPackageName();

        try {
            Date date = new Date() ;
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss") ;
            String fileName = "SpeleoIMU_" + dateFormat.format(date) + ".txt";
            //File myFile = new File("/storage/sdcard1/mysdfile.txt");
            myFile = new File(Environment.getExternalStorageDirectory(), fileName);
            myFile.createNewFile();
            fOut = new FileOutputStream(myFile);
            myOutWriter = new OutputStreamWriter(fOut);
            myOutWriter.append("SpeleoIMU\n");
            myOutWriter.append("1.0 0.0 0.0 0.0\n");
            myOutWriter.flush();

            if (isExternalStorageWritable()) {
                System.out.println("ExternalStorageWritable");
            } else {
                System.out.println("External Storage is not Writable");
            }
            System.out.println(myFile.getAbsolutePath());
        } catch (Exception e) {
            System.out.println("FILE ERROR");
            System.out.println(e.getMessage());
        }

        //ofApp = new OFAndroid(packageName,this);

        // =============== Bluetooth ==================
        //requestWindowFeature(Window.FEATURE_CUSTOM_TITLE);
        setContentView(R.layout.main_layout);
        //getWindow().setFeatureInt(Window.FEATURE_CUSTOM_TITLE, R.layout.title);

        rssiValue = "";

        yawValue = 0f;
        rollValue = 0f;
        pitchValue = 0f;
        distValue = 0f;

        if (!getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, "Ble not supported", Toast.LENGTH_SHORT)
                    .show();
            finish();
        }

        final BluetoothManager mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if (mBluetoothAdapter == null) {
            Toast.makeText(this, "Ble not supported", Toast.LENGTH_SHORT)
                    .show();
            finish();
            return;
        }

        Intent gattServiceIntent = new Intent(OFActivity.this, RBLService.class);
        bindService(gattServiceIntent, mServiceConnection, BIND_AUTO_CREATE);

        ofApp = new OFAndroid(packageName,this);
    }
	
	@Override
	public void onDetachedFromWindow() {
	}
	
    @Override
    protected void onPause() {
        super.onPause();
        ofApp.pause();
    }

//    @Override
//    protected void onResume() {
//        super.onResume();
//        ofApp.resume();
//    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
	if (OFAndroid.keyDown(keyCode, event)) {
	    return true;
	} else {
	    return super.onKeyDown(keyCode, event);
	}
    }
    
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
	if (OFAndroid.keyUp(keyCode, event)) {
	    return true;
	} else {
	    return super.onKeyUp(keyCode, event);
	}
    }


	OFAndroid ofApp;

	
    // Menus
    // http://developer.android.com/guide/topics/ui/menus.html
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
    	// Create settings menu options from here, one by one or infalting an xml
        return super.onCreateOptionsMenu(menu);
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
    	// This passes the menu option string to OF
    	// you can add additional behavior from java modifying this method
    	// but keep the call to OFAndroid so OF is notified of menu events
    	if(OFAndroid.menuItemSelected(item.getItemId())){
    		
    		return true;
    	}
    	return super.onOptionsItemSelected(item);
    }
    

    @Override
    public boolean onPrepareOptionsMenu (Menu menu){
    	// This method is called every time the menu is opened
    	//  you can add or remove menu options from here
    	return  super.onPrepareOptionsMenu(menu);
    }



    // =============== Bluetooth ==================
    @Override
    protected void onResume() {
        super.onResume();
        ofApp.resume();

        if (!mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }

        registerReceiver(mGattUpdateReceiver, makeGattUpdateIntentFilter());

        ofApp.resume();
    }

    @Override
    protected void onStop() {
        super.onStop();
        //ofApp.stop();

        flag = false;

        unregisterReceiver(mGattUpdateReceiver);

        try {
            myOutWriter.close();
            fOut.close();
        } catch (Exception e) {
            System.out.println("FILE ERROR");
            System.out.println(e.getMessage());
        }

        System.out.println("stop");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if (mServiceConnection != null)
            unbindService(mServiceConnection);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // User chose not to enable Bluetooth.
        if (requestCode == REQUEST_ENABLE_BT && resultCode == Activity.RESULT_CANCELED) {
            finish();
            return;
        }

        super.onActivityResult(requestCode, resultCode, data);
    }

    private void displayData(String data) {
        if (data != null) {
            rssiValue = data;
        }
    }

    private void readIMUValue(byte[] data) {
        for (int i = 0; i < data.length; i += 9) {
            if (data[i] == 0x0B) {
                int ValX;
                short ValY, ValZ;
                float yaw, pitch, roll;

                ValX = ((data[i + 1] << 8) & 0x0000ff00)
                        | (data[i + 2] & 0x000000ff);

                ValY = (short) (((data[i + 3] << 8) & 0x0000ff00)
                        | (data[i + 4] & 0x000000ff));

                ValZ = (short) (((data[i + 5] << 8) & 0x0000ff00)
                        | (data[i + 6] & 0x000000ff));

                yaw = (float) ValX / 10.0f;
                pitch = (float) ValY / 10.0f;
                roll = (float) ValZ / 10.0f;

                yawValue = yaw + 90;
                if (yawValue >= 360)
                    yawValue -= 360;
                pitchValue = pitch;
                rollValue = roll;
                distValue = -1f;
            } else if (data[i] == 0x0C) {
                int ValDist;
                float dist;

                ValDist = (((data[i + 7] << 8) & 0x0000ff00)
                        | (data[i + 8] & 0x000000ff));

                dist = (float) ValDist / 100.0f;
                distValue = dist;

                numPoints++;
                try {
                    String str = "1." + Integer.toString(numPoints) + " " +
                            Float.toString(distValue) + " " + Float.toString(yawValue) + " " + Float.toString(pitchValue) + "\n";
                    myOutWriter.append(str);
                    myOutWriter.flush();
                } catch (Exception e) {
                    System.out.println("FILE ERROR");
                    System.out.println(e.getMessage());
                }
            }
        }

        if (rssiValue.equals(""))
            rssi = 0f;
        else
            rssi = Float.parseFloat(rssiValue);

        this.callOF(rssi, yawValue, rollValue, pitchValue, distValue);
    }

    private void setButtonEnable() {
        flag = true;
        connState = true;

        measureBtn = (Button) findViewById(R.id.measure);
        sectionBtn = (Button) findViewById(R.id.section);
        laserOnOffBtn = (ToggleButton) findViewById(R.id.laser);
        connectBtn = (Button) findViewById(R.id.connect);

        measureBtn.setEnabled(flag);
        sectionBtn.setEnabled(flag);
        laserOnOffBtn.setEnabled(flag);
        connectBtn.setText("Disconnect");
    }

    private void setButtonDisable() {
        flag = false;
        connState = false;

        measureBtn = (Button) findViewById(R.id.measure);
        sectionBtn = (Button) findViewById(R.id.section);
        laserOnOffBtn = (ToggleButton) findViewById(R.id.laser);
        connectBtn = (Button) findViewById(R.id.connect);

        measureBtn.setEnabled(flag);
        sectionBtn.setEnabled(flag);
        laserOnOffBtn.setEnabled(flag);
        connectBtn.setText("Connect");
    }

    private void startReadRssi() {
        new Thread() {
            public void run() {

                while (flag) {
                    mBluetoothLeService.readRssi();
                    try {
                        sleep(500);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            };
        }.start();
    }

    private void getGattService(BluetoothGattService gattService) {
        if (gattService == null)
            return;

        setButtonEnable();
        startReadRssi();

        characteristicTx = gattService
                .getCharacteristic(UUID_BLE_SHIELD_TX);

        BluetoothGattCharacteristic characteristicRx = gattService
                .getCharacteristic(UUID_BLE_SHIELD_RX);
        mBluetoothLeService.setCharacteristicNotification(characteristicRx,
                true);
        mBluetoothLeService.readCharacteristic(characteristicRx);
    }

    private static IntentFilter makeGattUpdateIntentFilter() {
        final IntentFilter intentFilter = new IntentFilter();

        intentFilter.addAction(ACTION_GATT_CONNECTED);
        intentFilter.addAction(ACTION_GATT_DISCONNECTED);
        intentFilter.addAction(ACTION_GATT_SERVICES_DISCOVERED);
        intentFilter.addAction(ACTION_DATA_AVAILABLE);
        intentFilter.addAction(ACTION_GATT_RSSI);

        return intentFilter;
    }

    private void scanLeDevice() {
        new Thread() {

            @Override
            public void run() {
                mBluetoothAdapter.startLeScan(mLeScanCallback);

                try {
                    Thread.sleep(SCAN_PERIOD);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                mBluetoothAdapter.stopLeScan(mLeScanCallback);
            }
        }.start();
    }

    private BluetoothAdapter.LeScanCallback mLeScanCallback = new BluetoothAdapter.LeScanCallback() {

        @Override
        public void onLeScan(final BluetoothDevice device, final int rssi,
                             final byte[] scanRecord) {

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    byte[] serviceUuidBytes = new byte[16];
                    String serviceUuid = "";
                    for (int i = (32 - 3), j = 0; i >= (17 - 3); i--, j++) {   // AB -3 ?
                        serviceUuidBytes[j] = scanRecord[i];
                    }
                    serviceUuid = bytesToHex(serviceUuidBytes);
                    if (stringToUuidString(serviceUuid).equals(
                            RBLGattAttributes.BLE_SHIELD_SERVICE
                                    .toUpperCase(Locale.ENGLISH))) {
                        mDevice = device;
                    }
                }
            });
        }
    };

    private String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        int v;
        for (int j = 0; j < bytes.length; j++) {
            v = bytes[j] & 0xFF;
            hexChars[j * 2] = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0F];
        }
        return new String(hexChars);
    }

    private String stringToUuidString(String uuid) {
        StringBuffer newString = new StringBuffer();
        newString.append(uuid.toUpperCase(Locale.ENGLISH).substring(0, 8));
        newString.append("-");
        newString.append(uuid.toUpperCase(Locale.ENGLISH).substring(8, 12));
        newString.append("-");
        newString.append(uuid.toUpperCase(Locale.ENGLISH).substring(12, 16));
        newString.append("-");
        newString.append(uuid.toUpperCase(Locale.ENGLISH).substring(16, 20));
        newString.append("-");
        newString.append(uuid.toUpperCase(Locale.ENGLISH).substring(20, 32));

        return newString.toString();
    }
}



