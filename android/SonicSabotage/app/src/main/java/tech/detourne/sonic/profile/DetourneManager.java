/*
 * Copyright (c) 2015, Nordic Semiconductor
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package tech.detourne.sonic.profile;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.Context;
import android.support.annotation.NonNull;
import android.util.Log;

import java.util.Deque;
import java.util.LinkedList;
import java.util.Random;
import java.util.UUID;

import no.nordicsemi.android.ble.BleManager;
import no.nordicsemi.android.ble.Request;
import no.nordicsemi.android.log.LogContract;

public class DetourneManager extends BleManager<DetourneManagerCallbacks> {
	// Sonic Sabotage Service UUID
	public final static UUID LBS_UUID_SERVICE = UUID.fromString("000000ff-0000-1000-8000-00805f9b34fb");
	// set mode of device
	public final static UUID LBS_UUID_TRIG = UUID.fromString("0000ff02-0000-1000-8000-00805f9b34fb");
	// set beep on or off
	public final static UUID LBS_UUID_BUZZ_CHAR = UUID.fromString("0000ff01-0000-1000-8000-00805f9b34fb");

	// Fixed interval setting - beep on regular schedule
	public final static UUID LBS_UUID_FIXEDINT_CHAR = UUID.fromString("0000ff03-0000-1000-8000-00805f9b34fb");
	// Random interval setting - beep on random schedule
	public final static UUID LBS_UUID_RANDINT_CHAR = UUID.fromString("0000ff04-0000-1000-8000-00805f9b34fb");
	// RSSI threshold characteristic - beep on signal threshold
	public final static UUID LBS_UUID_RSSIMIN_CHAR = UUID.fromString("0000ff06-0000-1000-8000-00805f9b34fb");
	// Solar threshold characteristic - beep on luminosity threshold
	public final static UUID LBS_UUID_SOLARMIN_CHAR = UUID.fromString("0000ff09-0000-1000-8000-00805f9b34fb");
	//password characteristic for OTA connection
	public final static UUID LBS_UUID_PASS_CHAR = UUID.fromString("0000ff07-0000-1000-8000-00805f9b34fb");
	//luminosity value read from device
	public final static UUID LBS_UUID_SOLARVAL_CHAR = UUID.fromString("0000ff05-0000-1000-8000-00805f9b34fb");
	//RSSI value read from device
	public final static UUID LBS_UUID_RSSIVAL_CHAR = UUID.fromString("0000ff08-0000-1000-8000-00805f9b34fb");


	private BluetoothGattCharacteristic mButtonCharacteristic, mLedCharacteristic,
			mModeCharacteristic, mRSSIMinCharacteristic, mSolarMinCharacteristic,
			mSolarValCharacteristic, mRSSIValCharacteristic,
			mFixedIntCharacteristic, mRandIntCharacteristic, mPassCharacteristic;

	public DetourneManager(final Context context) {
		super(context);
	}

	@NonNull
	@Override
	protected BleManagerGattCallback getGattCallback() {
		return mGattCallback;
	}

	@Override
	protected boolean shouldAutoConnect() {
		// If you want to connect to the device using autoConnect flag = true, return true here.
		// Read the documentation of this method.
		return super.shouldAutoConnect();
	}

	/**
	 * BluetoothGatt callbacks for connection/disconnection, service discovery, receiving indication, etc
	 */
	private final BleManagerGattCallback mGattCallback = new BleManagerGattCallback() {

		@Override
		protected Deque<Request> initGatt(final BluetoothGatt gatt) {
			final LinkedList<Request> requests = new LinkedList<>();
			requests.push(Request.newReadRequest(mButtonCharacteristic));
			requests.push(Request.newReadRequest(mLedCharacteristic));
			requests.push(Request.newReadRequest(mModeCharacteristic));
			requests.push(Request.newReadRequest(mRSSIMinCharacteristic));
			requests.push(Request.newReadRequest(mSolarMinCharacteristic));
			requests.push(Request.newReadRequest(mFixedIntCharacteristic));
			requests.push(Request.newReadRequest(mRandIntCharacteristic));
			requests.push(Request.newReadRequest(mPassCharacteristic));
			requests.push(Request.newEnableNotificationsRequest(mButtonCharacteristic));
			requests.push(Request.newEnableNotificationsRequest(mRSSIValCharacteristic));
			requests.push(Request.newEnableNotificationsRequest(mSolarValCharacteristic));
			return requests;
		}

		@Override
		public boolean isRequiredServiceSupported(final BluetoothGatt gatt) {

			final BluetoothGattService service = gatt.getService(LBS_UUID_SERVICE);
			if (service != null) {
				mLedCharacteristic = service.getCharacteristic(LBS_UUID_BUZZ_CHAR);
				mModeCharacteristic = service.getCharacteristic(LBS_UUID_TRIG);
				mRSSIMinCharacteristic = service.getCharacteristic(LBS_UUID_RSSIMIN_CHAR);
				mSolarMinCharacteristic = service.getCharacteristic(LBS_UUID_SOLARMIN_CHAR);
				mFixedIntCharacteristic = service.getCharacteristic(LBS_UUID_FIXEDINT_CHAR);
				mRandIntCharacteristic = service.getCharacteristic(LBS_UUID_RANDINT_CHAR);
				mPassCharacteristic = service.getCharacteristic(LBS_UUID_PASS_CHAR);
				mSolarValCharacteristic = service.getCharacteristic(LBS_UUID_SOLARVAL_CHAR);
				mRSSIValCharacteristic = service.getCharacteristic(LBS_UUID_RSSIVAL_CHAR);
			}

			boolean writeRequest = false;
			if (mLedCharacteristic != null && mModeCharacteristic != null &&
					mRSSIMinCharacteristic != null && mSolarMinCharacteristic != null &&
						mFixedIntCharacteristic != null && mRandIntCharacteristic != null) {
				final int rxProperties = mLedCharacteristic.getProperties();
				writeRequest = (rxProperties & BluetoothGattCharacteristic.PROPERTY_WRITE) > 0;

				//test if writing to encrypted char is supported/trigger bonding
				//mModeCharacteristic.setValue(new byte[] {(byte) 0});
				//writeCharacteristic(mModeCharacteristic);

				return mLedCharacteristic != null && writeRequest;
			}

			return false;
		}

		@Override
		protected void onDeviceDisconnected() {
			mButtonCharacteristic = null;
			mLedCharacteristic = null;
			mModeCharacteristic = null;
			mRSSIMinCharacteristic = null;
			mSolarMinCharacteristic = null;
			mFixedIntCharacteristic = null;
			mRandIntCharacteristic = null;
			mPassCharacteristic = null;
			mSolarValCharacteristic = null;
			mRSSIValCharacteristic = null;
		}

		@Override
		protected void onCharacteristicRead(final BluetoothGatt gatt, final BluetoothGattCharacteristic characteristic) {
			if (characteristic == mLedCharacteristic) {
				final int data = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, 0);
				final boolean buzzOn = data == 0x01;
				log(LogContract.Log.Level.APPLICATION, "Buzz " + (buzzOn ? "ON" : "OFF"));
				mCallbacks.onDataSent(buzzOn);
				Log.d("charman","got a buzz");
			} else if(characteristic == mPassCharacteristic) {
				final String pass = characteristic.getStringValue(0);
				Log.d("charman","got a pass: " + pass);
				mCallbacks.onPassReceived(pass);
			}

			/*else {
				final boolean buttonPressed = data == 0x01;
				log(LogContract.Log.Level.APPLICATION, "Button " + (buttonPressed ? "pressed" : "released"));
				mCallbacks.onDataReceived(buttonPressed);
			}*/
		}

		@Override
		public void onCharacteristicWrite(final BluetoothGatt gatt, final BluetoothGattCharacteristic characteristic) {
			if(characteristic == mLedCharacteristic) {
				final int data = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, 0);
				final boolean buzzOn = data == 0x01;
				log(LogContract.Log.Level.APPLICATION, "Buzz " + (buzzOn ? "ON" : "OFF"));
				mCallbacks.onDataSent(buzzOn);
			}
		}

		@Override
		public void onCharacteristicNotified(final BluetoothGatt gatt, final BluetoothGattCharacteristic characteristic) {
			if(characteristic == mLedCharacteristic) {
				final int data = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, 0);
				final boolean buttonPressed = data == 0x01;
				log(LogContract.Log.Level.APPLICATION, "Button " + (buttonPressed ? "pressed" : "released"));
				mCallbacks.onDataReceived(buttonPressed);
			}
			else if(characteristic == mSolarValCharacteristic) {
				final int data = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, 0);
				mCallbacks.onRSSIReceived((byte)data);

			}
			else if (characteristic == mRSSIValCharacteristic) {
				final int data = characteristic.getIntValue(BluetoothGattCharacteristic.FORMAT_UINT8, 0);
				mCallbacks.onSolarReceived((byte)data);
			}
		}
	};

	/*
	public void send(final boolean onOff) {
		// Are we connected?
		if (mLedCharacteristic == null)
			return;

		final byte[] command = new byte[] {(byte) (onOff ? 1 : 0)};
		mLedCharacteristic.setValue(command);
		log(LogContract.Log.Level.WARNING, "Turning Buzz " + (onOff ? "ON" : "OFF") + "...");
		writeCharacteristic(mLedCharacteristic);
	}
	*/

	public void readPassCharacteristic() {
		readCharacteristic(mPassCharacteristic);
	}

	public void startFlashMode() {
		mModeCharacteristic.setValue(new byte[]{0x1});
		writeCharacteristic(mModeCharacteristic);
	}

	public void sendByte(final UUID uuid, final byte val) {
		// Are we connected?
		if (mLedCharacteristic == null || mFixedIntCharacteristic == null ||
				mRandIntCharacteristic == null || mModeCharacteristic == null ||
				mRSSIMinCharacteristic == null || mSolarMinCharacteristic == null)
			return;

		final byte[] command = new byte[] { val };

		if(uuid.equals(mLedCharacteristic.getUuid())) {
			mLedCharacteristic.setValue(command);
			writeCharacteristic(mLedCharacteristic);
			Log.i("writing a char", " *buzz* ");
		}
		else if(uuid.equals(mFixedIntCharacteristic.getUuid())) {
			mFixedIntCharacteristic.setValue(command);
			writeCharacteristic(mFixedIntCharacteristic);
			Log.i("writing a char", " *fixedInt* ");
		}
		else if(uuid.equals(mRandIntCharacteristic.getUuid())) {
			mRandIntCharacteristic.setValue(command);
			writeCharacteristic(mRandIntCharacteristic);
			Log.i("writing a char", " *randInt* ");
		}
		else if(uuid.equals(mRSSIMinCharacteristic.getUuid())) {
			mRSSIMinCharacteristic.setValue(command);
			writeCharacteristic(mRSSIMinCharacteristic);
			Log.i("writing a char", " *rssimin* ");
		}
		else if(uuid.equals(mSolarMinCharacteristic.getUuid())) {
			mSolarMinCharacteristic.setValue(command);
			writeCharacteristic(mSolarMinCharacteristic);
			Log.i("writing a char", " *solarmin* ");
		}

		//mLedCharacteristic.setValue(command);
		//log(LogContract.Log.Level.WARNING, "Writing Val " + val );
		//writeCharacteristic(mLedCharacteristic);
	}

}
