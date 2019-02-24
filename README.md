# dueros
baidu lightdueros for linux and Raspberry Pi 

#DCS3.0 linux 使用说明(下面所有说明在 linux,树莓派 3B/3B+中验证测试通过)

—————————————————————————————————————————————————

### 1. 编译环境搭建：

#### gstreamer1.0 ：
	sudo apt-get update
	sudo apt-get install gstreamer1.0*
	
	安装成功下面三行不需要执行，安装失败，请按照下面分三步分开安装
	sudo apt-get install libgstreamer1.0*
	sudo apt-get install gstreamer1.0-omx-generic*
	sudo apt-get install gstreamer1.0-plugins*

#### snowboy 使用的第三方库
	sudo apt-get install libatlas-base-dev
	
#### alsa ：

	sudo apt-get install alsa-base alsa-utils libasound2-dev


### 2. 编译方式：

在工程文件下运行以下命令：

1,make


### 3. 运行方式：

运行编译生成的可执行文件`duerospi`， -p `<路径>/profile`, -w '[路径]/唤醒词模型文件'

如果不指定唤醒词模型，默认为“小度小度”.

例如：
	./duerospi -p ./profile
	./duerospi -p ./profile -w ./resources/models/snowboy.umdl
	
### 4. 按键说明：

运行成功后，确保键盘不在大写锁定状态，所有按键都不需要长按。

 - q ： 退出
 - w ： 增加音量
 - s ： 减小音量
 - a ： 上一曲
 - d ： 下一曲
 - e ： 静音
 - z ： 暂停/开始 （播放）
 - x ： 开始录音
 - c ： 切换语音交互模式 （0：普通模式，1：中翻英，2：英翻中）
  
  ### 5. 树莓派运行环境
  
  树莓派默认没有麦克风，要接麦克风才可以正常使用，下面是树莓派专用的麦克风阵列淘宝购买链接为： 
  https://item.taobao.com/item.htm?spm=a230r.1.14.7.13d7138aIGBFSI&id=586904707401&ns=1&abbucket=2#detail 
 
  如果上面购买链接不存在 ，请在店铺主页中查找 https://diyiot.taobao.com/ 
    
  
