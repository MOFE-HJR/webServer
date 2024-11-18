#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    
    //制作response
    void makeResponse(Buffer& buff);

    //清空映射区
    void unMapFilePtr();

    //获取映射区的指针
    char* getFilePtr();

    //获得打开文件长度
    size_t getFileLen() const;

    //编辑失败文件
    void errorContent(Buffer& buff, std::string message);

    //返回code
    int Code() const { return code_; }

private:
    void makeStateLine_(Buffer &buff);
    void makeHeader_(Buffer &buff);
    void makeContent_(Buffer &buff);

    void errorHtml_();

    //转换本地文件格式为浏览器格式并返回
    std::string getFileType_();

    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;
    
    char* mapFilePtr_; //存储mmap文件指针
    struct stat statFile_;//存储打开的文件的stat

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;//对应的文件格式转换
    static const std::unordered_map<int, std::string> CODE_STATUS;//code的状态码
    static const std::unordered_map<int, std::string> CODE_PATH;//code对应的页面
};


#endif //_HTTPRESPONSE_H_