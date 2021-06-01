# App 文件组织

​	App根目录下property.json文件以及file_log文件、以及本身的可执行文件app和负责appOTA升级的可执行文件update

## property.json

​	该文件下记录了App的一些信息：App名字、App类型以及App的当前版本。

## file_log

​	该文件下记录了属于App的一些"核心"文件，"核心"意为纳入file_log记录，参与到计算App校验码中的文件。App运行中产生的临时文件以及App的配置文件不属于”核心“文件。计算App校验码时，按行读取file_log中”核心“文件路径的记录，然后对这些文件计算其哈希值，并建立起一颗Merkle树，树的根节点便作为App的校验码。

## app

​	该文件属于演示app的可执行文件，

## update

​	该文件属于demo的OTA升级模块，app进行升级时会新开一个进程来调用update，而app会在调用update后退出。