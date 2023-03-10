#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    // 初始化变量
    isMove=false;
    isLoad=false;
    isLoadShow=false;
    showWidget=new ShowWidget;
    QString style1="QScrollBar:vertical{border: none;width:8px;margin: 0 0 0 0;border-radius: 4px;}QScrollBar::handle:vertical{background: rgb(150, 150, 150);border-radius: 4px;}QScrollBar::add-page,QScrollBar::sub-page{background: transparent;}QScrollBar::add-line,QScrollBar::sub-line{background: transparent;}";
    QString style2="QScrollBar:horizontal{border: none;height:8px;margin: 0 0 0 0;border-radius: 4px;}QScrollBar::handle:horizontal{background: rgb(150, 150, 150);border-radius: 4px;}QScrollBar::add-page,QScrollBar::sub-page{background: transparent;}QScrollBar::add-line,QScrollBar::sub-line{background: transparent;}";
    ui->scrollArea->setWidget(showWidget);
    ui->scrollArea->verticalScrollBar()->setStyleSheet(style1);
    ui->scrollArea->horizontalScrollBar()->setStyleSheet(style2);
    connect(showWidget,&ShowWidget::positionChanged,this,[=](QPoint oldP,QPoint newP){
        int xChanged=newP.x()-oldP.x(),yChanged=newP.y()-oldP.y();
        ui->scrollArea->horizontalScrollBar()->setValue(ui->scrollArea->horizontalScrollBar()->value()-xChanged);
        ui->scrollArea->verticalScrollBar()->setValue(ui->scrollArea->verticalScrollBar()->value()-yChanged);
    });
    // 开启子线程
    this->tools=new ImageProcess;
    this->sub = new QThread;// 创建子线程
    this->tools->moveToThread(sub);
    this->sub->start();
    // 样式设置
    this->setWindowFlags(Qt::FramelessWindowHint);// 去掉标题栏
    this->setAttribute(Qt::WA_TranslucentBackground);// 设置窗体透明
    // 连接信号槽
    connect(ui->minButton,&QToolButton::clicked,this,&Widget::min);
    connect(ui->maxButton,&QToolButton::clicked,this,&Widget::max);
    connect(ui->closeButton,&QToolButton::clicked,this,&Widget::close);
    connect(ui->loadButton,&QToolButton::clicked,this,&Widget::loadImage);
    connect(ui->saveButton,&QToolButton::clicked,this,&Widget::saveImage);
    connect(ui->returnButton,&QToolButton::clicked,this,[=](){
        if(!isLoad)QMessageBox::critical(this,"错误","未导入任何图像");
        else{
            this->targetImage.copy(&(this->sourceImage));
            this->grawWindow();
        }
    });
    connect(ui->applyButton_2,&QToolButton::clicked,this,&Widget::grawWindow);
    connect(ui->reverseButton,&QToolButton::clicked,this,&Widget::grayReverse);
    connect(ui->reverseButton_2,&QToolButton::clicked,this,&Widget::reverse);
    connect(ui->reverseButton_3,&QToolButton::clicked,this,&Widget::grayShowReverse);
    connect(ui->scaleButton,&QToolButton::clicked,this,&Widget::scale);
    connect(ui->applyButton,&QToolButton::clicked,this,&Widget::enhance);
    // 图像处理
    connect(this,&Widget::startGrayWindow,tools,&ImageProcess::grayWindow);
    connect(this,&Widget::startGrayReverse,tools,&ImageProcess::grayReverse);
    connect(this,&Widget::startReverse,tools,&ImageProcess::reverse);
    connect(this,&Widget::startGrayShowReverse,tools,&ImageProcess::grayShowReverse);
    connect(this,&Widget::startScale,tools,&ImageProcess::scale);
    connect(this,&Widget::startEnhance,tools,&ImageProcess::enhance);
    // 处理完成
    connect(tools,&ImageProcess::endGrayWindow,this,&Widget::draw);
    connect(tools,&ImageProcess::endGrayReverse,this,&Widget::grawWindow);
    connect(tools,&ImageProcess::endReverse,this,&Widget::draw);
    connect(tools,&ImageProcess::endGrayShowReverse,this,&Widget::draw);
    connect(tools,&ImageProcess::endScale,this,&Widget::draw);
    connect(tools,&ImageProcess::endEnhance,this,[=](){
        QMessageBox::information(this,"提示","增强完成");
        this->grawWindow();
    });
}

Widget::~Widget()
{
    delete ui;
    delete showWidget;
    delete tools;
    delete sub;
}
void Widget::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton){
        if(event->pos().x()>0&&event->pos().x()<this->width()&&event->pos().y()>0&&event->pos().y()<ui->frame_2->height()){// 如果鼠标按下的位置在标题栏上
            position=event->globalPos();//获取鼠标绝对位置
            isMove=true;
        }

    }
}
void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if(isMove){//当鼠标按下时才能拖拽
        QFlags<Qt::WindowState>::Int state=this->windowState();
        if(state==Qt::WindowMaximized){// 最大化状态
            this->setWindowState(Qt::WindowNoState);// 改变状态
            ui->maxButton->setIcon(QIcon(":/resource/max1.png"));// 改变图标样式
        }
        QPoint p=event->globalPos();//获取临时位置
        //调整窗口位置
        this->move(this->pos().x()+p.x()-position.x(),this->pos().y()+p.y()-position.y());
        position=p;//更新位置
    }
}
void Widget::mouseReleaseEvent(QMouseEvent *event)
{
    isMove=false;
}
// 工具函数
void Widget::min()
{
    this->setWindowState(Qt::WindowMinimized);
    ui->maxButton->setIcon(QIcon(":/resource/max1.png"));
}
void Widget::max()
{
    QFlags<Qt::WindowState>::Int state=this->windowState();
    if(state==Qt::WindowMaximized){// 最大化状态
        this->setWindowState(Qt::WindowNoState);// 改变状态
        ui->maxButton->setIcon(QIcon(":/resource/max1.png"));// 改变图标样式
    }else if(state==Qt::WindowNoState){
        this->setWindowState(Qt::WindowMaximized);
        ui->maxButton->setIcon(QIcon(":/resource/max2.png"));
    }
}
void Widget::draw(){
    isLoadShow=true;
    this->showWidget->draw(this->showImage);
}
void Widget::loadImage(){
    // 点击后打开文件对话框
    QString fileName = QFileDialog::getOpenFileName(this,tr("选择图像"),"",tr("自定义图像(*raw);;标准图像(*bmp)"));// 只能选择一些指定的文件类型
    if(fileName!=""){// 只有当输入的文件路径不为空时，才进行下一步
        QString type=fileName.mid(fileName.size()-3,3);
        if(type=="bmp"){
            // 将导入的图像存储下来，并绘制在原图像窗口
            this->showImage.load(fileName);// 加载图像
            QVector<QRgb> grayTable;
            for(int i=0;i<256;i++)grayTable.push_back(qRgb(i,i,i));
            showImage.setColorTable(grayTable);
            ui->label_8->setText("已导入   宽*高："+QString::number(this->showImage.width())+"*"+QString::number(this->showImage.height()));
            ui->spinBox->setDisabled(true);
            ui->spinBox_2->setDisabled(true);
            ui->doubleSpinBox->setDisabled(true);
            ui->doubleSpinBox_2->setDisabled(true);
            ui->applyButton->setDisabled(true);
            ui->applyButton_2->setDisabled(true);
            ui->reverseButton->setDisabled(true);
            isLoadShow=true;
            this->draw();
        }else if(type=="raw"){
            // 将导入的图像存储下来，并绘制在原图像窗口
            this->sourceImage.load(fileName);// 加载图像
            this->targetImage.copy(&(this->sourceImage));
            ui->label_8->setText("已导入   宽*高："+QString::number(this->sourceImage.width())+"*"+QString::number(this->sourceImage.height()));
            this->isLoad=true;
        }
    }
}
void Widget::saveImage(){
    if(!this->isLoad||!this->isLoadShow)QMessageBox::critical(this,"错误","未导入任何图像");
    else {// 导出图像
        QString fileName=QFileDialog::getSaveFileName(this,tr("保存图像"),"",tr("自定义图像(*raw);;标准图像(*bmp)"));
        if(fileName!=""){
            QString type=fileName.mid(fileName.size()-3,3);
            if(type=="png"||type=="bmp"){
                this->showImage.save(fileName);
            }else if(type=="raw"){
                this->targetImage.save(fileName);
            }
        }
    }
}
void Widget::grawWindow(){
    if(!this->isLoad)QMessageBox::critical(this,"错误","未导入任何图像");
    else {
        const unsigned short maxPixelV=4095;
        int w=ui->spinBox->value();
        int p=ui->spinBox_2->value();
        // 计算开始位置和结束位置
        int start=p-(w-1)/2;
        int end=p+(w-1)/2+(w%2==0?1:0);
        qRegisterMetaType<MyImage>("MyImage");
        if(start>=0&&end<=maxPixelV)this->startGrayWindow(start,end,&(this->targetImage),&(this->showImage));
        else QMessageBox::critical(this,"错误","灰度窗设置错误");
    }
}
void Widget::grayReverse(){
    if(!this->isLoad)QMessageBox::critical(this,"错误","未导入任何图像");
    else {
        qRegisterMetaType<MyImage>("MyImage");
        this->startGrayReverse(&(this->targetImage));
    }
}
void Widget::reverse(){
    if(!this->isLoadShow)QMessageBox::critical(this,"错误","未产生展示图像");
    else {
        this->startReverse(&(this->showImage));
    }
}
void Widget::grayShowReverse(){
    if(!this->isLoadShow)QMessageBox::critical(this,"错误","未产生展示图像");
    else {
        this->startGrayShowReverse(&(this->showImage));
    }
}
void Widget::scale(){
    if(!this->isLoadShow)QMessageBox::critical(this,"错误","未产生展示图像");
    else {
        if(ui->doubleSpinBox_3->value()>0){
            this->startScale(ui->doubleSpinBox_3->value(),&(this->showImage));
        }
    }
}
void Widget::enhance(){
    if(!this->isLoad)QMessageBox::critical(this,"错误","未导入任何图像");
    else {
        this->startEnhance(ui->doubleSpinBox->value(),ui->doubleSpinBox_2->value(),&(this->targetImage));
    }
}
void Widget::showEvent(QShowEvent *e){
    QFile file;
    file.open(stdout, QFile::WriteOnly);
    QString message="show "+QString::number(this->width())+" "+QString::number(this->height());
    file.write(message.toUtf8());
    file.close();
}
void Widget::closeEvent(QCloseEvent *e){
    QFile file;
    file.open(stdout, QFile::WriteOnly);
    file.write(QString("close").toUtf8());
    file.close();
}

