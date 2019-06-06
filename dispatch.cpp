#include "widget.h"

void Widget::parseData(QJsonObject subrespond)
{  //数据处理，调用对应的响应函数
    int windspeed;
    double roomtemp;
    double targettemp;
    QString targetmodel;
    QString action = subrespond.value("Action").toVariant().toString();
    int roomid = subrespond.value("roomID").toVariant().toInt();
//    QString action = subrespond["Action"].toString();
//    int roomid = subrespond["roomID"].toInt();

    qDebug()<<"进入parseData函数"<<"roomID"<<roomid<<"Action"<<action;

    if(action.compare("Login") == 0)
    {
        qDebug()<<"接收到一个登陆请求";
        this->subMachineLogIn(roomid);
    }
    else if(action.compare("Turnon") == 0)
    {
        qDebug()<<"接收到一个开机请求";
        this->subMachineStart(roomid);
    }
    else if(action.compare("Turnoff") == 0)
    {
        qDebug()<<"接收到一个关机请求";
        this->subMachineStop(roomid);
    }
    else if(action.compare("Changemode") == 0)
    {
        qDebug()<<"接收到一个更改模式请求";
        targetmodel = subrespond.value("mode").toVariant().toString();
        this->subMachineRequestModel(roomid,targetmodel);
    }
    else if(action.compare("Changetemp") == 0)
    {
        qDebug()<<"接收到一个更改温度请求";
        targettemp = subrespond.value("requiredtemp").toVariant().toDouble();
        this->subMachineRequestTem(roomid,targettemp);
    }
    else if(action.compare("Sendtemp") == 0)
    {
        qDebug()<<"接收到一个定期更新包";
        roomtemp = subrespond.value("currenttemp").toVariant().toDouble();
        //windspeed = subrespond.value("currentwindspeed").toVariant().toInt();
        this->updateRoomTemp(roomid,roomtemp);
    }
    else if(action.compare("Checkout") == 0)
    {
        qDebug()<<"接收到一个退房请求";
        this->subMachineLogOut(roomid);
    }
    else if(action.compare("Changewind") == 0)
    {
        qDebug()<<"接收到一个更改风速请求";
        windspeed = subrespond.value("requiredwindspeed").toVariant().toInt();
        if(serveinglist.contains(roomid)){//如果该房间在服务队列中服务
            tuple<QJsonObject,time_t>fromserve = serveinglist[roomid];
//            serveinglist.remove(roomid);
            QJsonObject json;//拆包修改协议里的风速
            json.insert("Action",get<0>(fromserve)["Action"].toString());
            json.insert("requiredwindspeed",windspeed);
            json.insert("roomID",get<0>(fromserve)["roomID"].toInt());
            time_t servetime = time(NULL);
            tuple<QJsonObject,time_t>toserve = make_tuple(json,servetime);
            serveinglist.insert(roomid,toserve);
            this->subMachineRequestWindServequeue(roomid,windspeed);
        }
        else if (waitinglist.contains(roomid)){
            QString reason;
            int findid = this->findservespeed(windspeed);
            if(findid == -2 || findid == -1){
                if(findid == -2)reason = "small";
                else reason = "equality";
                tuple<QString,QJsonObject,time_t>fromserve = waitinglist[roomid];
//                waitinglist.remove(roomid);
                QJsonObject json;//拆包修改协议里的风速
                json.insert("Action",get<1>(fromserve)["Action"].toString());
                json.insert("requiredwindspeed",windspeed);
                json.insert("roomID",get<1>(fromserve)["roomID"].toInt());
                time_t waittime = time(NULL);
                tuple<QString,QJsonObject,time_t>towait = make_tuple(reason,json,waittime);
                waitinglist.insert(roomid,towait);
                this->subMachineRequestWindWaitqueue(roomid,windspeed);
            }
            else {
                reason = "iskicked";
                tuple<QJsonObject,time_t> tpfromserve = serveinglist[findid];
                QJsonObject protocol = get<0>(tpfromserve);
                time_t waittime = time(NULL);
                tuple<QString,QJsonObject,time_t> tptowait = make_tuple(reason,protocol,waittime);
                waitinglist.insert(protocol["roomID"].toInt(),tptowait);
                this->subMachineRemoveServeQueue(findid);
                serveinglist.remove(findid);

                time_t servetime = time(NULL);
                tuple<QJsonObject,time_t>tptoserve = make_tuple(subrespond,servetime);
                serveinglist.insert(roomid,tptoserve);
                this->subMachineStartWind(roomid,windspeed);
            }

        }
    }
    else if(action.compare("Stopwind") == 0)
    {
        qDebug()<<"接收到一个停风请求";
        QString reason = "";
        serveinglist.remove(roomid);//从服务队列里移除
        this->subMachineStopWind(roomid);
    }
    else if(action.compare("Startwind") == 0)  //调度
    {
        qDebug()<<"接收到一个送风请求";
        windspeed = subrespond.value("requiredwindspeed").toVariant().toInt();
        if(!serveinglist.contains(roomid)){
            if(serveinglist.size() < queuelen){//如果队列小于3，可以直接进入服务队列
                qDebug()<<"队列小于1，直接进入服务队列";
                time_t servetime = time(NULL);
                tuple<QJsonObject,time_t>toserve = make_tuple(subrespond,servetime);
                serveinglist.insert(roomid,toserve);
                this->subMachineStartWind(roomid,windspeed);
            }
            else{
                //如果服务队列大于3,则新请求需要判断当前等待队列中是否有该房间号的等待请求，
                //如果发送的是请求送风，（认为请求送风只可能有一个），则根据优先级调度
                //如果发送的是更改风速，如果请求送风在等待队列，直接替换，如果在服务队列,则根据优先级修改}
                qDebug()<<"队列等于1，进入调度策略";
                int findid = this->findservespeed(windspeed);
                if (findid == -2){//没有比它风速小的,进入等待队列，无限期等待
                    QString reason = "small";
                    qDebug()<<"没有比他小的";
                    time_t waittime = time(NULL);
                    tuple<QString,QJsonObject,time_t>tptowait = make_tuple(reason,subrespond,waittime);
                    waitinglist.insert(roomid,tptowait);
                }
                else if (findid == -1){//有和它风速相等的，进入等待队列，等待两分钟
                    QString reason = "equality";
                    qDebug()<<"有和他一样的";
                    time_t waittime = time(NULL);
                    if(waitinglist.contains(roomid)){
                        waittime = get<2>(waitinglist[roomid]);
                    }
                    tuple<QString,QJsonObject,time_t>tptowait = make_tuple(reason,subrespond,waittime);
                    waitinglist.insert(roomid,tptowait);
                }
                else{//有比它小的,从服务队列中取出，计费，放入等待队列，将它放入服务队列
                    QString reason = "iskicked";
                    qDebug()<<"接收到一个停风请求";
                    qDebug()<<"有比他小的";

                    tuple<QJsonObject,time_t> tpfromserve = serveinglist[findid];
                    QJsonObject protocol = get<0>(tpfromserve);
                    time_t waittime = time(NULL);
                    tuple<QString,QJsonObject,time_t> tptowait = make_tuple(reason,protocol,waittime);
                    waitinglist.insert(protocol["roomID"].toInt(),tptowait);
                    this->subMachineRemoveServeQueue(findid);
    //                serveinglist.remove(findid);

                    time_t servetime = time(NULL);
                    tuple<QJsonObject,time_t>tptoserve = make_tuple(subrespond,servetime);
                    serveinglist.insert(roomid,tptoserve);
                    serveinglist.remove(findid);
                    this->subMachineStartWind(roomid,windspeed);
                    if(waitinglist.contains(roomid)){
                        waitinglist.remove(roomid);
                    }
                    }
                }
        }

    }
}
int Widget::findservespeed(int askspeed){
    qDebug()<<"findservespeed";
    qDebug()<<askspeed<<"askspeed";
    int loserspeed = 10;
    long long losertime = 100000000;
    int findid = -2;
    int findsmall = 0;
    QMap<int,tuple<QJsonObject,time_t>>::Iterator it = serveinglist.begin();
    while(it!=serveinglist.end()){
        tuple<QJsonObject,time_t>item = it.value();
        int roomid = it.key();
        QJsonObject protocol = get<0>(item);
        time_t waittime = get<1>(item);
        SingleSubMacInfo curroom = submacinfo[roomid];
        qDebug()<< "curroom.speed"<<curroom.speed;
        qDebug()<< "askspeed"<<askspeed;
        if (curroom.speed < askspeed){ //判断如果风速小于当前的风速，则替换替换，
            if (loserspeed > curroom.speed){
                findsmall = 1;
                loserspeed = curroom.speed;
                findid = roomid;
            }
        }
        if (findsmall == 0){//如果风速相等，比较服务时间，服务时间越小则可替换
            if ((curroom.speed == askspeed)){
                if(losertime > curroom.currentT){
                    loserspeed = curroom.speed;
                    losertime = curroom.currentT;
                    findid = -1;
                }
            }
        }
        it++;
    }
    return findid;
   //返回-2说明没有比他小的，返回-1说明只有相等的，否则返回房间号
}
int Widget::findtime(QString str){ //找到等待/服务开始时间最靠前的并返回place
    long long earlytime = 100000000000;
    int roomid = -1;
    if (str == "wait"){
        QMap<int,tuple<QString,QJsonObject,time_t>>::Iterator it = waitinglist.begin();
        while(it != waitinglist.end()){
            tuple<QString,QJsonObject,time_t>tp = it.value();
            time_t starttime  = get<2>(tp);
            if (earlytime > starttime){
                earlytime = starttime;
                roomid = it.key();
            }
            it++;
         }
    }
    else {
        QMap<int,tuple<QJsonObject,time_t>>::Iterator it = serveinglist.begin();
        while(it != serveinglist.end()){
            tuple<QJsonObject,time_t>tp = it.value();
            time_t starttime  = get<1>(tp);
            if (earlytime > starttime){
                earlytime = starttime;
                roomid = it.key();
            }
            it++;
         }
   }
    return roomid;
}
void Widget::waitqueuecheck(){//放在定时器中
    /*10s触发一次
     1.判断serve队列是否小于3,小于3进行调度，调度时选取开始等待时间最前的请求
     2.判断key为"equality"的请求是否到达2分钟，到达则将开始服务时间最前的请求挤下来
    */
    qDebug()<<"定时检查队列";
    qDebug()<<"serveinglist"<<serveinglist.size();
    QMap<int,tuple<QJsonObject,time_t>>::Iterator it2 = serveinglist.begin();
    while(it2 != serveinglist.end()){
         qDebug()<<"在服务队列中的房间"<<it2.key()<<get<0>(it2.value())["Action"].toString();
        it2++;
    }
    qDebug()<<"waitinglist"<<waitinglist.size();
    QMap<int,tuple<QString,QJsonObject,time_t>>::Iterator it1 = waitinglist.begin();
    while(it1 != waitinglist.end()){
        qDebug()<<"在等待队列中的房间"<<it1.key()<<get<1>(it1.value())["Action"].toString();
        it1++;
    }
    if (serveinglist.size() < queuelen){
        qDebug()<<"计入定时器，且队列少于2，不需要调度";
        if(waitinglist.size()>0){
            int roomid = this->findtime("wait");
            qDebug()<<"roomid:"<<roomid;
            qDebug()<<"waiting队列里有";
            tuple<QString,QJsonObject,time_t>tpfromwait = waitinglist[roomid];
            QJsonObject protocol = get<1>(tpfromwait);  //协议内容
            QString reason = get<0>(tpfromwait);
            waitinglist.remove(roomid);
            time_t servetime = time(NULL);
            int windspeed = protocol.value("requiredwindspeed").toVariant().toInt();
            tuple<QJsonObject,time_t>tptoserve = make_tuple(protocol,servetime);
            serveinglist.insert(roomid,tptoserve);
            this->subMachineStartWind(roomid,windspeed);

        }

    }
    QMap<int,tuple<QString,QJsonObject,time_t>>::Iterator it = waitinglist.begin();
    QMap<int,tuple<QString,QJsonObject,time_t>>::Iterator preit;
    while(it != waitinglist.end()){
        qDebug()<<"遍历等待队列，找2分钟相等";
        time_t curtime =  time(NULL);
        tuple<QString,QJsonObject,time_t>tp = it.value();
        QString reason = get<0>(tp);
        QJsonObject protocol = get<1>(tp);  //协议内容
        time_t starttime  = get<2>(tp);
        int windspeed = protocol["requiredwindspeed"].toInt();
        int roomid = protocol["roomID"].toInt();
        qDebug()<<reason;
        qDebug()<<(curtime - starttime);
        if ((reason == "equality") && (curtime - starttime) > 10){
            qDebug()<<"进入相等";//如果找到满足条件的，将他它从waiting队列中取出，替换服务队列服务时间最长的
            int findid = this->findtime("serve");
            tuple<QJsonObject,time_t>tpfromserve = serveinglist[findid];
            time_t waittime = time(NULL);
            QString reasontowait = "iskicked";
            tuple<QString,QJsonObject,time_t>tptowait = make_tuple(reasontowait,get<0>(tpfromserve),waittime);
            waitinglist.insert(findid,tptowait);
            serveinglist.remove(findid);
            this->subMachineRemoveServeQueue(findid);

            preit = it;
            it++;
            waitinglist.erase(preit);
            time_t servetime = time(NULL);
            tuple<QJsonObject,time_t>tptoserve = make_tuple(protocol,servetime);
            serveinglist.insert(roomid,tptoserve);
            this->subMachineStartWind(roomid,windspeed);//处理新请求
            qDebug()<<"还是好的";
        }
        else{
            it++;
        }


    }
}
