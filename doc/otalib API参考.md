## OTALIB

### buffer.hpp

#### Buffer

```C++
class Buffer;
```

##### 描述

​	Buffer类用于客户端与网络端之间的通信。

------------------------------------------------------

### delta_log.h

#### DeltaInfo

``` c++
enum class Action { ADD, DELETEACT, DELTA};
enum class Category { FILE, DIR };
struct DeltaInfo {
  Action action;
  Category category;
  QString position;
  QString opaque;
};
using DeltaInfoStream = QVector<DeltaInfo>;
```

##### 描述

​	DeltaInfoStream两个版本间所有不同文件的变动。

​	结构体DeltaInfo记录了两个版本间单个文件(目录)的变动。

​	枚举类Action描述了新版本中对旧版本文件(目录)的改动：增、删、改。

​	枚举类Category描述了文件的类别：目录或是文件。



#### WriteDeltaLog

```c++
void writeDeltaLog(QTextStream& log, const DeltaInfo& info);
```

##### 描述

​	该函数将结构体DeltaInfo写入文件。

##### 参数

​	**log：需要写入的文件**

​	**info：需要写入的信息**

##### 备注

​	log在函数调用前应该处于打开状态。



#### readDeltaLog

```C++
DeltaInfoStream readDeltaLog(QTextStream& log);
```

##### 描述

​	该函数将从文件中读取所有的DeltaInfo。

##### 参数

​	**log：需要读取的文件**

##### 返回值

​	**DeltaInfoStream：读取完毕的DeltaInfo，放在了QVector内。**

##### 备注

​	log在函数调用前应该处于打开状态。

-------------------------

### diff.h

#### generateDeltaPack

``` C++
bool generateDeltaPack(QDir& dir_old, QDir& dir_new, QDir& rollback_dest, QDir& update_dest);
```

##### 描述

​	该函数在目标目录下自动生成差分补丁。

##### 参数

​	**dir_old：旧版本App的目录**

​	**dir_new：新版本App的目录**

​	**rollback_dest：在该目录下生成回滚包(dir_new --> dir_old)**

​	**update_dest：在该目录下生成升级包(dir_old --> dir_new)**

##### 返回值

**bool：差分补丁生成是否成功**



#### applyDeltaPack

```C++
bool applyDeltaPack(const QDir& pack, const QDir& target);
```

##### 描述

​	该函数在App上应用差分补丁

##### 参数

​	**pack：差分包的目录**

​	**target：App的根目录**

##### 返回值

​	**bool：差分补丁应用是否成功**

----------------------

### file_logger.h

#### FileLogger

```C++
struct FileLogger;
```

##### 描述

​	该类用于记录App中的“核心”文件。

##### FileLogger::FileLogger(const QString& log_file)

	###### 描述

​	将log_file作为file_log的文件路径并打开它。

###### 参数

​	**log_file：需要打开的文件**



##### static merkle_hash_t FileLogger::GetHashFromLogFile(const QString &log_file)

###### 描述

​	将log_file作为file_log的文件路径，打开后按行读取

##### void FileLogger::Close()

###### 描述

​	关闭FileLogger



##### void FileLogger::AppendDir(const QString &dir_name)

###### 描述

​	将目录下所有文件加入到FileLogger的记录中

###### 参数

​	**dir_name：需要加入FileLogger的目录**



##### void FileLogger::Append(const QString &file)

###### 描述

​	将文件加入FileLogger的记录中

###### 参数

​	**file：需要加入的文件路径**

----------------------------------

### Otaerr.hpp

#### 	OTAError

	##### 		描述

​			该类用于程序运行过程中的错误信息的输出，通过Index()成员函数可以得出错误类型。

-------------------------------

### pack_apply.hpp

#### 	getPackFromPaths(...)

```C++
template <typename VersionType, typename CallbackOnFind,
          typename EdgeType = ::std::pair<VersionType, VersionType>>
void getPackFromPaths(const QDir& dest_dir,const ::std::vector<EdgeType>& paths, CallbackOnFind&& callback)
```

	##### 描述

​	该函数用于从VersionMap中search()返回的路径处在服务器储存数据的目录下找出相关文件。然后将其复制到目标目录下。

##### VersionType

​	选用版本的类型。

##### CallbackOnFind

```c++
using CallbackType = ::std::tuple<QFileInfo, QFileInfo, QFileInfo>(const VersionType& v1, const VersionType& v2)
```

	###### 	返回值

​		返回值为一个tuple，第一个元素是差分包的路径，第二个是打完差分补丁后App的当前版本校验码，第三个是当前差分包的签名文件。

	###### 	参数

​		v1为旧版本的版本号，v2为新版本的版本号。

##### 参数

​	**dest_dir：目标目录**

​	**paths：VersionMap中调用search()的到相关路径信息**

​	**callback：用于寻找相关文件的回调函数**



#### applyPackOnApp(...)

```c++
template <typename VersionType, typename EdgeType = ::std::pair<VersionType, VersionType>>
void applyPackOnApp(const QDir& app_root, const QDir& pack_root, const QFileInfo& pubkey, bool safe_mode = true)
```

##### 描述

​	该函数将指定目录下的差分补丁应用到App上。每次应用差分补丁前会先验证差分包的签名，如验证不通过则禁止升级。

##### VersionType

​	选用版本的类型

##### 参数

​	**app_root：App的根目录**

​	**pack_root：差分包的根目录**

​	**pubkey：App公钥的路径**

​	**safe_mode：是否开启安全模式**

##### 安全模式

​	在开启安全模式下时，每次应用差分包后都会立刻计算一次App当前版本的校验码，将其与服务器上的校验码进行对比，如失败则立刻退出升级流程并报错。

------------------------------------

### property.hpp

#### 描述

​	记录了一些服务器与App都会用到的一些文件名，和记录App相关信息的读取。

#### Property

```c++
struct Property {
  QString app_name_;
  QString app_type_;
  QString app_version_;
};
```

	##### 描述

​	记录了App的名字、类别和版本信息。



#### static Property ReadProperty(const QString& path = kPropertyPath)

```c++
static Property ReadProperty(const QString& path = kPropertyPath);
```

##### 描述

​	读取App的一些信息。

##### 参数

​	**path：property.json文件的路径**

##### 返回值

​	**Property：App的信息**

--------------------

### sha256_hash.h

#### 描述

​	对建立merkle树提供支持。

------------------------------------

	### shell_cmd.hpp

#### 描述

​	该文件记录了一些常用的linux shell指令和一些openssl的shell指令（使用API存在bug，被迫转为使用shell命令）。

---------------

### signature.h

#### 描述

​	该文件封装了一些shell指令，提供API给其他模块来进行rsa签名和验证。

#### void genKey(const QString& prikey_file, const QString& pubkey_file)

	##### 描述

​	在当前目录下产生一对RSA密钥。

##### 参数

**prikey_file：私钥的文件路径，在此路径上创建私钥**

**pubkey_file：公钥的文件路径，在此路径上创建公钥**



#### bool sign(const QFileInfo& target,  const QFileInfo& prikey, const QString& version) noexcept

##### 描述

​	使用私钥对目标文件进行签名。

##### 参数

​	**target：需要进行签名的文件**

​	**prikey：需要使用的私钥**

​	**version：路径前缀（服务器使用时需要提供）**

##### 返回值

​		**bool：签名是否成功**



#### bool verify(const QFileInfo& hash, const QFileInfo& signature, const QFileInfo& pubkey) noexcept

##### 描述

​	使用公钥对签名进行验证

##### 参数

​	**hash：需要验证的文件的摘要**

​	**signature：需要验证的文件的签名文件**

​	**pubkey：需要使用的公钥**

##### 返回值

​	**bool：验证是否成功**

--------------------------------

### ssl_socket_client.hpp

#### 描述

​	该文件有一个SSLSocketClient类，其提供了一些App与服务器通信的一些接口。

#### SSLSocketClient

##### 	描述

​		提供了服务器与客户端通信的一些接口

##### 	explicit SSLSocketClient(const QString& ip, quint16 port)

	###### 		描述

​			类的构造函数

		###### 		参数

​			**ip：服务器IP地址**

​			**port：服务器端口号**



	##### 	bool connectServer(quint32 timeout_second, int* error)

###### 		描述

​			连接到服务器

###### 		参数

​			**timeout_second：连接超时时间**

​			**error：错误码**

###### 		返回值

​			**bool：是否与服务器连接成功**



	##### 	int send(const QByteArray& data)

	###### 		描述

​			向服务器发送数据

###### 		参数

​			**data：需要发送的数据**

###### 		返回值

​			**int：错误码**



##### 	int recv()

###### 		描述

​			从服务器接收数据

###### 		返回值

​			**int：错误码**

​	

##### 	decltype(auto) sender()

###### 		描述

​			得到发送缓冲区

###### 		返回值

​			**Buffer：发送缓冲区**



##### decltype(auto) recver()

###### 		描述

​			得到接收缓冲区

###### 		返回值

​			**Buffer：接收缓冲区**

------------------

### update_strategy.hpp

	#### 描述

​	提供了一些用于服务器与客户端进行数据交换的函数。

-------------------

### vcm.hpp

#### 描述

​	服务器的核心数据结构，这个数据结构管理着所有的服务器端上所有的版本。

------------------

### version.hpp

#### 描述

​	使用CRTP，使版本类型具有必要的接口，同时控制VersionMap的大小。