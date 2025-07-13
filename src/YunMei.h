#ifndef YunMei_h

#define YunMei_h

#include<Arduino.h>
#include <BLEDevice.h>

class YunMei{
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
        YunMei(String bleName, String bleServerName, String bleServerAddress, String ServiceUUID, String characteristicUUID, String secret);
        ~YunMei();
        bool isConnected();
        bool begin();
        void unLock();
};

#endif