#include "httpresponse.h"

using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
    { ".mp3",   "audio/mpeg"},
    { ".mp4",   "video/mp4"},
};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const unordered_map<int, string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_ = false;
    mapFilePtr_ = nullptr; 
    statFile_ = { 0 };
};

HttpResponse::~HttpResponse() {
    unMapFilePtr();
}

void HttpResponse::init(const string& srcDir, string& path, bool isKeepAlive, int code){
    assert(srcDir != "");
    //清空映射区
    if( mapFilePtr_ ) unMapFilePtr();
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mapFilePtr_ = nullptr; 
    statFile_ = { 0 };
}

void HttpResponse::makeResponse(Buffer& buff) {
    //不是目录
    if(stat((srcDir_ + path_).data(), &statFile_) < 0 || S_ISDIR(statFile_.st_mode)) {
        code_ = 404;
    }
    //判断是否有读权限
    else if(!(statFile_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) { 
        code_ = 200; 
    }
    errorHtml_();
    makeStateLine_(buff);
    makeHeader_(buff);
    makeContent_(buff);
}

char* HttpResponse::getFilePtr() {
    return mapFilePtr_;
}

size_t HttpResponse::getFileLen() const {
    return statFile_.st_size;
}

void HttpResponse::errorHtml_() {
    //如果上面stat失败，重新打开
    if(CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).data(), &statFile_);
    }
}

void HttpResponse::makeStateLine_(Buffer& buff) {
    string status;
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.appendByStr("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::makeHeader_(Buffer& buff) {
    buff.appendByStr("Connection: ");
    if(isKeepAlive_) {
        buff.appendByStr("keep-alive\r\n");
        buff.appendByStr("keep-alive: max=6, timeout=120\r\n");
    } else{
        buff.appendByStr("close\r\n");
    }
    buff.appendByStr("Content-type: " + getFileType_() + "\r\n");
}

void HttpResponse::makeContent_(Buffer& buff) {
    int fd = open((srcDir_ + path_).data(), O_RDONLY);
    if(fd < 0) { 
        errorContent(buff, "File NotFound!");
        return; 
    }

    //将文件放在映射区提高读效率
    //MAP_PRIVATE 建立一个写入时拷贝的私有映射，即写时复制
    int* mapRet = (int*)mmap(0, statFile_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(*mapRet == -1) {
        errorContent(buff, "File NotFound!");
        return; 
    }
    mapFilePtr_ = (char*)mapRet;
    close(fd);
    buff.appendByStr("Content-length: " + to_string(statFile_.st_size) + "\r\n\r\n");
}

void HttpResponse::unMapFilePtr() {
    if(mapFilePtr_) {
        munmap(mapFilePtr_, statFile_.st_size);
        mapFilePtr_ = nullptr;
    }
}

string HttpResponse::getFileType_() {
    //逆向查找
    //if 没找到'.' 则idx==npos
    string::size_type idx = path_.find_last_of('.');
    if(idx == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::errorContent(Buffer& buff, string message) 
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.appendByStr("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.appendByStr(body);
}
