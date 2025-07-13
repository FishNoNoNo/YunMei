#include <YunMei.h>

#define unLockBtn 4

//初始化对象
YunMei yunmei("MyESP32", "06_522", "D1:95:6E:ED:CF:6D",
              "6E400001-B5A3-F393-E0A9-D1956EEDCF6D",
              "6E400002-B5A3-F393-E0A9-D1956EEDCF6D",
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