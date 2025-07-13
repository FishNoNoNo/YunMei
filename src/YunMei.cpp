#include "YunMei.h"

uint8_t YunMei::hexCharToByte(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return 0;
}

uint8_t *YunMei::hexStringToBytes(const char *hex, size_t *outLen)
{
    *outLen = 0;
    if (!hex || strlen(hex) == 0)
        return nullptr;

    int inputLength = strlen(hex);
    int validCount = 0;
    for (int i = 0; i < inputLength; i++)
    {
        if (isxdigit((unsigned char)hex[i]))
        {
            validCount++;
        }
    }

    if (validCount == 0 || validCount % 2 != 0)
    {
        return nullptr;
    }

    *outLen = validCount / 2;
    uint8_t *bytes = (uint8_t *)malloc(*outLen);
    if (!bytes)
    {
        return nullptr;
    }

    int byteIndex = 0;
    for (int i = 0; i < inputLength; i++)
    {
        if (!isxdigit((unsigned char)hex[i]))
            continue;

        uint8_t highNibble = hexCharToByte(hex[i]);
        i++;
        if (i >= inputLength || !isxdigit((unsigned char)hex[i]))
        {
            free(bytes);
            return nullptr;
        }
        uint8_t lowNibble = hexCharToByte(hex[i]);

        bytes[byteIndex++] = (highNibble << 4) | lowNibble;
    }

    return bytes;
}

YunMei::YunMei(String bleName, String bleServerName, String bleServerAddress, String serviceUUID, String characteristicUUID, String secret)
{
    this->bleName = bleName;
    this->bleServerName = bleServerName;
    this->ServiceUUID = BLEUUID(serviceUUID);
    this->CharacteristicUUID = BLEUUID(characteristicUUID);
    this->secret = secret;

    this->client = nullptr;
    this->characteristic = nullptr;
    this->serverAddress = new BLEAddress(bleServerAddress);
    this->remoteService = nullptr;

    this->addressType = BLE_ADDR_TYPE_RANDOM;
    this->doConnect = false;
    this->connected = false;
    this->firstScan = true;
}

YunMei::~YunMei()
{
    if (client && client->isConnected())
    {
        client->disconnect();
    }
    if (client)
    {
        delete client;
        client = nullptr;
    }
}

class YunMei::DeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
private:
    String targetName;
    BLEAddress *&serverAddressPtr;
    esp_ble_addr_type_t &addressTypeRef;
    bool &doConnectRef;

public:
    DeviceCallbacks(String name, BLEAddress *&addr, esp_ble_addr_type_t &type, bool &connectFlag)
        : targetName(name), serverAddressPtr(addr), addressTypeRef(type), doConnectRef(connectFlag) {}

    void onResult(BLEAdvertisedDevice advertisedDevice) override
    {
        // 打印发现的设备名称
        String deviceName = advertisedDevice.getName();
        Serial.print("Found device: ");
        if (deviceName.length() > 0)
        {
            Serial.print("Name='");
            Serial.print(deviceName);
            Serial.print("'");
        }
        else
        {
            Serial.print("Unnamed device");
        }

        // 打印设备地址
        BLEAddress addr = advertisedDevice.getAddress();
        Serial.print(", Address=");
        Serial.print(addr.toString().c_str());

        // 打印地址类型
        esp_ble_addr_type_t type = advertisedDevice.getAddressType();
        Serial.print(", Address Type=");
        switch (type)
        {
        case BLE_ADDR_TYPE_PUBLIC:
            Serial.println("PUBLIC");
            break;
        case BLE_ADDR_TYPE_RANDOM:
            Serial.println("RANDOM");
            break;
        case BLE_ADDR_TYPE_RPA_PUBLIC:
            Serial.println("RPA_PUBLIC");
            break;
        case BLE_ADDR_TYPE_RPA_RANDOM:
            Serial.println("RPA_RANDOM");
            break;
        default:
            Serial.println("UNKNOWN");
            break;
        }

        // 如果匹配目标名称
        if (deviceName == this->targetName)
        {
            Serial.println("[MATCH] Target device found!");
            Serial.print("Stopping scan and preparing to connect to ");
            Serial.println(addr.toString().c_str());

            // 删除旧地址
            if (this->serverAddressPtr)
            {
                delete this->serverAddressPtr;
                this->serverAddressPtr = nullptr;
            }

            // 设置新地址和地址类型
            this->serverAddressPtr = new BLEAddress(addr);
            this->addressTypeRef = type;

            Serial.print("Saved address type: ");
            switch (this->addressTypeRef)
            {
            case BLE_ADDR_TYPE_PUBLIC:
                Serial.println("PUBLIC");
                break;
            case BLE_ADDR_TYPE_RANDOM:
                Serial.println("RANDOM");
                break;
            case BLE_ADDR_TYPE_RPA_PUBLIC:
                Serial.println("RPA_PUBLIC");
                break;
            case BLE_ADDR_TYPE_RPA_RANDOM:
                Serial.println("RPA_RANDOM");
                break;
            default:
                Serial.println("UNKNOWN");
                break;
            }

            this->doConnectRef = true;

            // 停止扫描
            advertisedDevice.getScan()->stop();
            Serial.println("Scan stopped.");
        }
    }
};

void YunMei::restartScan()
{
    if (this->client)
    {
        client->disconnect();
        delete client;
        client = nullptr;
    }

    this->characteristic = nullptr;
    this->connected = false;

    this->scan();
}

void YunMei::scan()
{
    if (firstScan)
    {
        BLEDevice::init(this->bleName);
        BLEDevice::setPower(ESP_PWR_LVL_P9);
        firstScan = false;
    }

    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(
        new DeviceCallbacks(this->bleServerName, this->serverAddress,
                            this->addressType, this->doConnect));
    pBLEScan->setActiveScan(true);
    pBLEScan->start(10);
}

class YunMei::ClientCallbacks : public BLEClientCallbacks
{
private:
    bool &connectedRef;

public:
    ClientCallbacks(bool &connectedRef) : connectedRef(connectedRef) {}

    void onConnect(BLEClient *client) override
    {
        Serial.println("Connected to server (callback)");
        this->connectedRef = true;
        return;
    }

    void onDisconnect(BLEClient *client) override
    {
        Serial.println("Disconnected from server");
        this->connectedRef = false;
    }
};

bool YunMei::connectToServer()
{
    Serial.print("Connecting to device: ");

    if (!this->client || !this->client->isConnected())
    {
        if (this->client)
        {
            delete this->client;
        }
        this->client = BLEDevice::createClient();
        this->client->setClientCallbacks(new ClientCallbacks(this->connected));
    }
    else if (this->client->isConnected())
    {
        // 如果已连接，直接返回成功
        return true;
    }
    if (!this->client->connect(*(this->serverAddress), this->addressType, 1000 * 10))
    {
        Serial.println("Connection failed");

        return false;
    }
    else
    {
        Serial.println("Connected with primary address type");
    }

    Serial.println("Connected to server");

    if (!this->remoteService)
    {
        this->remoteService = this->client->getService(this->ServiceUUID);
        if (!this->remoteService)
        {
            client->disconnect();
            delete this->client;
            this->client = nullptr;
            return false;
        }
    }
    Serial.println("Found server");
    Serial.println("Connected to characteristic");
    if (!this->characteristic)
    {
        this->characteristic = remoteService->getCharacteristic(this->CharacteristicUUID);
        if (!this->characteristic)
        {
            client->disconnect();
            delete this->client;
            this->client = nullptr;
            return false;
        }
    }
    Serial.println("Found characteristic");
    return true;
}

void YunMei::writeCharacteristic(const char *message)
{
    if (!this->connected)
    {
        Serial.println("Not connected to server");
        return;
    }

    size_t len = 0;
    uint8_t *data = hexStringToBytes(message, &len);

    if (data && len > 0)
    {
        this->characteristic->writeValue(data, len);
        Serial.print("Sent HEX: ");
        for (size_t i = 0; i < len; i++)
        {
            Serial.printf("%02X ", data[i]);
        }
        Serial.println();
    }

    if (data)
        free(data); // 使用 malloc 要记得释放
}

bool YunMei::isConnected(){
    return this->connected;
}


bool YunMei::begin()
{
    this->scan();
    int connectRetries = 0;
    while (!this->connectToServer())
    {
        if (connectRetries >= 2)
        {
            return false;
        }
        this->restartScan();
        delay(1000);
        connectRetries++;
    }
    this->connected = true;
    return true;
}

void YunMei::unLock()
{
    this->writeCharacteristic(this->secret.c_str());
}
