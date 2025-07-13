# 云莓智能

## 项目介绍

这是为了方便大家使用特意写的 Arduino 库,调用几个函数轻松开门

## 使用说明

在使用前须初始化对象,这里展示的是构造函数

```cpp
YunMei(
        String bleName,              //个人蓝牙名称，自定义
        String bleServerName,       //门锁蓝牙名称
        String bleServerAddress,    //门锁蓝牙地址
        String ServiceUUID,         //门锁蓝牙服务uuid
        String characteristicUUID,  //门锁蓝牙特征uuid
        String secret               //加密后得到的密钥
    );
```

提供了三个 public 函数

1. **bool isConnected()**
   判断当前是否已经与设备连接
   因为低功耗蓝牙设备不允许长时间连接,建议在连接前先调用这个函数,以免造成不必要的资源浪费
1. **bool begin()**
   连接设备,并返回是否连接成功
1. **void unLock()**
   开门

使用示例(这里)
```cpp
#include <YunMei.h>

#define unLockBtn 4

//初始化对象
YunMei yunmei("MyESP32", "06_522", "D1:95:6E:ED:CF:6C",
              "6E400001-B5A3-F393-E0A9-D1956EEDCF6C",
              "6E400002-B5A3-F393-E0A9-D1956EEDCF6C",
              "D00F65637032777363757031706BA8"); //这里示例是我修改过的，不然要开我门了

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(unLockBtn, INPUT_PULLUP);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    
    if (yunmei.isConnected()) {     //判断是否已连接
      yunmei.unLock();
      Serial.println("解锁成功");
    } else if(yunmei.begin()) {     //连接门锁
      yunmei.unLock();              //开锁
      Serial.println("解锁成功");
    }else{
      Serial.println("BLE连接失败");
    }

    delay(1000);
  } else {
    Serial.println("初次启动");
  }

  Serial.println("进入 Deep-sleep...");
  esp_sleep_enable_ext0_wakeup((gpio_num_t)unLockBtn, 1);  // 高电平唤醒
  esp_deep_sleep_start();
}

void loop() {

}
```
