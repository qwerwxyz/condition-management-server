#include "widget.h"
#include "ui_widget.h"

void Widget::initTCP()
{
    qDebug()<<"初始化TCP";
    mainserver=new QTcpServer(this);
    connect(mainserver,&QTcpServer::newConnection,this,&Widget::processNewConnection);
}

void Widget::newConnection()
{
    mainserver->listen(QHostAddress("10.128.245.186"),5555);//set ip and port
    qDebug()<<"开始TCP监听";
}

void Widget::endConnection()
{
    mainserver->close();
}

void Widget::receiveData() //接收消息并调用处理函数
{
    /*QDataStream in(mainserver);
       in.setVersion(QDataStream::Qt_5_9); //设置数据流的版本，客户端和服务器端使用的版本要相同
       if(blocksize == 0){
           if(mainserver->bytesAvailable()<(int)sizeof(quint16)) return;    //判断接收的数据是否大于两字节，也就是文件的大小信息所占的空间
           in >> blocksize;    //如果是则保存到blocksize变量中，否则直接返回，继续接收数据
       }
       if(mainserver->bytesAvailable()<blocksize) return;   //如果没有得到全部的数据，则返回，继续接收数据
       in >> message;  //将接收到的数据存放到变量中

       QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toLocal8Bit().data());
       QJsonObject subrespond = jsonDoc.object();
       //parseData(subrespond);*/

    qDebug()<<"接收到一个包";

    QString recvdata;
    for(QMap<int,QTcpSocket*>::iterator i=roomsocketmap.begin();i!=roomsocketmap.end();++i){  //遍历socketmap，处理其中有消息的端口
        if(i.value()->bytesAvailable()>0){  //如果有消息
            qDebug()<<"消息来自已建立的链接,长度为："<<i.value()->bytesAvailable();
            QDataStream in(i.value());
            in.setVersion(QDataStream::Qt_5_9);
            in >> blocksize;
            in >> recvdata;
            qDebug()<<"接受的消息为"<<recvdata;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(recvdata.toLocal8Bit().data());
            QJsonObject subrespond = jsonDoc.object();
            parseData(subrespond);
        }
    }
    for(int i=unknownconnection.size()-1;i>=0;--i)
    {     //遍历未知房间的socket
        if(unknownconnection[i]->bytesAvailable()>0)
        {
            qDebug()<<"消息来自新连接,标号为"<<i<<"。"<<unknownconnection[i]->bytesAvailable();
            int roomid;
            QDataStream in(unknownconnection[i]);
            in.setVersion(QDataStream::Qt_5_9);
            in >> blocksize;
            in >> recvdata;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(recvdata.toLocal8Bit().data());
            QJsonObject subrespond = jsonDoc.object();
            qDebug()<<subrespond;
            if(subrespond.contains("roomID")) //从消息中获取房间ID
            {
                roomid = subrespond.value("roomID").toVariant().toInt();
            }
            qDebug()<<"获取的房间ID为"<<roomid;
            if(roomid != -1)  //成功获取
            {
                if(roomsocketmap.contains(roomid)==false) //socketmap中该ID未有socket
                {
                    roomsocketmap.insert(roomid,unknownconnection[i]);  //将此socket从未知房间号list移至已知房间号list
                    unknownconnection.removeAt(i);
                }
            }
            else//未成功获取ID，直接断开链接
                {
                    unknownconnection[i]->disconnectFromHost();
                    unknownconnection.removeAt(i);
                }
            this->parseData(subrespond);  //处理该报文
        }
    }
}


void Widget::processNewConnection()
{
    qDebug()<<"接收到连接建立请求";
    unknownconnection.push_back(mainserver->nextPendingConnection());
    connect(unknownconnection.last(),&QTcpSocket::readyRead,this,&Widget::receiveData);
    connect(unknownconnection.last(),&QTcpSocket::disconnected,unknownconnection.last(),&QTcpSocket::deleteLater);
}
