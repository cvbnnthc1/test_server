#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include <stdio.h>

using boost::asio::ip::tcp;

class session
{
public:
  session(boost::asio::io_service& io_service)
    : socket_(io_service)
  {
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

private:
  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if (!error)
    {
      int pos = std::find(data_, data_ + bytes_transferred, '\n') - data_;
      std::string request = std::string(data_, 4, pos - 14);   //Парсим запрос
      std::string file_path = parse_path(request);
      if(file_path == "") //неправильный запрос
      {
        std::string response(kWrongRequest);
        boost::asio::async_write(socket_,
            boost::asio::buffer(response.c_str(), response.size() - 1),
            boost::bind(&session::handle_write, this,
              boost::asio::placeholders::error));
      }
      else
      {
        file_path = std::string(kHomeDir) + file_path;
        std::ifstream ifs;
        ifs.open(file_path.c_str(), std::ios::binary);
        if(!ifs.is_open())  //файл не найден
        {
          std::string response = kNotFound;
          boost::asio::async_write(socket_,
              boost::asio::buffer(response.c_str(), response.size() - 1),
              boost::bind(&session::handle_write, this,
                boost::asio::placeholders::error));
        }
        else
        {
          send_file(ifs, file_path);
        }
      }
    }
    else
    {
      delete this;
    }
  }

  void send_file(std::ifstream& ifs, std::string& file_name)
  {
     char temp_buff[100];
     int fsize = ifs.tellg();
     ifs.seekg(0, std::ios::end );
     fsize = ifs.tellg() - fsize; //считаем размер файла
     ifs.seekg(0, std::ios::beg);
     sprintf(temp_buff, "%d", fsize);  //переводим в строку
     std::string okStatus = std::string("HTTP/1.1 200 OK\r\n"
                   "Content-Type: application/octet-stream/html\r\n"
                   "Connection: close\r\n"
                   "Content-Length: ") + std::string(temp_buff) + std::string("\r\n"
                   "\r\n"
                   "s"); //ответ браузеру
      boost::asio::write(socket_, boost::asio::buffer(okStatus, okStatus.size() - 1));
      char* buffer = new char[fsize];
      ifs.read (buffer, fsize);
      ifs.close();
      boost::asio::async_write(socket_,
          boost::asio::buffer(buffer, fsize),
          boost::bind(&session::handle_write, this,
            boost::asio::placeholders::error)); //отправка файла
  }

  void handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
      socket_.async_read_some(boost::asio::buffer(data_, max_length),
          boost::bind(&session::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      delete this;
    }
  }
  std::string parse_path(std::string& request)
  {
      if(request.substr(0, 4) == std::string("/get") && request[request.size() - 1] != '/')
      {
        return request.substr(4, request.size() - 1);
      }
      else
      {
        return std::string(""); //неправильный запрос
      }
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
  const char* kNotFound = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length:20\r\n\r\n Not found ";
  const char* kWrongRequest = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length:20\r\n\r\n Wrong request "; //константы для ответов браузеру
  const char* kHomeDir = "D:/projects/server2/test_server";
};

#endif // SESSION_H_INCLUDED
