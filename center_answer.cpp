#include "widget.h"
#include "ui_widget.h"


void  Widget::subMachineLogIn(int roomid)//从控机登录请求响应
{
    QJsonObject JSON;
    if(1) //在数据库中未找到该房间
    {
        JSON.insert("Action","Login_S");
        JSON.insert("roomID",roomid);
        //数据库修改登录状态
    }
    else //该房间已登陆过
    {
        JSON.insert("Action","Login_F");
        JSON.insert("roomID",roomid);
    }
    qDebug()<<"回复登录请求的报文"<<JSON;
    this->sendData(JSON);
}
void  Widget::subMachineLogOut(int roomid)//从控机退房请求响应
{
    qDebug()<<"进入退房请求响应函数";
    QMap<int,SingleSubMacInfo>::iterator ptr;
    double totalfree;
    if((ptr=submacinfo.find(roomid))!=submacinfo.end())//在房间列表中找到该房间
    {
        if(submacinfo.value(roomid).speed != No_wind_speed)
        {
            totalfree = submacinfo.value(roomid).useF;
            submacinfo.remove(roomid);  //从房间列表和所有队列中移除
        }
        else
        {
            submacinfo.remove(roomid);  //从房间列表和所有队列中移除
        }
        //waitinglist.removeAll(roomid);
        //serveinglist.removeAll(roomid);

        //修改数据库登录状态，计费累加
    }
    qDebug()<<"房间队列长度"<<submacinfo.size();
    //从数据库中读取该房间累计总费用发送，然后费用清零
    QJsonObject JSON;
    JSON.insert("Action","Checkout_S");
    JSON.insert("roomID",roomid);
    JSON.insert("money",totalfree);
    qDebug()<<"回复退房请求的报文"<<JSON;
    this->sendData(JSON);

    QMap<int,QTcpSocket*>::iterator i;
    if((i=roomsocketmap.find(roomid)) !=roomsocketmap.end()) //在Socket列表中找到该房间对应的Socket
    {
        i.value()->disconnectFromHost(); //断开其连接
        roomsocketmap.remove(roomid);
    }
    qDebug()<<"roomsocketmap的长度"<<roomsocketmap.size();

}


void  Widget::subMachineStart(int roomid ) //从控机启动请求响应
{
    QMap<int,SingleSubMacInfo>::iterator ptr;
    if((ptr=submacinfo.find(roomid))==submacinfo.end()){  //房间列表中未找到该房间
       submacinfo.insert(roomid,SingleSubMacInfo(temperature_Init,work_Model));  //该房间号对应从控机启动，设置为初始化
        //数据库修改启动状态
    }
    QJsonObject JSON;
    JSON.insert("Action","Turnon_S");
    JSON.insert("roomID",roomid);
    JSON.insert("windspeed",1);
    if(work_Model == 0)
        JSON.insert("mode","cold");
    else
        JSON.insert("mode","hot");
    JSON.insert("starttemp",temperature_Init);
    qDebug()<<"工作中从控机列表长度"<<submacinfo.size();
    qDebug()<<"回复开机请求的报文"<<JSON;
    this->sendData(JSON);
    qDebug()<<submacinfo.value(roomid).freecounttime<<"   "
            <<submacinfo.value(roomid).adjuststarttime<<"   "
              <<submacinfo.value(roomid).currentModel<<"   "
                <<submacinfo.value(roomid).currentT<<"   "
                  <<submacinfo.value(roomid).speed<<"   "
                    <<submacinfo.value(roomid).targetT<<"   "
                      <<submacinfo.value(roomid).useF<<"   "
                        <<roomid;
}

void  Widget::subMachineStop(int roomid)  //从控机停止请求响应
{
    //未停风则先停风
    if(submacinfo.value(roomid).speed != No_wind_speed)
    {
        QMap<int,SingleSubMacInfo>::iterator ptr;
        if((ptr=submacinfo.find(roomid))!=submacinfo.end())
        {  //找到该房间
            ptr.value().count_free(free_Model);//计算费用并累加
            ptr.value().speed=No_wind_speed;  //修改清单中对应值
            submacinfo.remove(roomid);  //从房间列表和所有队列中移除
            //数据库修改
        }

    }
    else
    {
        QMap<int,SingleSubMacInfo>::iterator ptr;
        if((ptr=submacinfo.find(roomid))!=submacinfo.end()){  //找到该房间
            submacinfo.remove(roomid);  //从房间列表和所有队列中移除
        }
    }
    QJsonObject JSON;
    JSON.insert("Action","Turnoff_S");
    JSON.insert("roomID",roomid);
    this->sendData(JSON);
}

void  Widget::subMachineRequestWindWaitqueue(int roomid,int windspeed) //从控机更改风速请求响应
{

    QJsonObject JSON;
    JSON.insert("Action","Changewind_S");
    JSON.insert("roomID",roomid);
    JSON.insert("requiredwindspeed",windspeed);
    this->sendData(JSON);
    qDebug()<<submacinfo.value(roomid).freecounttime<<"   "
            <<submacinfo.value(roomid).adjuststarttime<<"   "
              <<submacinfo.value(roomid).currentModel<<"   "
                <<submacinfo.value(roomid).currentT<<"   "
                  <<submacinfo.value(roomid).speed<<"   "
                    <<submacinfo.value(roomid).targetT<<"   "
                      <<submacinfo.value(roomid).useF<<"   "
                            <<roomid;
}

void  Widget::subMachineRequestWindServequeue(int roomid,int windspeed) //从控机更改风速请求响应
{
    QMap<int,SingleSubMacInfo>::iterator ptr;
    if((ptr=submacinfo.find(roomid))!=submacinfo.end()){  //找到该房间
        ptr.value().count_free(free_Model); //计算费用并累加
        ptr.value().freecounttime = QDateTime::currentDateTime(); //更新计费时间段
        ptr.value().speed=windspeed;  //修改清单中对应值
    }
    QJsonObject JSON;
    JSON.insert("Action","Changewind_S");
    JSON.insert("roomID",roomid);
    JSON.insert("requiredwindspeed",windspeed);
    this->sendData(JSON);
    qDebug()<<submacinfo.value(roomid).freecounttime<<"   "
            <<submacinfo.value(roomid).adjuststarttime<<"   "
              <<submacinfo.value(roomid).currentModel<<"   "
                <<submacinfo.value(roomid).currentT<<"   "
                  <<submacinfo.value(roomid).speed<<"   "
                    <<submacinfo.value(roomid).targetT<<"   "
                      <<submacinfo.value(roomid).useF<<"   "
                            <<roomid;
}

void  Widget::subMachineRequestTem(int roomid,double targettemp) //从控机更改温度请求响应
{
    QMap<int,SingleSubMacInfo>::iterator ptr;
    if((ptr=submacinfo.find(roomid))!=submacinfo.end()){  //找到该房间
        ptr.value().targetT=targettemp;  //修改清单中对应值
        //数据库修改
    }
    QJsonObject JSON;
    JSON.insert("Action","Changetemp_S");
    JSON.insert("roomID",roomid);
    JSON.insert("settem",targettemp);
    this->sendData(JSON);
    qDebug()<<submacinfo.value(roomid).freecounttime<<"   "
            <<submacinfo.value(roomid).adjuststarttime<<"   "
              <<submacinfo.value(roomid).currentModel<<"   "
                <<submacinfo.value(roomid).currentT<<"   "
                  <<submacinfo.value(roomid).speed<<"   "
                    <<submacinfo.value(roomid).targetT<<"   "
                      <<submacinfo.value(roomid).useF<<"   "
                        <<roomid;
}

void  Widget::subMachineRequestModel(int roomid,QString model)//从控机更改模式请求响应
{
    qDebug()<<"进入更改模式响应函数,请求model"<<model;
    QMap<int,SingleSubMacInfo>::iterator ptr;
    if((ptr=submacinfo.find(roomid))!=submacinfo.end()){  //找到该房间
        ptr.value().currentModel=model;  //修改清单中对应值
            qDebug()<<"更改列表后的model"<<ptr.key()<<ptr.value().currentModel;
        //数据库修改
    }
    QJsonObject JSON;
    JSON.insert("Action","Changemode_S");
    JSON.insert("roomID",roomid);
    this->sendData(JSON);
    qDebug()<<submacinfo.value(roomid).freecounttime<<"   "
            <<submacinfo.value(roomid).adjuststarttime<<"   "
              <<submacinfo.value(roomid).currentModel<<"   "
                <<submacinfo.value(roomid).currentT<<"   "
                  <<submacinfo.value(roomid).speed<<"   "
                    <<submacinfo.value(roomid).targetT<<"   "
                      <<submacinfo.value(roomid).useF<<"   "
                        <<roomid;
}

void  Widget::subMachineStartWind(int roomid,int windspeed) //从控机送风请求响应
{
    QMap<int,SingleSubMacInfo>::iterator ptr;
    if((ptr=submacinfo.find(roomid))!=submacinfo.end()){  //找到该房间
        ptr.value().freecounttime = QDateTime::currentDateTime();//计费时间更新
        ptr.value().speed=windspeed;  //修改清单中对应值
        //数据库修改
    }
    QJsonObject JSON;
    JSON.insert("Action","Startwind_S");
    JSON.insert("roomID",roomid);
    this->sendData(JSON);
    qDebug()<<submacinfo.value(roomid).freecounttime<<"   "
            <<submacinfo.value(roomid).adjuststarttime<<"   "
              <<submacinfo.value(roomid).currentModel<<"   "
                <<submacinfo.value(roomid).currentT<<"   "
                  <<submacinfo.value(roomid).speed<<"   "
                    <<submacinfo.value(roomid).targetT<<"   "
                      <<submacinfo.value(roomid).useF<<"   "
                        <<roomid;
}

void  Widget::subMachineStopWind(int roomid) //从控机停风请求响应
{
    QMap<int,SingleSubMacInfo>::iterator ptr;
    if((ptr=submacinfo.find(roomid))!=submacinfo.end()){  //找到该房间
        double countfree = ptr.value().count_free(free_Model);//计算费用并累加
        ptr.value().speed=No_wind_speed;  //修改清单中对应值
        //数据库修改
    }
    QJsonObject JSON;
    JSON.insert("Action","Stopwind_S");
    JSON.insert("roomID",roomid);
    this->sendData(JSON);
    qDebug()<<submacinfo.value(roomid).freecounttime<<"   "
            <<submacinfo.value(roomid).adjuststarttime<<"   "
              <<submacinfo.value(roomid).currentModel<<"   "
                <<submacinfo.value(roomid).currentT<<"   "
                  <<submacinfo.value(roomid).speed<<"   "
                    <<submacinfo.value(roomid).targetT<<"   "
                      <<submacinfo.value(roomid).useF<<"   "
                        <<roomid;
}

void  Widget::updateRoomTemp(int roomid,double roomtemp) //从控机当前状态更新报文响应
{
    QMap<int,SingleSubMacInfo>::iterator ptr;
    if((ptr=submacinfo.find(roomid))!=submacinfo.end()){  //找到该房间
        ptr.value().currentT=roomtemp;  //修改清单中对应值
        //数据库修改
    }
    QJsonObject JSON;
    JSON.insert("Action","Sendtemp_S");
    JSON.insert("roomID",roomid);
    this->sendData(JSON);
    qDebug()<<submacinfo.value(roomid).freecounttime<<"   "
            <<submacinfo.value(roomid).adjuststarttime<<"   "
              <<submacinfo.value(roomid).currentModel<<"   "
                <<submacinfo.value(roomid).currentT<<"   "
                  <<submacinfo.value(roomid).speed<<"   "
                    <<submacinfo.value(roomid).targetT<<"   "
                      <<submacinfo.value(roomid).useF<<"   "
                        <<roomid;
}

void Widget::subMachineRemoveServeQueue(int roomid) //通知从控机已被移出服务队列
{
    QMap<int,SingleSubMacInfo>::iterator ptr;
    if((ptr=submacinfo.find(roomid))!=submacinfo.end()){  //找到该房间
        double countfree = ptr.value().count_free(free_Model);//计算费用并累加
        ptr.value().speed=No_wind_speed;  //修改清单中对应值
        //数据库修改
    }
    QJsonObject JSON;
    JSON.insert("Action","Stopwind_S");
    JSON.insert("roomID",roomid);
    this->sendData(JSON);
}

void Widget::subMachineEnterServeQueue(int roomid) //通知从控机已进入服务队列
{
    QMap<int,SingleSubMacInfo>::iterator ptr;
    if((ptr=submacinfo.find(roomid))!=submacinfo.end()){  //找到该房间
        ptr.value().freecounttime = QDateTime::currentDateTime();//计费时间更新
        ptr.value().speed=No_wind_speed;  //修改清单中对应值
        //数据库修改
    }
    QJsonObject JSON;
    JSON.insert("Action","Startwind_S");
    JSON.insert("roomID",roomid);
    this->sendData(JSON);
}


void Widget::sendData(QJsonObject info)//发送数据
{
    int roomid = info.value("roomID").toVariant().toInt();

    QJsonDocument rectJsonDoc;
    rectJsonDoc.setObject(info);
    QString request(rectJsonDoc.toJson(QJsonDocument::Compact));


    QByteArray block;   //用于暂存要发送的数据
    QDataStream out(&block, QIODevice::WriteOnly);

    out.setVersion(QDataStream::Qt_5_6);    //设置数据流的版本，客户端和服务器端使用的版本要相同
    out << (quint16)0;  //预留两字节
    out << request;

    out.device()->seek(0);  //  跳转到数据块的开头
    out << (quint16)(block.size() - sizeof(quint16));   //填写大小信息

    qDebug()<<"sendData函数执行";
    qDebug()<<info;
    if(roomsocketmap.contains(roomid)){
        roomsocketmap[roomid]->write(block);
        roomsocketmap[roomid]->flush();
    }
}

