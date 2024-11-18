#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../connectpool/connection.h"
#include "../connectpool/connectpool.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    
    HttpRequest() { init(); }
    ~HttpRequest() = default;

    void init();

    //解析request报文
    bool parse(Buffer& buff);

    //返回path
    std::string& path();

    //判断是否活跃
    bool isKeepAlive() const;


private:
    bool parseRequestLine_(const std::string& line);
    void parseHeader_(const std::string& line);
    void parseBody_(const std::string& line);
    void parsePath_();

    //用于用户认证
    void parsePost_();
    void parseFromUrlencoded_();
    static bool userVerify_(const std::string& name, const std::string& pwd, bool isLogin);
    static int converHex_(char ch);

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;//存储head的k-v
    std::unordered_map<std::string, std::string> post_;//存储用户输入的用户名和密码

    static const std::unordered_set<std::string> DEFAULT_HTML;//存储6个基本页面名字
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;//区分登录和注册
    
};


#endif //_HTTPREQUEST_H_