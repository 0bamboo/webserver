#include "HttpResponse.hpp"

HttpResponse::HttpResponse(HttpRequest const &request, serverData const &server)
{
    init_response();

    _responseStatus = request.getRequestStatus();
    _method = request.getMethod();
    _httpVersion = request.getHttpVersion();
    _path = request.getPath();
    _postRequestData = request.getBodyParts();
    std::map<std::string, std::string> _headers = request.getHedaers();
    if (_headers.find("Content-Type:") != _headers.end())
        __contentType = request.getHedaers()["Content-Type:"];
    // generate response
    std::string hosts[3] = {"Host", "host", "HOST"};
    for (int i = 0; i < 3; i++)
    {
        if (_headers.find(hosts[i]) != _headers.end())
        {
            _host = request.getHedaers()[hosts[i]];
            break;
        }
    }
    __connection = static_cast<std::string>(request.getConnectionType());

    // init servers find server
    std::cout << "========================>" << _host << "<========================\n";

    _server = server;

    // root

    _root = _server.getRoot();


    // error pages
    __errorPages = _server.getErrorPages();
    if (__errorPages.size())
        _errorPagesExist = true;

    // locations
    _locations = _server.getLocations();
    location _location;
    bool isLocationFounded = false;

    if(_root.empty())
    {
        _errorPagesExist = false;
    }
    // get upload location and cgi
    for (std::vector<location>::iterator it = _locations.begin(); it != _locations.end(); it++)
    {
        if (it->getIsUploadEnable())
        {
            _uploadenabled = true;
            _uploadpath = it->getUploadPath();
        }
        // get cgi
        if (!_php_cgi_exist && it->getIsCgi() && it->getPath() == "*.php")
        {
            _php_cgi_exist = true;
            _php_cgi_path = it->getCgiPath();
        }
        if (!_python_cgi_exist && it->getIsCgi() && it->getPath() == "*.py")
        {
            _python_cgi_exist = true;
            _python_cgi_path = it->getCgiPath();
        }
    }

    /**** match path with location****/
    for (std::vector<location>::iterator it = _locations.begin(); it != _locations.end(); it++)
    {
        if (it->getPath() == _path || std::string(it->getPath() + "/") == std::string(_path))
        {
            _location = *it;
            isLocationFounded = true;
            break;
        }
        if (_path.find(std::string(it->getPath() + "/")) != std::string::npos && _path.find(std::string(it->getPath() + "/")) == 0)
        {
            _location = *it;
            isLocationFounded = true;
            break;
        }
    }
    /**** match path with a cgi ***/
    if (!isLocationFounded)
    {
        if (_path.rfind(".php") != std::string::npos)
        {
            _is_cgi = true;
            for (std::vector<location>::iterator it = _locations.begin(); it != _locations.end(); it++)
            {
                if (it->getPath() == "*.php")
                {
                    _location = *it;
                    isLocationFounded = true;
                    _indices.insert(_php_index);
                    _autoIndex = false;
                    _allowedMethods = _location.getAllowedMethods();
                }
            }
        }
        else if (_path.rfind(".py") != std::string::npos)
        {
            _is_cgi = true;
            for (std::vector<location>::iterator it = _locations.begin(); it != _locations.end(); it++)
            {
                if (it->getPath() == "*.py")
                {
                    _location = *it;
                    isLocationFounded = true;
                    _indices.insert(_python_index);
                    _autoIndex = false;
                    _allowedMethods = _location.getAllowedMethods();
                }
            }
        }
    }

    if (!isLocationFounded) // set default location if no location founded
    {
        if (_locations.size())
            _location = _locations[0];
        else
        {
            _finaleResponse = generateErrorResponse(NOT_FOUND);
            return;
        }
    }

    // if root exist inside location
    if (!_location.getRoot().empty())
    {
        _root = _location.getRoot();
    }

    // if location is redirection
    if (_location.getIsRedirection())
    {
        _finaleResponse = handleRedirection(_host, _location.getReturnData().first, _location.getReturnData().second);
        return;
    }
    else
    {
        if (!_is_cgi)
        {
            _autoIndex = _location.getAutoIndex();
            _allowedMethods = _location.getAllowedMethods();
            if (_location.getIndices().size()) // set indeces
                _indices = _location.getIndices();
        }
    }

    std::cout << "location : " << _location.getPath() << "\n";
    // location  is retdirection or not
    /// allowed methods
    // error pages
    // indeces
    // location is cgi cgi path
    // upload path
    //

    _finaleResponse = generateResponse(_responseStatus, _root, _path); // get config obj ; root
}

void HttpResponse::init_response()
{
    _uploadenabled = false;

    _php_cgi_exist = false;
    _php_index = "index.php";
    _python_cgi_exist = false;
    _python_index = "index.py";
    _is_cgi = false;
    _autoIndex = false;
    // default index
    _indices.insert("index.html");
    _indices.insert("index.htm");

    CRLF_Combination = std::string(CRLF);
    __http = "HTTP/1.1";
    __statusDesciption = "";
    __connection = "keep-alive";
    __contentLength = "150";
    __contentType = "text/html";
    __Date = "";
    __server = "Webserv/1.0";

    //// Successful responses
    __codes[200] = "OK";
    //// Redirection messages
    __codes[300] = "Multiple Choice";
    __codes[301] = "Moved Permanently";
    __codes[307] = "Temporary Redirect";
    __codes[308] = "Permanent Redirect";
    //// Client error responses
    __codes[400] = "Bad Request";
    __codes[404] = "Not Found";
    __codes[403] = "Forbidden";
    __codes[405] = "Method Not Allowed";
    __codes[415] = "Unsupported Media Type";
    //// Server error responses
    __codes[500] = "Internal Server Error";
    __codes[501] = "Not Implemented";
    __codes[505] = "HTTP Version Not Supported";

    ////// conten Types list

    __contentTypesList[".aac"] = "audio/aac";
    __contentTypesList[".abw"] = "application/x-abiword";
    __contentTypesList[".arc"] = "application/x-freearc";
    __contentTypesList[".avif"] = "image/avif";
    __contentTypesList[".avi"] = "video/x-msvideo";
    __contentTypesList[".azw"] = "application/vnd.amazon.ebook";
    __contentTypesList[".bin"] = "application/octet-stream";
    __contentTypesList[".bmp"] = "image/bmp";
    __contentTypesList[".bz"] = "application/x-bzip";
    __contentTypesList[".bz2"] = "application/x-bzip2";
    __contentTypesList[".cda"] = "application/x-cdf";
    __contentTypesList[".csh"] = "application/x-csh";
    __contentTypesList[".css"] = "text/css";
    __contentTypesList[".csv"] = "text/csv";
    __contentTypesList[".doc"] = "application/msword";
    __contentTypesList[".docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    __contentTypesList[".eot"] = "application/vnd.ms-fontobject";
    __contentTypesList[".epub"] = "application/epub+zip";
    __contentTypesList[".gz"] = "application/gzip";
    __contentTypesList[".gif"] = "image/gif";
    __contentTypesList[".htm"] = "text/html";
    __contentTypesList[".html"] = "text/html";
    __contentTypesList[".ico"] = "image/vnd.microsoft.icon";
    __contentTypesList[".ics"] = "text/calendar";
    __contentTypesList[".jar"] = "application/java-archive";
    __contentTypesList[".jpeg"] = "image/jpeg";
    __contentTypesList[".jpg"] = "image/jpeg";
    __contentTypesList[".js"] = "text/javascript ";
    __contentTypesList[".json"] = "application/json";
    __contentTypesList[".jsonld"] = "application/ld+json";
    __contentTypesList[".mid"] = "audio/midi audio/x-midi";
    __contentTypesList[".mjs"] = "text/javascript";
    __contentTypesList[".mp3"] = "audio/mpeg";
    __contentTypesList[".mp4"] = "video/mp4";
    __contentTypesList[".mpeg"] = "video/mpeg";
    __contentTypesList[".mpkg"] = "application/vnd.apple.installer+xml";
    __contentTypesList[".odp"] = "application/vnd.oasis.opendocument.presentation";
    __contentTypesList[".ods"] = "application/vnd.oasis.opendocument.spreadsheet";
    __contentTypesList[".odt"] = "application/vnd.oasis.opendocument.text";
    __contentTypesList[".oga"] = "audio/ogg";
    __contentTypesList[".ogv"] = "video/ogg";
    __contentTypesList[".ogx"] = "application/ogg";
    __contentTypesList[".opus"] = "audio/opus";
    __contentTypesList[".otf"] = "font/otf";
    __contentTypesList[".png"] = "image/png";
    __contentTypesList[".pdf"] = "application/pdf";
    __contentTypesList[".php"] = "application/x-httpd-php";
    __contentTypesList[".ppt"] = "application/vnd.ms-powerpoint";
    __contentTypesList[".pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    __contentTypesList[".rar"] = "application/vnd.rar";
    __contentTypesList[".rtf"] = "application/rtf";
    __contentTypesList[".sh"] = "application/x-sh";
    __contentTypesList[".svg"] = "image/svg+xml";
    __contentTypesList[".swf"] = "application/x-shockwave-flash";
    __contentTypesList[".tar"] = "application/x-tar";
    __contentTypesList[".tif"] = "image/tiff";
    __contentTypesList[".tiff"] = "image/tiff";
    __contentTypesList[".ts"] = "video/mp2t";
    __contentTypesList[".ttf"] = "font/ttf";
    __contentTypesList[".txt"] = "text/plain";
    __contentTypesList[".vsd"] = "application/vnd.visio";
    __contentTypesList[".wav"] = "audio/wav";
    __contentTypesList[".weba"] = "audio/webm";
    __contentTypesList[".webm"] = "video/webm";
    __contentTypesList[".webp"] = "image/webp";
    __contentTypesList[".woff"] = "font/woff";
    __contentTypesList[".woff2"] = "font/woff2";
    __contentTypesList[".xhtml"] = "application/xhtml+xml";
    __contentTypesList[".xls"] = "application/vnd.ms-excel";
    __contentTypesList[".xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    __contentTypesList[".xml"] = "application/xml";
    __contentTypesList[".xul"] = "application/vnd.mozilla.xul+xml";
    __contentTypesList[".zip"] = "application/zip";
    __contentTypesList[".3gp"] = "audio/video";
    __contentTypesList[".3g2"] = "audio/video";
    __contentTypesList[".7z"] = "application/x-7z-compressed";
}

HttpResponse::~HttpResponse()
{
}

std::string HttpResponse::getLocalTime() const
{
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    memset(buffer, '\0', 80);
    strftime(buffer, 80, "%a, %d %h %Y %T GMT", timeinfo);
    return (std::string(buffer));
}

std::string HttpResponse::generateStartLine(unsigned int const &status_code)
{
    if (__codes.find(status_code) != __codes.end())
        return (__http + " " + std::to_string(status_code) + " " + __codes[status_code]);
    else
        return (__http + " " + std::to_string(NOT_FOUND) + " " + __codes[NOT_FOUND]);
}

std::string HttpResponse::generateHeader(unsigned int const &status_code, unsigned int const &body_lenght, std::string const &content_type)
{
    std::string header;
    header += generateStartLine(status_code) + CRLF_Combination;
    header += "Content-Type: " + content_type + CRLF_Combination;
    header += "Content-Lenght: " + std::to_string(body_lenght) + CRLF_Combination;
    header += "Server: " + __server;
    return header;
}

void HttpResponse::print()
{
    std::cout << "+++++++++++++++++++++ Responce BEGIN ++++++++++++++++++++\n";
    std::cout << _finaleResponse << std::endl;
    std::cout << "+++++++++++++++++++++ Responce END ++++++++++++++++++++\n";
}
/*
std::string HttpResponse::generateBody()
{
    return " ";
}
*/
std::string const HttpResponse::defaultServerPages(unsigned int &statusCode) const
{
    if (statusCode == OK)
        return ResponseOK();
    else if (statusCode == BAD_REQUEST)
        return ResponseBadRequest();
    else if (statusCode == NOT_FOUND)
        return ResponseNotFound();
    else if (statusCode == METHOD_NOT_ALLOWED)
        return ResponseMethodNotAllowed();
    else if (statusCode == HTTP_VERSION_NOT_SUPPORTED)
        return ResponseHttpVersionNotSupported();
    else if (statusCode == NOT_IMPLLIMENTED)
        return ResponseMethodNotAllowed();
    else if (statusCode == FORBIDDEN)
        return ResponseForbidden();
    else if (statusCode == INTERNAL_SERVER_ERROR)
        return ResponseForbidden();
    else
        return ResponseNotFound();
}

std::string const HttpResponse::generateErrorResponse(unsigned int errorCode)
{
    if (_errorPagesExist)
    {
        if (__errorPages.find(errorCode) == __errorPages.end())
            return defaultServerPages(errorCode);
        else
        {
            std::string body;
            // get Html page path
            std::string pagePath = _root + __errorPages[errorCode];
            struct stat sb;
            if (!stat(pagePath.c_str(), &sb) && S_ISREG(sb.st_mode))
            {
                int fd = open(pagePath.c_str(), O_RDONLY);
                if (fd > 0)
                {
                    char buffer[(int)sb.st_size];
                    int readedBytes;
                    while ((readedBytes = read(fd, buffer, sb.st_size)) > 0)
                    {
                        buffer[readedBytes] = '\0';
                        body += buffer;
                        if (body.length() >= (unsigned int)sb.st_size)
                            break;
                    }
                    close(fd);
                }
            }
            return (generateHeader(errorCode, body.length(), __contentType) + CRLF_Combination + CRLF_Combination + body);
        }
    }
    else
    {
        return defaultServerPages(errorCode);
    }
}

std::string HttpResponse::generateResponse(unsigned int const code_status, std::string const &root /*or location*/, std::string const &path)
{
    std::string response;
    std::string header;
    std::string body;

    if (code_status != 200)
    {
        return generateErrorResponse(code_status);
    }
    ///
    if (_method == "GET" && _allowedMethods["GET"])
    {
        return handle_GET_Request(root, path);
    }
    else if (_method == "POST" && _allowedMethods["POST"])
    {
        std::cout << "post called\n";
        return handle_POST_Request();
    }
    else if (_method == "DELETE" && _allowedMethods["DELETE"])
    {
        return handle_DELETE_Request(root, path);
    }
    else
        return generateErrorResponse((code_status == NOT_IMPLLIMENTED));
}

std::string HttpResponse::ResponseOK() const
{
    std::string body = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>Welcome to webserv!</title> \r\n</head> \r\n<body>\r\n<center><h1>Welcome to webserv</h1></center>\r\n<hr><center><p>webserv/1.0</p></center>\r\n</body>\r\n</html>\r\n";
    std::string const header = "HTTP/1.1 200 OK" + CRLF_Combination + "Content-Type: text/html; charset=UTF-8\r\nContent-Length: " + std::to_string(body.length()) + "\r\nConnection: Closed\r\nServer: webserv/1.0\r\nDate: " + getLocalTime();

    return (header + CRLF_Combination + CRLF_Combination + body);
}

std::string const HttpResponse::ResponseBadRequest() const
{
    std::string const body = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>400 Bad Request</title> \r\n</head> \r\n<body>\r\n<center><h1>400 Bad Request</h1></center>\r\n<hr><center><p>webserv/1.0</p></center>\r\n</body>\r\n</html>\r\n";
    std::string const header = "HTTP/1.1 400 Bad Request" + CRLF_Combination + "Content-Type: text/html; charset=UTF-8" + CRLF_Combination + "Content-Length: " + std::to_string(body.length()) + CRLF_Combination + "Connection: Closed" + CRLF_Combination + "Server: webserv/1.0" + CRLF_Combination + "Date: " + getLocalTime();
    return (header + CRLF_Combination + CRLF_Combination + body);
}
std::string const HttpResponse::ResponseForbidden() const
{
    std::string const body = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>403 Forbidden</title> \r\n</head> \r\n<body>\r\n<center><h1>403 Forbidden</h1></center>\r\n<hr><center><p>webserv/1.0</p></center>\r\n</body>\r\n</html>\r\n";
    std::string const header = "HTTP/1.1 403 Forbidden" + CRLF_Combination + "Content-Type: text/html; charset=UTF-8" + CRLF_Combination + "Content-Length: " + std::to_string(body.length()) + CRLF_Combination + "Connection: Closed" + CRLF_Combination + "Server: webserv/1.0" + CRLF_Combination + "Date: " + getLocalTime();
    return (header + CRLF_Combination + CRLF_Combination + body);
}
std::string const HttpResponse::ResponseNotFound() const
{
    std::string const body = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>404 Not Found</title> \r\n</head> \r\n<body>\r\n<center><h1>404 Not Found</h1></center>\r\n<hr><center><p>webserv/1.0</p></center>\r\n</body>\r\n</html>\r\n";
    std::string const header = "HTTP/1.1 404 Not Found" + CRLF_Combination + "Content-Type: text/html; charset=UTF-8" + CRLF_Combination + "Content-Length: " + std::to_string(body.length()) + CRLF_Combination + "Connection: Closed" + CRLF_Combination + "Server: webserv/1.0" + CRLF_Combination + "Date: " + getLocalTime();
    return (header + CRLF_Combination + CRLF_Combination + body);
}
std::string const HttpResponse::ResponseMethodNotAllowed() const
{
    std::string const body = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>405 Method Not Allowed</title> \r\n</head> \r\n<body>\r\n<center><h1>405 Method Not Allowed</h1></center>\r\n<hr><center><p>webserv/1.0</p></center>\r\n</body>\r\n</html>\r\n";
    std::string const header = "HTTP/1.1 405 Method Not Allowed" + CRLF_Combination + "Content-Type: text/html; charset=UTF-8" + CRLF_Combination + "Content-Length: " + std::to_string(body.length()) + CRLF_Combination + "Connection: Closed" + CRLF_Combination + "Server: webserv/1.0" + CRLF_Combination + "Date: " + getLocalTime();
    return (header + CRLF_Combination + CRLF_Combination + body);
}
std::string const HttpResponse::ResponseHttpVersionNotSupported() const
{
    std::string const body = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>505 HTTP Version Not Supported</title> \r\n</head> \r\n<body>\r\n<center><h1>505 HTTP Version Not Supported</h1></center>\r\n<hr><center><p>webserv/1.0</p></center>\r\n</body>\r\n</html>\r\n";
    std::string const header = "HTTP/1.1 505 HTTP Version Not Supported" + CRLF_Combination + "Content-Type: text/html; charset=UTF-8" + CRLF_Combination + "Content-Length: " + std::to_string(body.length()) + CRLF_Combination + "Connection: Closed" + CRLF_Combination + "Server: webserv/1.0" + CRLF_Combination + "Date: " + getLocalTime();
    return (header + CRLF_Combination + CRLF_Combination + body);
}

bool HttpResponse::upload(std::string const &path, std::string const &filename, std::string const &data)
{
    struct stat stats;

    if (stat(path.c_str(), &stats) == 0 && S_ISDIR(stats.st_mode))
    {
        std::ofstream outfile(path + "/" + filename);
        outfile << data;
        outfile.close();
        return 0;
    }
    else
    {
        std::cout << "file not created" << std::endl;
        return 1;
    }
}

std::string HttpResponse::readFile(std::string const &file_path)
{

    std::ifstream file(file_path);
    if (file)
    {
        std::ostringstream stream_string;
        stream_string << file.rdbuf();
        return stream_string.str();
    }
    else
        return "error file not opened";
}

std::string HttpResponse::readDirectory(std::string const &root, std::string const &path)
{
    IndexOfHtmlTemplate index(root, path);
    return index.getTable();
}

std::string HttpResponse::handle_GET_Request(std::string const &root, std::string const &path)
{
    std::string response;
    std::string header;
    std::string body;
    unsigned int code_status = OK;

    /// check the path is correct and file request exist
    struct stat sb;
    if (stat(root.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        std::string fullPath = root + path;
        struct stat sb;
        if (stat(fullPath.c_str(), &sb) == 0)
        {
            // check if path is file or directory
            if (S_ISDIR(sb.st_mode))
            {
                if (fullPath[fullPath.length() - 1] == '/')
                {
                    /// check  indeces
                    // std::string default_page = "index.html";
                    //  if php file;
                    std::string default_page;
                    bool default_page_exist = 0;
                    for (std::set<std::string>::iterator it = _indices.begin(); it != _indices.end(); it++)
                    {
                        default_page = *it;
                        if (default_page.rfind(".php") != std::string::npos)
                        {
                            _is_cgi = true;
                            if (stat((fullPath + default_page).c_str(), &sb) == 0)
                            {
                                    return CGI_GET_Request(root, path + default_page, _php_cgi_path);
                            }
                        }
                        else if (default_page.rfind(".py") != std::string::npos)
                        {
                            _is_cgi = true;
                            if (stat((fullPath + default_page).c_str(), &sb) == 0)
                            {
                                    return CGI_GET_Request(root, path + default_page, _python_cgi_path);
                            }
                        }
                        if (stat((fullPath + default_page).c_str(), &sb) == 0)
                        {
                            // check if you have access to the file
                            if (access((fullPath + default_page).c_str(), R_OK) == 0)
                            {
                                default_page_exist = 1;
                                body = readFile(fullPath + default_page);
                                break;
                            }
                            else
                            {
                                return generateErrorResponse(FORBIDDEN);
                            }
                        }
                    }
                    if (!default_page_exist)
                    {
                        // if index not exist check the auto index is enabled  and acces to list content of directory
                        if (!default_page_exist && _autoIndex && access((fullPath).c_str(), R_OK | W_OK) == 0)
                            body = readDirectory(root, path);
                        else
                        {
                            return generateErrorResponse(FORBIDDEN);
                        }
                    }
                }
                else
                { /// generate redirection
                    return handleRedirection(_host, 301, path + "/");
                }
            }
            else // if file open it and ;
            {
                if (fullPath.find(".php") != std::string::npos)
                {
                    return run_CGI(fullPath, _php_cgi_path);
                }
                if (fullPath.find(".py") != std::string::npos)
                {
                    return run_CGI(fullPath, _python_cgi_path);
                }
                if (access((fullPath).c_str(), R_OK) == 0)
                {
                    if (fullPath.find(".") != std::string::npos)
                    {
                        std::string filetype = fullPath.substr(fullPath.find(".", fullPath.length() - 5));
                        if (__contentTypesList.find(filetype) != __contentTypesList.end())
                            __contentType = __contentTypesList[filetype];
                    }
                    body = readFile(fullPath);
                }
                else
                {
                    return generateErrorResponse(FORBIDDEN);
                }
            }
        }
        else // if path not exist return not found
            return generateErrorResponse(NOT_FOUND);
    }
    else
        return generateErrorResponse(NOT_FOUND);
    ///
    header += generateHeader(code_status, body.length(), __contentType);
    return (header + CRLF_Combination + CRLF_Combination + body);
}
std::string HttpResponse::handleRedirection(std::string const &host, int const &code, std::string const &location)
{
    return "HTTP/1.1 " + std::to_string(code) + std::string(" ") + __codes[code] + "\r\nLocation: http://" + host + location + CRLF_Combination + CRLF_Combination;
}

std::string HttpResponse::handle_POST_Request()
{
    // check if upload location exist
    struct stat sb;
    std::string uploadPath = _root + _uploadpath;
    if (stat(uploadPath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode) /* && access(uploadPath.c_str(), W_OK)*/)
    {
        std::cout << "upload : " << uploadPath << "\n";
        for (std::vector<t_bodyPart>::iterator it = _postRequestData.begin(); it != _postRequestData.end(); it++)
        {
            if (!(it->_filename.empty())) // a filename exist
            {
                std::cout << "filename : " << it->_filename << "| data" << it->_data << "\n";
                std::ofstream outFile(uploadPath + "/" + it->_filename,std::ios::out | std::ios::binary);
                if (outFile)
                {
                    outFile << it->_data;
                }
                outFile.close();
            }
        }
        std::string body = "<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title></title> \r\n</head> \r\n<body>\r\n<center><h1>uploaded</h1>\r\n</body>\r\n</html>\r\n";
        std::string header = "HTTP/1.1 200 OK" + CRLF_Combination + "Content-Type: text/html; charset=UTF-8\r\nContent-Length: " + std::to_string(body.length()) + "\r\nConnection: Closed\r\nServer: webserv/1.0\r\nDate: " + getLocalTime();
        return (header + CRLF_Combination + CRLF_Combination + body);
    }
    else
    {
        return generateErrorResponse((_responseStatus == INTERNAL_SERVER_ERROR));
    }
}

std::string HttpResponse::handle_DELETE_Request(std::string const &root, std::string const &path)
{
    std::cout << "DELETE CALLED " << root + path << "\n";
    struct stat sb;
    if (stat((root + path).c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
    {
        if (access((root + path).c_str(), W_OK) == 0 && remove((root + path).c_str()) == 0)
            return "HTTP/1.1 202 Accepted\r\n\r\n";
        else
            return generateErrorResponse(FORBIDDEN);
    }
    else
    {
        return generateErrorResponse(NOT_FOUND);
    }
}

std::string HttpResponse::CGI_GET_Request(std::string const &root, std::string const &path, std::string const &cgi_path)
{
    struct stat sb;
    std::string body;
    std::string header;
    // regular file call
    if (stat((root + path).c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
    {
        body = run_CGI(root + path, cgi_path);
    }
    else // directory
    {
        body = "";
    }
    return header + CRLF_Combination + CRLF_Combination + body;
}

std::string HttpResponse::run_CGI(std::string const &filename, std::string const &cgi_path)
{
    extern char **environ;
    int pipefd[2];
    pid_t pid;
    char **cmd;
    std::string str;

    char buffer[10000];
    int r;
    cmd = (char **)malloc(sizeof(char *) * 3);
    cmd[0] = strdup(cgi_path.c_str());
    cmd[1] = strdup(filename.c_str());
    cmd[2] = NULL;
    pipe(pipefd);
    if (!(pid = fork()))
    {
        close(pipefd[0]);
        dup2(pipefd[1], 1);
        if (execve(cmd[0], cmd, environ) == -1)
        {
            return generateErrorResponse(NOT_FOUND);
        }
    }
    else
    {
        close(pipefd[1]);
        while ((r = read(pipefd[0], &buffer, 1000)) > 1)
        {
            buffer[r] = '\0';
            str.append(buffer);
        }
        close(pipefd[0]);
    }
    std::cout << "++++++++++++++\n";
    std::cout << str << "\n";
    std::cout << "++++++++++++++\n";
    std::string body = str.substr(str.find("\r\n\r\n") + 5);
    std::string header = generateHeader(OK, body.length(), "text/html; charset=UTF-8");
    header.append("\r\nX-Powered-By: PHP/8.1.4");
    return header + CRLF_Combination + CRLF_Combination + body;
}