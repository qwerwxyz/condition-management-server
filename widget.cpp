#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);//主界面建立

    /***********************************/
    /*主控机属性初始化*/
    this->work_State = center_Air_C_OFF; //关闭
    ui->pushButton_3->setEnabled(false); //OFF不可用
    this->work_Model = center_Air_C_Unset;//工作模式未设置
    this->temperature_Init = center_Air_C_Unset;//温度范围未设置
    this->free_Model = center_Air_C_Unset;//计费模式未设置
    //房间列表初始化

    /***********************************/
    /*初始化网络连接*/
    this->initTCP();

    /*调度策略*/
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(waitqueuecheck()));
    timer->start(2000);

    /***********************************/
    /*初始化工作日志模块*/
}

Widget::~Widget()
{
    delete ui;
}

void Widget::turn_Off_center_Air_C() //将中央空调改为关闭状态，将所有初始化设定清空
{
    //工作队列为空则关闭
    //不为空则不做处理
    this->work_Model = center_Air_C_Unset;//工作模式未设置
    this->temperature_Init = center_Air_C_Unset;//温度未设置
    this->free_Model = center_Air_C_Unset;//计费模式未设置
    ui->label_6->setText("Unset");
    ui->label_2->setText("Unset");
    ui->label_4->setText("Unset");
}

bool Widget::init_set_illegal_judge() //开机初始设定的合法性判断
{
    //合法则返回true
    //不合法返回false，并弹出弹窗提示
    if(work_Model == center_Air_C_Unset)
        return false;
    if(temperature_Init == center_Air_C_Unset )
        return false;
    if(free_Model == center_Air_C_Unset)
        return false;
    return true;
}

void Widget::on_pushButton_clicked()    //ON按钮
{
    qDebug()<<"进入开机函数";
    if(!init_set_illegal_judge())//如果设置不合法
    {
        QMessageBox::information(NULL, "Warning", "初始化设置错误！", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        return;
    }
    ui->pushButton_3->setEnabled(true); //OFF可用
    ui->pushButton->setEnabled(false);  //ON不可用
    ui->pushButton_2->setEnabled(false);//模式设置不可用
    ui->pushButton_5->setEnabled(false);//温度范围设置不可用
    ui->pushButton_6->setEnabled(false);//计费模式设置不可用
    this->work_State = center_Air_C_ON; //更改中央空调状态为打开
    this->newConnection();  //建立TCP监听
    qDebug()<<"主控机工作状态变为"<<this->work_State;
}

void Widget::on_pushButton_3_clicked()  //OFF按钮
{
    qDebug()<<"进入关机函数";
    ui->pushButton_3->setEnabled(false); //OFF不可用
    ui->pushButton->setEnabled(true);  //ON可用
    ui->pushButton_2->setEnabled(true);//模式设置可用
    ui->pushButton_5->setEnabled(true);//温度范围设置可用
    ui->pushButton_6->setEnabled(true);//计费模式设置可用
    this->work_State = center_Air_C_OFF;//更改状态为关闭
    waitinglist.clear();
    serveinglist.clear();   //清空两个队列
    //向工作队列中各房间从控机发送消息，通知其断开
    this->endConnection(); //关闭TCP监听

    /*完善工作日志*/

    this->turn_Off_center_Air_C();//执行关闭函数，清空所有初始化设定
    qDebug()<<"主控机工作状态变为"<<this->work_State;
}

void Widget::on_pushButton_2_clicked()  //模式设置按钮
{
    QPalette pa;
    if(this->work_Model == center_Air_C_Freeze)
    {
        this->work_Model = center_Air_C_Heat;
        pa.setColor(QPalette::WindowText,Qt::red);
        ui->label_6->setPalette(pa);
        ui->label_6->setText("制热");
    }
    else
    {
        this->work_Model = center_Air_C_Freeze;
        pa.setColor(QPalette::WindowText,Qt::blue);
        ui->label_6->setPalette(pa);
        ui->label_6->setText("制冷");
    }
    qDebug()<<"主控机默认模式变为"<<this->work_Model;
}

void Widget::on_pushButton_5_clicked()  //温度范围设置按钮
{
    bool ok;
    double inputTem = QInputDialog::getDouble(this,tr("请输入缺省温度"),tr("缺省温度"),25.0,18.0,30.0,1,&ok);
    if(ok)
        this->temperature_Init = inputTem; //更改设置
    ui->label_2->setText(QString::number(this->temperature_Init));
    qDebug()<<"主控机默认温度变为"<<this->temperature_Init;
}

void Widget::on_pushButton_6_clicked()  //计费模式设置按钮
{
    bool ok;
    double inputFree = QInputDialog::getDouble(this,tr("请输入费率"),tr("计费费率"),0.3,0.1,1.0,1,&ok);
    if(ok)
        this->free_Model = inputFree; //更改设置
    ui->label_4->setText(QString::number(this->free_Model));
    qDebug()<<"主控机默认计费费率变为"<<this->free_Model;
}
