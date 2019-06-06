#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <allstruct.h>
#include <QInputDialog>
#include <QMessageBox>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QMap>
#include <QList>
#include <QTimer>
#include <QPair>
#include <QMultiMap>
#include <QTimer>
#include<QJsonObject>
#include<QJsonDocument>


#define center_Air_C_ON  0
#define center_Air_C_OFF  1

#define center_Air_C_Freeze  0
#define center_Air_C_Heat 1
#define center_Air_C_Unset 99

#define High_wind_speed 3
#define Mid_wind_speed 2
#define Low_wind_speed 1
#define No_wind_speed 0
#define queuelen 2
using namespace std;
class QTcpServer;

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

    void receiveData(); //接收消息并调用处理函数
    void waitqueuecheck();

private:
    Ui::Widget *ui;
    int work_Model; //中央空调工作模式：制冷（0）/制热（1）
    int work_State; //中央空调工作状态：开启（0）/关闭（1）/等待（2）
    double temperature_Init;  //设置的缺省工作温度
    double free_Model; //设置的计费费率


    int stand;//计费标准
    QMap<int,WaitQueueObject> waitinglist;
    //QList<tuple<QString,QJsonObject,time_t>> waitinglist;//等待队列，存进入等待队列的原因，协议，等待服务时间
    QMap<int,ServeQueueObject> serveinglist;
    //QList<tuple<QJsonObject,time_t>> serveinglist;//服务队列，协议，服务开始时间
    QMap<int,SingleSubMacInfo> submacinfo;   //房间信息列表

    void turn_Off_center_Air_C(); //将中央空调改为关闭状态
    bool init_set_illegal_judge(); //开机初始设定的合法性判断

    /***************************************网络控制**/
    QTcpServer *mainserver;  //建立服务端
    QTcpSocket *clientConnection;
    quint16 blocksize;
    QMap<int,QTcpSocket*> roomsocketmap;  //已知房间号的对应的SOCKET链接
    QList<QTcpSocket*> unknownconnection;//未知房间号的SOCKET链接

    void initTCP();//初始化TCP设置
    void newConnection();//新建TCP链接
    void endConnection();//关闭TCP链接
    void processNewConnection(); //建立连接并接受消息
    void parseData(QJsonObject subrespond);//消息处理函数
    int getRoomID(QString &recvdata);//从消息中提取房间号

    int findservespeed(int askspeed);
    int findwaitspeed(int askspeed);
    int findtime(QString str);


    /****************************************数据控制**/


    /****************************************响应处理**/
    void sendData(QJsonObject info); //发送消息
    void subMachineLogIn(int roomid); //从控机登录请求响应
    void subMachineLogOut(int roomid); //从控机退房请求响应
    void subMachineStart(int roomid); //从控机启动请求响应
    void subMachineStop(int roomid);  //从控机停止请求响应
    void subMachineRequestWindWaitqueue(int roomid, int windspeed);
    void subMachineRequestWindServequeue(int roomid, int windspeed);//从控机更改风速请求响应
    void subMachineRequestTem(int roomid, double targettemp); //从控机更改温度请求响应
    void subMachineRequestModel(int roomid, QString model); //从控机更改模式请求响应
    void subMachineStartWind(int roomid, int windspeed); //从控机送风请求响应
    void subMachineStopWind(int roomid); //从控机停风请求响应
    void updateRoomTemp(int roomid, double roomtemp); //从控机当前状态更新报文响应
    void subMachineRemoveServeQueue(int roomid);//通知从控机已被移出服务队列
    void subMachineEnterServeQueue(int roomid); //通知从控机已进入服务队列

};

#endif // WIDGET_H
