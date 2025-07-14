#ifndef YunMeiESP32_h

#define YunMeiESP32_h

#include<Arduino.h>
#include <BLEDevice.h>

class YunMeiESP32{
    private:
        String bleName;
        String bleServerName;
        BLEUUID ServiceUUID;
        BLEUUID CharacteristicUUID;
        String secret;

        BLERemoteCharacteristic *characteristic;
        BLEClient *client;
        BLEAddress *serverAddress;

        BLERemoteService *remoteService;

        esp_ble_addr_type_t addressType;
        bool doConnect;
        bool connected;
        bool firstScan;

        class DeviceCallbacks;
        class ClientCallbacks;

        void restartScan();
        void scan();
        bool connectToServer();
        void writeCharacteristic(const char *message);

        uint8_t *hexStringToBytes(const char *hex, size_t *outLen);
        uint8_t hexCharToByte(char c);

    public:
        YunMeiESP32(String bleName, String bleServerName, String bleServerAddress, String serviceUUID, String characteristicUUID, String secret);
        ~YunMeiESP32();
        bool isConnected();
        bool begin();
        void unLock();
};

#endif