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
            serveinglist[roomid].windspeed = windspeed;
            time_t servetime = time(NULL);
            serveinglist[roomid].starttime = servetime;
            this->subMachineRequestWindServequeue(roomid,windspeed);
        }
        else if (waitinglist.contains(roomid)){
            QString reason;
            int findid = this->findservespeed(windspeed);
            if(findid == -2 || findid == -1){
                if(findid == -2)reason = "small";
                else reason = "equality";
                waitinglist[roomid].reason = reason;
                waitinglist[roomid].windspeed = windspeed;
                time_t waittime = time(NULL);
                waitinglist[roomid].starttime = waittime;
                waitinglist[roomid].roomid = roomid;
                waitinglist[roomid].temp = submacinfo[roomid].targetT;
                this->subMachineRequestWindWaitqueue(roomid,windspeed);
            }
            else {
                reason = "iskicked";
                ServeQueueObject qfromserve = serveinglist[findid];
//                QJsonObject protocol = get<0>(tpfromserve);
                time_t waittime = time(NULL);
                WaitQueueObject towait(qfromserve.roomid,qfromserve.windspeed,waittime,qfromserve.temp,reason);
//                tuple<QString,QJsonObject,time_t> tptowait = make_tuple(reason,protocol,waittime);
                waitinglist.insert(towait.roomid,towait);
                this->subMachineRemoveServeQueue(findid);
                serveinglist.remove(findid);

                time_t servetime = time(NULL);
                ServeQueueObject toserve(roomid,windspeed,servetime,submacinfo[roomid].targetT);
                serveinglist.insert(roomid,toserve);
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
                ServeQueueObject toserve(roomid,windspeed,servetime,submacinfo[roomid].targetT);
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
                    WaitQueueObject towait(roomid,windspeed,waittime,submacinfo[roomid].targetT,reason);
                    waitinglist.insert(roomid,towait);
                }
                else if (findid == -1){//有和它风速相等的，进入等待队列，等待两分钟
                    QString reason = "equality";
                    qDebug()<<"有和他一样的";
                    time_t waittime = time(NULL);
                    if(waitinglist.contains(roomid)){
                        waittime = waitinglist[roomid].starttime;
                    }
                    WaitQueueObject towait(roomid,windspeed,waittime,submacinfo[roomid].targetT,reason);
                    waitinglist.insert(roomid,towait);
                }
                else{//有比它小的,从服务队列中取出，计费，放入等待队列，将它放入服务队列
                    QString reason = "iskicked";
                    qDebug()<<"接收到一个停风请求";
                    qDebug()<<"有比他小的";

                    ServeQueueObject fromserve = serveinglist[findid];
//                    QJsonObject protocol = get<0>(tpfromserve);
                    time_t waittime = time(NULL);
                    WaitQueueObject towait(fromserve.roomid,fromserve.windspeed,waittime,fromserve.temp,reason);
                    waitinglist.insert(towait.roomid,towait);
                    this->subMachineRemoveServeQueue(findid);
    //                serveinglist.remove(findid);

                    time_t servetime = time(NULL);
                    ServeQueueObject toserve(roomid,windspeed,servetime,submacinfo[roomid].targetT);
                    serveinglist.insert(roomid,toserve);
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
    QMap<int,ServeQueueObject>::Iterator it = serveinglist.begin();
    while(it!=serveinglist.end()){
        ServeQueueObject item = it.value();
        int roomid = it.key();
//        QJsonObject protocol = get<0>(item);
        time_t waittime = item.starttime;
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
        QMap<int,WaitQueueObject>::Iterator it = waitinglist.begin();
        while(it != waitinglist.end()){
            WaitQueueObject tp = it.value();
            time_t starttime  = tp.starttime;
            if (earlytime > starttime){
                earlytime = starttime;
                roomid = it.key();
            }
            it++;
         }
    }
    else {
        QMap<int,ServeQueueObject>::Iterator it = serveinglist.begin();
        while(it != serveinglist.end()){
            ServeQueueObject tp = it.value();
            time_t starttime  = tp.starttime;
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
    QMap<int,ServeQueueObject>::Iterator it2 = serveinglist.begin();
    while(it2 != serveinglist.end()){
         qDebug()<<"在服务队列中的房间"<<it2.key();
        it2++;
    }
    qDebug()<<"waitinglist"<<waitinglist.size();
    QMap<int,WaitQueueObject>::Iterator it1 = waitinglist.begin();
    while(it1 != waitinglist.end()){
        qDebug()<<"在等待队列中的房间"<<it1.key();
        it1++;
    }
    if (serveinglist.size() < queuelen){
        qDebug()<<"计入定时器，且队列少于2，不需要调度";
        if(waitinglist.size()>0){
            int roomid = this->findtime("wait");
            qDebug()<<"roomid:"<<roomid;
            qDebug()<<"waiting队列里有";
            WaitQueueObject fromwait = waitinglist[roomid];
//            QJsonObject protocol = get<1>(tpfromwait);  //协议内容
            QString reason = fromwait.reason;
            waitinglist.remove(roomid);
            time_t servetime = time(NULL);
            int windspeed = fromwait.windspeed;
            ServeQueueObject toserve(fromwait.roomid,fromwait.windspeed,fromwait.starttime,fromwait.temp);
            serveinglist.insert(roomid,toserve);
            this->subMachineStartWind(roomid,windspeed);

        }

    }
    QMap<int, WaitQueueObject>::Iterator it = waitinglist.begin();
    QMap<int,WaitQueueObject>::Iterator preit;
    while(it != waitinglist.end()){
        qDebug()<<"遍历等待队列，找2分钟相等";
        time_t curtime =  time(NULL);
        WaitQueueObject tp = it.value();
        QString reason = tp.reason;
        time_t starttime  = tp.starttime;
        int windspeed = tp.windspeed;
        int roomid = tp.roomid;
        qDebug()<<reason;
        qDebug()<<(curtime - starttime);
        if ((reason == "equality") && (curtime - starttime) > 10){
            qDebug()<<"进入相等";//如果找到满足条件的，将他它从waiting队列中取出，替换服务队列服务时间最长的
            int findid = this->findtime("serve");
            ServeQueueObject fromserve = serveinglist[findid];
            time_t waittime = time(NULL);
            QString reason = "iskicked";
            WaitQueueObject towait(fromserve.roomid,fromserve.windspeed,waittime,fromserve.temp,reason);
            waitinglist.insert(findid,towait);
            serveinglist.remove(findid);
            this->subMachineRemoveServeQueue(findid);
            preit = it;
            it++;
            waitinglist.erase(preit);
            time_t servetime = time(NULL);
            ServeQueueObject toserve(roomid,windspeed,starttime,submacinfo[roomid].targetT);
            serveinglist.insert(roomid,toserve);
            this->subMachineStartWind(roomid,windspeed);//处理新请求
            qDebug()<<"还是好的";
        }
        else{
            it++;
        }


    }
}
