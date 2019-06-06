#ifndef ALLSTRUCT_H
#define ALLSTRUCT_H


#include <QDateTime>
#include <QVector>
#include <QObject>


class SingleSubMacInfo;  //房间信息
class SingleReportData;
class SingleAdjustData;

class SingleSubMacInfo{
public:
    double currentT;  //当前温度
    double targetT;    //设置温度
    int speed;      //当前风速
    double useF;    //使用费用
    int schedulenum;    //调度次数
    QString currentModel; //当前模式
    //retrore adjust start info
    QDateTime adjuststarttime;
    QDateTime freecounttime;

    SingleSubMacInfo(){  //初始化
        currentT=25;
        targetT=25;
        speed=0;
        useF=0;
        schedulenum = 0;
        currentModel = "cold";
    }

    SingleSubMacInfo(int ct,int tt){  //设置
        currentT=ct;
        targetT=ct;
        speed=0;
        useF=0;
        schedulenum = 0;
        if(tt == 0)
            currentModel = "cold";
        else
            currentModel = "hot";
        adjuststarttime = QDateTime::currentDateTime();
        freecounttime = QDateTime::currentDateTime();
    }

    double count_free(double freemodel)   //更新费用
    {
        //数据库操作，将当前时间段的费用写入
        QDateTime  now = QDateTime::currentDateTime();
        //与计费时间计算时间差
        int counttime = freecounttime.secsTo(now);
        if(speed == 1)//当前为低风
        {
            useF += (counttime/3)*freemodel;
        }
        else if(speed == 2)
        {
            useF += (counttime/2)*freemodel;
        }
        else if(speed == 3)
        {
            useF += counttime*freemodel;
        }
        return useF;
    }
};

class SingleReportData{
public:
    SingleReportData(){}
    SingleReportData(int val){
        roomid=val;
    }

    int roomid;
    QVector<QDateTime> openclosedata;
    QVector<SingleAdjustData> adjustdata;
};

class SingleAdjustData{
public:
    SingleAdjustData(){}
    SingleAdjustData(bool modeval,int speedval,int starttempval,int stoptempval,QDateTime starttimeval,QDateTime stoptimeval,double feeval){
        mode=modeval;
        speed=speedval;
        starttemp=starttempval;
        stoptemp=stoptempval;
        starttime=starttimeval;
        stoptime=stoptimeval;
        fee=feeval;
        shown=false;
    }

    bool mode;
    int speed;
    int starttemp;
    int stoptemp;
    QDateTime starttime;
    QDateTime stoptime;
    double fee;
    //used in display report table
    bool shown;
};
class QueueObject{
public:
    int roomid;
    int windspeed;
    time_t starttime;
    float temp;

    QueueObject(){}
    QueueObject(int roomid,int windspeed,time_t starttime,float temp){
        this->roomid = roomid;
        this->windspeed = windspeed;
        this->starttime = starttime;
        this->temp = temp;
    }
};
class WaitQueueObject:public QueueObject{
public:
    QString reason;
    WaitQueueObject(){}
    WaitQueueObject(int roomid,int windspeed,time_t starttime,float temp,QString reason):QueueObject(roomid,windspeed,starttime,temp),
        reason(reason){}
    void changereason(QString reason){
        this->reason = reason;
    }
    WaitQueueObject(const WaitQueueObject& C)
       {
        this->reason = C.reason;
        this->roomid = C.roomid;
        this->starttime = C.starttime;
        this->roomid = C.roomid;
        this->windspeed = C.windspeed;

       }
};
class ServeQueueObject:public QueueObject{
public:
    ServeQueueObject(){}
    ServeQueueObject(int roomid,int windspeed,time_t starttime,float temp):QueueObject(roomid,windspeed,starttime,temp){}
     ServeQueueObject(const  ServeQueueObject& C)
       {
        this->roomid = C.roomid;
        this->starttime = C.starttime;
        this->roomid = C.roomid;
        this->windspeed = C.windspeed;

       }
};

#endif // ALLSTRUCT_H

