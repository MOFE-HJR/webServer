#include "httprequest.h"
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/audio", "/picture", };

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };

void HttpRequest::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::isKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.readableBytes() <= 0) {
        return false;
    }
    while(buff.readableBytes() && state_ != FINISH) {
        //按行读取
        //左闭右开
        char* lineEnd = search(buff.beginReadPtr() , buff.beginWritePtr() , CRLF , CRLF + 2);
        std::string line(buff.beginReadPtr(), lineEnd);
        //state_初始化为request_line
        switch(state_)
        {
        case REQUEST_LINE:
            if(!parseRequestLine_(line)) {
                return false;
            }
            parsePath_();
            break;    
        case HEADERS:
            parseHeader_(line);
            if(buff.readableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            parseBody_(line);
            break;
        default:
            break;
        }
        if(lineEnd == buff.beginWritePtr()) { break; }
        buff.offReadPosByPtr(lineEnd + 2);
        line.clear();
    }
    return true;
}

void HttpRequest::parsePath_() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        //"/index" , "/register" , "/login" ,"/welcome" , "/audio" , "/picture"
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::parseRequestLine_(const string& line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if(regex_match(line, subMatch, patten)) {   
        //get /mofe HTTP/1.1 
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    return false;
}

void HttpRequest::parseHeader_(const string& line) {
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;
    //空行标志着head结束
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        state_ = BODY;
    }
}

void HttpRequest::parseBody_(const string& line) {
    body_ = line;
    parsePost_();
    state_ = FINISH;
}

int HttpRequest::converHex_(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

void HttpRequest::parsePost_() {
    //post请求且以浏览器默认编码格式，使用'&'分割键值对
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        parseFromUrlencoded_();
        //登录1、注册0
        if(DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                //用户认证
                if(userVerify_(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } 
                else {
                    path_ = "/error.html";
                }
            }
        }
    }   
}

void HttpRequest::parseFromUrlencoded_() {
    if(body_.size() == 0) { return; }

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            //保存key
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            //加号变空格
            body_[i] = ' ';
            break;
        case '%':
            //将十六进制转为10进制
            num = converHex_(body_[i + 1]) * 16 + converHex_(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            //构建k-y
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            break;
        default:
            break;
        }
    }
    //保存一行中最后一个value
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::userVerify_(const string &name, const string &pwd, bool isLogin) {
    if(name == "" || pwd == "") { return false; }
    
    //从连接池中获取一个连接
	ConnectPool_ *pool = ConnectPool_::getConnectPool();
	shared_ptr<Connection_> sp = pool->getConnect();
    
    unsigned int j = 0;
    char sql[256] = {0};

    //结果集
    MYSQL_RES *res = nullptr;
    
    //limit 1 返回一条
    sprintf(sql,"select username, password from user where \
        username='%s' limit 1",name.c_str());

    if(!(sp->update(sql))) { 
        return false; 
    }

    //获取结果集
    bool flag = false;
    res = sp->query(sql);
    //获取行的数量
    j = sp->getRowNum(res);

    if( j == 0 ){
        if( !isLogin ) {
            //没有用过的用户名+注册状态
            bzero(sql , 256);
            snprintf(sql , 256 , "insert into user(username, password) values('%s','%s')" ,
                name.c_str() , pwd.c_str());
            if( sp->update(sql) ) flag = true;
        }
    } else{
        if( isLogin ){
            //查找到用户+登录状态
            while( MYSQL_ROW row = sp->getResRow(res)) {
                string password(row[1]);
                if( pwd == password ) flag = true;
            }
        }
    }

    //释放结果集
    sp->freeRes(res);
    return flag;
}

std::string& HttpRequest::path(){
    return path_;
}
