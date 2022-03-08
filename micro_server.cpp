#include "micro_server.hpp"

int main()
{

    // Create a socket (IPv4, TCP)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        std::cout << "Failed to create socket. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    // Listen to port 9999 on any address
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(PORT); // htons is necessary to convert a number to
                                     // network byte order
    if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
        std::cout << "Failed to bind to port 9999. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    // Start listening. Hold at most 10 connections in the queue
    if (listen(sockfd, 10) < 0)
    {
        std::cout << "Failed to listen on socket. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    // Grab a connection from the queue
    size_t addrlen = sizeof(sockaddr);
    
   while (1)
    {
        /* code */

        int connection = accept(sockfd, (struct sockaddr *)&sockaddr, (socklen_t *)&addrlen);
        if (connection < 0)
        {
            std::cout << "Failed to grab connection. errno: " << errno << std::endl;
            exit(EXIT_FAILURE);
        }
        char buffer[10000];
        int bytesRead;
        //bytesRead = read(connection, buffer, 10000);
        std::string recievedData;
        while ((bytesRead = read(connection, buffer, 10000)) > 0)
        {
            std::cout << "-----------------------------readed " << bytesRead << "\n";
            buffer[bytesRead] = '\0';
            recievedData = recievedData + buffer;
            std::cout << recievedData;
            std::cout << "-------------\n";
            if (bytesRead < 10000)
                break;
        }

        (void)bytesRead;
        //buffer[bytesRead] = '\0';
        //std::cout << buffer;
        //parseRequest(buffer);
        //parseRequest(recievedData);


        std::cout << recievedData;
        /////// request parse begin 
        HttpRequest request(recievedData);
        ////// request parse end
        request.print();
        std::string myResponse = "HTTP/1.1 200 OK\r\nDate: Mon, 27 Jul 2009 12:28:53 GMT\r\nServer: Apache/2.2.14 (Win32)\r\nLast-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\nContent-Length: 88\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n<html><body><h1>Hello, World!</h1></body></html>";
        send(connection, myResponse.c_str(), myResponse.size(), 0);
        // close(connection);
    }
    close(sockfd);
    // Read from the connection

    // Send a message to the connection
    // Close the connections
}

/*
parsing request, set status code
parse first header -> method: [GET, POST, DELETE], url(3la 7ssab config file), protocol = HTTP/1.1
everythings else: map[key] = value

example for post method:
special variable: std::string postbody
POST / HTTP/1.1
Host: localhost:9998
Content-Type: application/json
Content-Length: 15

{
    "t": 5,
    "a": 6
}

example for post method with files:
struct fileParse {
    std:string ConDisposition;
    std:string ContType;
    std::string body;
    string filename;
}
special variable for files: std::vector<struct fileParse> v;
POST / HTTP/1.1
Host: localhost:9998
other-headers
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryePkpFF7tjBAqx29L

------WebKitFormBoundaryePkpFF7tjBAqx29L
Content-Disposition: form-data; name="MAX_FILE_SIZE"

100000
------WebKitFormBoundaryePkpFF7tjBAqx29L
Content-Disposition: form-data; name="uploadedfile"; filename="hello.o"
Content-Type: application/x-object

... contents of file goes here ...
------WebKitFormBoundaryePkpFF7tjBAqx29L--
*/