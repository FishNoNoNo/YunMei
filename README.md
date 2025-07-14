# 云莓智能第三方

## 项目介绍

这是为了方便大家使用特意写的 Arduino 库,调用几个函数轻松开门

如果对具体怎么实现的感兴趣,可以看我的微信公众号的文章[云莓智能深度解析](https://mp.weixin.qq.com/s/d183nESuxdFjtkC82yFAww)

## 使用说明

### 获取信息

运行下面的 python 代码,获取你的门锁信息

```python
import requests
import hashlib

# 在这里填写账号密码
userName=''

userPwd=''

lockInfo={}

def login():
    url='https://base.yunmeitech.com/login'
    data={
        "userName":userName,
        "userPwd":hashlib.md5(userPwd.encode('utf-8')).hexdigest()
    }
    response=requests.post(url=url,data=data)

    # print(response.text)

    res_data=response.json()

    userId=res_data['o']['userId']
    token=res_data['o']['token']

    return userId,token

def getNewToken():
    global token
    url = "https://base.yunmeitech.com/userschool/getbyuserid"
    data={
        "userId":userId
    }
    headers={
        "tokenData":token,
        "tokenUserId":str(userId)
    }
    response=requests.post(url=url,data=data,headers=headers)

    # print(response.text)
    res_data=response.json()

    schoolNo=res_data[0]['schoolNo']
    token=res_data[0]['token']

    return schoolNo

def getLockInfo():
    url = "https://xiaoyun.yunmeitech.com/dormuser/getuserlock"
    data = {
        "schoolNo": schoolNo
    }

    headers={
        "tokenData":token,
        "tokenUserId":str(userId)
    }

    response = requests.post(url=url, data=data, headers=headers)

    res_data = response.json()

    lockInfo["bleServerName"] = res_data[0]["lockName"]
    lockInfo["bleServerAddress"] = res_data[0]["lockNo"]
    lockInfo["serviceUUID"] = res_data[0]["lockServiceUuid"]
    lockInfo["characterUUID"] = res_data[0]["lockCharacterUuid"]

    return res_data[0]["lockSecret"]

def crypto():
    import binascii
    from Crypto.Cipher import AES
    from Crypto.Util.Padding import pad
    from typing import List

    def string_to_hex(s: str) -> str:
        return "".join(f"{ord(c):02x}" for c in s)

    def decimal_to_hex(d: int, length: int = 2) -> str:
        hex_str = f"{d:x}"
        return hex_str.zfill(length)

    def hex2buf(hex_str: str) -> bytes:
        return binascii.unhexlify(hex_str)

    def B(e: str, i: List[str], n: List[str]) -> List[bytes]:
        r = []
        o = 0
        c = 0
        u = ""

        if n and n[0] == "cZ9FwvPi":
            u += string_to_hex(n[0])
            c = len(n[0])
        else:
            u += string_to_hex(e)
            o = 0
            for idx in range(len(i)):
                u += i[idx] + string_to_hex(n[idx])
                o += len(n[idx])

            c = 2 + len(e) + len(i) + o
            u = "D0" + decimal_to_hex(c, 2) + u

        l = hex2buf(u)

        for f in range((len(l) + 19) // 20):
            start = f * 20
            end = start + 20
            r.append(l[start:end])

        return r

    return B(lockSecret, ["A7"], [""])[0].hex().upper()


userId,token=login()
schoolNo=getNewToken()
lockSecret=getLockInfo()
lockInfo['secret']=crypto()

print(lockInfo)
```

### 在 esp32 上实现

将本项目下载后添加到你的 Arduino 的 library 目录下

```
library/
├── YunMei/
    ├── ...
```

**构造函数**

```cpp
YunMei(
        String bleName,              //个人蓝牙名称，自定义
        String bleServerName,       //门锁蓝牙名称
        String bleServerAddress,    //门锁蓝牙地址
        String serviceUUID,         //门锁蓝牙服务uuid
        String characteristicUUID,  //门锁蓝牙特征uuid
        String secret               //加密后得到的密钥
    );
```

**工具函数**

1. **bool isConnected()**
   判断当前是否已经与设备连接
   因为低功耗蓝牙设备不允许长时间连接,建议在连接前先调用这个函数,以免造成不必要的资源浪费
1. **bool begin()**
   连接设备,并返回是否连接成功
1. **void unLock()**
   开门

## 使用示例

可以直接在 Arduino 的示例中打开这个项目

```cpp
#include <YunMeiESP32.h>

#define unLockBtn 4

//初始化对象
YunMeiESP32 yunmei("MyESP32", "06_522", "D1:95:6E:ED:CF:6D",
              "6E400001-B5A3-F393-E0A9-D1956EEDCF6D",
              "6E400002-B5A3-F393-E0A9-D1956EEDCF6D",
              "D00F65637032777363757031706BA8"); //这里示例是我修改过的，不然要开我门了

void setup() {
  Serial.begin(115200);

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
