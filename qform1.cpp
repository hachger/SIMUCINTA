#include "qform1.h"
#include "ui_qform1.h"

#include <QDebug>

QForm1::QForm1(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QForm1)
{
    ui->setupUi(this);

    qApp->setStyle("Fusion");

    QSerialPort1 = new QSerialPort(this);
    connect(QSerialPort1, &QSerialPort::readyRead, this, &QForm1::OnQSerialPort1);

    QSerialSetup1 = new QSerialSetup(this, QSerialPort1);

    QTimer1 = new QTimer(this);
    connect(QTimer1, &QTimer::timeout, this, &QForm1::OnQTimer1);

    QTcpServer1 = new QTcpServer(this);
    connect(QTcpServer1, &QTcpServer::newConnection, this, &QForm1::OnQTcpServer1ClientConnect);
    connect(QTcpServer1, &QTcpServer::acceptError, this, &QForm1::OnQTcpServer1Error);

    ui->tableWidget->setColumnWidth(2, 900);
    ui->tableWidget->setRowHeight(0, 100);
    ui->tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("ID"));
    ui->tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("IP:PORT"));
    ui->tableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem("CINTA"));
    ui->tableWidget->setHorizontalHeaderItem(3, new QTableWidgetItem("V CINTA"));
    ui->tableWidget->setHorizontalHeaderItem(4, new QTableWidgetItem("START/STOP"));


    QTimer1->start(10);
}

QForm1::~QForm1()
{
    if(QTcpServer1->isListening()){
        for (int i = 0; i < MyTCPClientsList.count(); ++i) {
            MyTCPClientsList.at(i)->GetClient()->close();
        }
        QTcpServer1->close();
        MyTCPClientsList.clear();
    }

    delete ui;
}

void QForm1::DecodeCMD()
{

    uint8_t length, cks, i;

    length = 0;
    switch(rx[0]){
    case 0xF0:
        tx[7] = 0x0D;
        length = 2;
        break;
    }

    if(length){
        HEADER[4] = length + 1;
        HEADER[6] = rx[0];
        cks = 0;
        length += 7;

        for (i = 0; i < length; ++i) {
            if(i < 7)
                tx[i] = HEADER[i];
            cks ^= tx[i];
        }
        tx[i] = cks;

        if(QSerialPort1->isOpen())
            QSerialPort1->write((char *)tx, length);
    }

}

void QForm1::OnQTimer1()
{

}

void QForm1::OnQSerialPort1()
{
    int count = QSerialPort1->bytesAvailable();
    if(count <= 0)
        return;

    uint8_t *buf = new uint8_t[count];
    QSerialPort1->read((char *)buf, count);
    for (int i = 0; i < count; ++i) {
        switch(header){
        case 0:
            if(HEADER[header] == buf[i]){
                header++;
                timeout = 10;
            }
            break;
        case 1:
        case 2:
        case 3:
        case 5:
            if(HEADER[header] == buf[i])
                header++;
            else{
                header = 0;
                i--;
            }
            break;
        case 4:
            nBytes = buf[i];
            cks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ nBytes ^ ':';
            index = 0;
            header = 5;
            break;
        case 6:
            nBytes--;
            if(nBytes){
                rx[index++] = buf[i];
                cks ^= buf[i];
            }
            else{
                header = 0;
                if(cks == buf[i])
                    DecodeCMD();
            }
            break;
        }
    }
}

void QForm1::OnQTcpServer1Error()
{
    QTcpServer1->close();
    ui->pushButton->setText("OPEN");
}

void QForm1::OnQTcpServer1ClientConnect()
{
    QString clientID;
    MyClient *aux;
    int row;
    bool ok;
    float f;

    // aux->client = QTcpServer1->nextPendingConnection();

    aux = new MyClient(nullptr, QTcpServer1->nextPendingConnection());

    MyTCPClientsList.append(aux);

    clientID = MyTCPClientsList.last()->GetPeerAddress().toString() + ":";
    clientID = clientID +  QString().number(MyTCPClientsList.last()->GetPeerPort(),10);

    row = ui->tableWidget->rowCount();

    ui->tableWidget->setRowCount(row+1);
    ui->tableWidget->setRowHeight(row, 100);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem("NAME"));
    ui->tableWidget->item(row, 0)->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(clientID));
    ui->tableWidget->item(row, 1)->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(""));
    ui->tableWidget->item(row, 3)->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget->setItem(row, 4, new QTableWidgetItem("START"));
    ui->tableWidget->item(row, 4)->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget->item(row, 4)->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);

    QLabel *lb = new QLabel;
    lb->setAlignment(Qt::AlignCenter);
    ui->tableWidget->setCellWidget(row, 2, lb);

    MyTCPClientsList.last()->SetClientWidget(ui->tableWidget->cellWidget(row, 2));


    connect(aux, &MyClient::MyClientUpdateWidget, this, &QForm1::OnMyClientUpdateWidget);
    MyTCPClientsList.last()->SetWidget(ui->tableWidget->columnWidth(2)-10, 80);
    f = ui->lineEdit_3->text().toFloat(&ok);
    if(!ok)
        f = 1.0;
    else{
        if(f > 3.0f)
            f = 1.0;
    }
    if(ui->pushButton_4->text()=="STOP"){
        ui->tableWidget->item(row, 3)->setText(QString().asprintf("%0.2f", f));
        MyTCPClientsList.last()->StartCinta(f);
        ui->tableWidget->item(row, 4)->setText("STOP");
    }
    else{
        MyTCPClientsList.last()->SetVCinta(f);
        ui->tableWidget->item(row, 3)->setText(QString().asprintf("%0.2f", f));
    }

//   connect(MyTCPClientsList.last()->client, &QTcpSocket::readyRead, this, &QForm1::OnQTcpClientTxData);
//    connect(MyTCPClientsList.last()->client, &QTcpSocket::disconnected, this, &QForm1::OnQTcpClientDisconnected);
    connect(aux, &MyClient::MyClientDisconnect, this, &QForm1::OnMyClientDisconnect);


}

void QForm1::OnMyClientDisconnect(QTcpSocket *aClient){
    QString clientID;

    clientID = aClient->peerAddress().toString() + ":";
    clientID = clientID +  QString().number(aClient->peerPort(),10);

    for (int i = 0; i < MyTCPClientsList.count(); ++i) {
        if(MyTCPClientsList.at(i)->GetPeerAddress()==aClient->peerAddress() &&
            MyTCPClientsList.at(i)->GetPeerPort()==aClient->peerPort()){
            MyTCPClientsList.at(i)->terminate();
            while(!MyTCPClientsList.at(i)->wait());
            delete MyTCPClientsList.takeAt(i);
            break;
        }
    }

    for(int i=0; i<ui->tableWidget->rowCount(); i++){
        if(ui->tableWidget->item(i, 1)->text() == clientID){
            ui->tableWidget->removeRow(i);
            break;
        }
    }


    ui->label->setText(QString().asprintf("CLIENTS (%04d)", (int)MyTCPClientsList.count()));


}

void QForm1::OnMyClientUpdateWidget(QWidget *aClientWidget, QPixmap *aQPixmapCinta)
{
    ((QLabel *)aClientWidget)->setPixmap(*aQPixmapCinta);
}

// void QForm1::OnQTcpClientTxData()
// {
//     // QString strhex;
//     quint8 *buf;
//     int Count, i;

//     QTcpSocket* Client = static_cast<QTcpSocket*>(QObject::sender());

//     Count = Client->bytesAvailable();
//     if(Count <= 0)
//         return;
//     buf = new quint8[Count];

//     Client->read((char *)buf, Count);
//     for (i = 0; i < MyTCPClientsList.count(); ++i) {
//         if(MyTCPClientsList.at(i)->client->peerAddress()==Client->peerAddress() &&
//             MyTCPClientsList.at(i)->client->peerPort()==Client->peerPort()){
//             break;
//         }
//     }

//     if(i < MyTCPClientsList.count()){
//         _sUNERBUSHandle *aux = &MyTCPClientsList.at(i)->unerbus;

//         UNERBUS_ReceiveBuf(aux, buf, Count);
//         UNERBUS_Task(aux);
//         if(UNERBUS_IsNewData(aux)){
//             UNERBUS_ResetNewData(aux);
//             OnDecodeCMDTCP(MyTCPClientsList.at(i));
//         }
//     }
//     //    ui->plainTextEdit->appendPlainText("Data -- "+Client->peerAddress().toString()+":"+
//     //                                       QString().number(Client->peerPort(),10)+":"+
//     //                                       QString().number(Count,10));

//     // strhex="-->  ";
//     // for(int i=0; i<Count; i++){
//     //     if(!iscntrl(buf[i]))
//     //         strhex = strhex + QString((char)buf[i]);
//     //     else
//     //         strhex = strhex + "{" + QString("%1").arg(buf[i],2,16,QChar('0')).toUpper() + "}";
//     // }
//     //    ui->plainTextEdit->appendPlainText(strhex);

//     delete [] buf;
// }

// void QForm1::OnQTcpClientDisconnected(){
//     QString clientID;

//     QTcpSocket* Client = static_cast<QTcpSocket*>(QObject::sender());

//     clientID = Client->peerAddress().toString() + ":";
//     clientID = clientID +  QString().number(Client->peerPort(),10);

//     for(int i=0; i<ui->listWidget->count(); i++){
//         if(ui->listWidget->item(i)->text() == clientID){
//             delete ui->listWidget->takeItem(i);
//             break;
//         }
//     }

//     for (int i = 0; i < MyTCPClientsList.count(); ++i) {
//         if(MyTCPClientsList.at(i)->client->peerAddress() == Client->peerAddress()){
//             delete MyTCPClientsList.takeAt(i);
//             break;
//         }
//     }

//     ui->label->setText(QString().asprintf("CLIENTS (%04d)", (int)MyTCPClientsList.count()));

//     // clientID = Client->peerAddress().toString() + ":" + QString().number(Client->peerPort(), 10);
//     //    ui->plainTextEdit->appendPlainText(clientID + " >> DISCONNECT");

// }


void QForm1::on_pushButton_clicked()
{
    if(QSerialPort1->isOpen()){
        QSerialPort1->close();
        ui->pushButton_2->setText("OPEN");
    }

    QSerialSetup1->exec();
    ui->lineEdit->setText(QSerialSetup1->_PortName);
}


void QForm1::on_pushButton_2_clicked()
{
    if(QSerialPort1->isOpen()){
        QSerialPort1->close();
        ui->pushButton_2->setText("OPEN");
    }
    else{
        if(QSerialPort1->open(QSerialPort::ReadWrite))
            ui->pushButton_2->setText("CLOSE");
    }
}


void QForm1::on_pushButton_3_clicked()
{
    quint16 serverPort;
    bool ok;

    if(QTcpServer1->isListening()){
        for (int i = 0; i < MyTCPClientsList.count(); ++i) {
            MyTCPClientsList.at(0)->GetClient()->close();
            i--;
        }
        QTcpServer1->close();
        MyTCPClientsList.clear();
        ui->pushButton_3->setText("TCP OPEN");
        return;
    }
    else{
        serverPort = ui->lineEdit_2->text().toUShort(&ok, 10);
        if(!ok){
            QMessageBox::information(this, "TCP SERVER", "Invalid PORT number.");
            return;
        }
        if(!QTcpServer1->listen(QHostAddress::Any, serverPort)){
            QMessageBox::information(this, "TCP SERVER", "Can't OPEN SERVER.");
            return;
        }
        ui->pushButton_3->setText("TCP CLOSE");
    }

}


MyClient::MyClient(QObject *parent, QTcpSocket *clientSocket):QThread(parent)
{
    client = clientSocket;
    irRead = 0;
    header = 0;
    QPixmapCinta = nullptr;
    QPixmapBoxes = nullptr;
    timePixels = 10;
    pixelsTime = 1;
    cintaStarted = false;
    outputsDrawed = false;

    memset(nBoxes, 0, sizeof(nBoxes));

    timerId = this->startTimer(10);
    this->start(Priority::NormalPriority);
}

MyClient::~MyClient()
{
    this->requestInterruption();
    this->wait(ULONG_MAX);
}

QHostAddress MyClient::GetPeerAddress()
{
    return client->peerAddress();
}

quint16 MyClient::GetPeerPort()
{
    return client->peerPort();
}

QTcpSocket *MyClient::GetClient()
{
    return client;
}

void MyClient::SetWidget(int width, int height)
{
    if(QPixmapCinta != nullptr)
        delete QPixmapCinta;
    if(QPixmapBoxes != nullptr)
        delete QPixmapBoxes;

    QPixmapCinta = new QPixmap(width, height);
    QPixmapCinta->fill(Qt::black);

    QPixmapBoxes = new QPixmap(width-40, height-33);
    QPixmapBoxes->fill(Qt::black);

    qDebug() << QString().asprintf("SIZE %d, %d", QPixmapCinta->width(), QPixmapCinta->height());

    DrawCinta(0);
    emit MyClientUpdateWidget(clientWidget, QPixmapCinta);
}


QPixmap *MyClient::GetPixmap()
{
    return QPixmapCinta;
}

void MyClient::SetClientWidget(QWidget *aClientWidget)
{
    clientWidget = aClientWidget;
}

void MyClient::SetVCinta(float v)
{
    vCinta = v;
    pixelsTime = floor(v) + 1;
    timePixels = floor((1000/(100.0*v)) * pixelsTime + 0.5);

    boxTimeAux = 100/pixelsTime;

    qDebug() << QString().asprintf("pxTime: %d, tPx: %d, boxTime: %d", pixelsTime, timePixels, boxTimeAux);

    if(cintaStarted){
        this->killTimer(timerId);
        timerId = this->startTimer(timePixels);
    }
}

float MyClient::GetVCinta()
{
    return vCinta;
}

void MyClient::StartCinta(float v)
{
    vCinta = v;
    pixelsTime = floor(v) + 1;
    timePixels = floor((1000/(100.0*v)) * pixelsTime + 0.5);

    boxTimeAux = 100/pixelsTime;
    boxTime = 2000/timePixels;

    qDebug() << QString().asprintf("pxTime: %d, tPx: %d, boxTime: %d, v: %0.1f", pixelsTime, timePixels, boxTimeAux, vCinta);

    cintaStarted = true;
    this->killTimer(timerId);
    timerId = this->startTimer(timePixels);

}

void MyClient::StopCinta()
{
    cintaStarted = false;
    this->killTimer(timerId);
    timerId = this->startTimer(10);
}

void MyClient::ResetCinta()
{
    if(cintaStarted || QPixmapCinta==nullptr)
        return;

    int width;
    int height;

    width = QPixmapCinta->width();
    height = QPixmapCinta->height();
    delete QPixmapCinta;

    if(QPixmapBoxes != nullptr)
        delete QPixmapBoxes;

    QPixmapCinta = new QPixmap(width, height);
    QPixmapCinta->fill(Qt::black);

    QPixmapBoxes = new QPixmap(width-40, height-33);
    QPixmapBoxes->fill(Qt::black);

    qDebug() << QString().asprintf("SIZE %d, %d", QPixmapCinta->width(), QPixmapCinta->height());

    memset(nBoxes, 0, sizeof(nBoxes));

    DrawCinta(0);
    emit MyClientUpdateWidget(clientWidget, QPixmapCinta);
}

uint16_t *MyClient::GetCurrentBoxes()
{
    return (uint16_t *)nBoxes;
}

void MyClient::run()
{
    client->moveToThread(this->thread());
    connect(client, &QIODevice::readyRead, this, &MyClient::OnQTcpClientTxData);
    client->moveToThread(this->thread());
    connect(client, &QTcpSocket::disconnected, this, &MyClient::OnTcpClientDisconnect);

//    timer = new QTimer();
//    connect(timer, &QTimer::timeout, this, &MyClient::OnQTimer);
//    timer->start(10);

    exec();

//    delete timer;
}

void MyClient::OnQTimer()
{
    if(header){
        timeout--;
        if(!timeout)
            header = 0;
    }

    angle += 30;
    if(angle >= 360)
        angle = 0;

    if(QPixmapCinta != nullptr){
        if(cintaStarted){
            for(int i=0; i<boxes.count(); i++){
                boxes.at(i)->xPos += pixelsTime;
//                if(i == 0)
//                    qDebug() << QString().number(boxes.at(0)->xPos);
                if(boxes.at(i)->xPos > (uint32_t)QPixmapCinta->width()){
                    nBoxes[(boxes.at(i)->boxType-5)/3][2]++;
                    delete boxes.takeAt(i);
                }
            }

            boxTime--;
            if(boxTime == 0){
                boxTime = boxTimeAux + myRandom.global()->bounded(80)/pixelsTime;

                struct box *aux = new struct box;
                boxes.append(aux);
                boxes.last()->boxType = 5 + myRandom.global()->bounded(3)*3;
                boxes.last()->xPos = 2;

                nBoxes[(boxes.last()->boxType-5)/3][0]++;

                int i;
                HEADER[4] = 3;
                HEADER[6] = 0x5F;
                tx[7] = boxes.last()->boxType;
                cks = 0;
                for (i = 0; i < 8; ++i) {
                    if(i < 7)
                        tx[i] = HEADER[i];
                    cks ^= tx[i];
                }
                tx[i] = cks;

                client->write((char *)tx, 9);
            }

            DrawCinta(angle);
            AddBoxToCinta();
            emit MyClientUpdateWidget(clientWidget, QPixmapCinta);
        }
    }
}

void MyClient::OnQTcpClientTxData()
{
    quint8 *buf;
    int count;

    count = client->bytesAvailable();
    if(count <= 0)
        return;
    buf = new quint8[count];

    client->read((char *)buf, count);
    for (int i = 0; i < count; ++i) {
        switch(header){
        case 0:
            if(HEADER[header] == buf[i]){
                header++;
                timeout = 10;
            }
            break;
        case 1:
        case 2:
        case 3:
        case 5:
            if(HEADER[header] == buf[i])
                header++;
            else{
                header = 0;
                i--;
            }
            break;
        case 4:
            nBytes = buf[i];
            cks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ nBytes ^ ':';
            index = 0;
            header = 5;
            break;
        case 6:
            nBytes--;
            if(nBytes){
                rx[index++] = buf[i];
                cks ^= buf[i];
            }
            else{
                header = 0;
                if(cks == buf[i])
                    DecodeCMD();
            }
            break;
        }
    }
}

void MyClient::OnTcpClientDisconnect()
{
    emit MyClientDisconnect(client);
}

void MyClient::timerEvent(QTimerEvent *event)
{
    OnQTimer();
}

void MyClient::DecodeCMD()
{
    uint8_t length, cks, i;
    _uWork w;
    bool stateBox;
    QString str;

    length = 0;
    switch(rx[0]){
    case 0x50://START CINTA
        w.u32 = vCinta*10;
        tx[7] = w.u8[0];
        tx[8] = w.u8[1];
        tx[9] = w.u8[2];
        tx[10] = w.u8[3];
        for(int i=0; i<3; i++){
            tx[11+i*3] = boxOutput[i].boxType;
            w.u16[0] = boxOutput[i].xPos+10;
            tx[12+i*3] = w.u8[0];
            tx[13+i*3] = w.u8[1];
        }
        length = 14;
        if(!cintaStarted)
            StartCinta(vCinta);
        break;
    case 0x52://DROP BOX
        w.u8[0] = rx[1];
        tx[7] = 0x0A;
        stateBox = false;
        for(int i=0; i<boxes.count(); i++){
            if(boxes.at(i)->boxType==w.u8[0]){
                stateBox = false;
                if(boxOutput[0].boxType==w.u8[0])
                    stateBox = (boxes.at(i)->xPos>=boxOutput[0].xPos && boxes.at(i)->xPos<=(boxOutput[0].xPos+40));
                if(boxOutput[1].boxType==w.u8[0])
                    stateBox = (boxes.at(i)->xPos>=boxOutput[1].xPos && boxes.at(i)->xPos<=(boxOutput[1].xPos+40));
                if(boxOutput[2].boxType==w.u8[0])
                    stateBox = (boxes.at(i)->xPos>=boxOutput[2].xPos && boxes.at(i)->xPos<=(boxOutput[2].xPos+40));
            }
            if(stateBox){
                nBoxes[(boxes.at(i)->boxType-5)/3][1]++;
                delete boxes.takeAt(i);
                tx[7] = 0x0D;
                break;
            }
        }
        length = 2;
        break;
    case 0x53://RESET CINTA
        if(cintaStarted)
            tx[7] = 0x0A;
        else{
            ResetCinta();
            tx[7] = 0x0D;
        }
        length = 2;
        break;
    case 0x51://STOP CINTA
    case 0xF0:
        tx[7] = 0x0D;
        length = 2;
        break;
    }

    if(length){
        HEADER[4] = length + 1;
        HEADER[6] = rx[0];
        cks = 0;
        length += 6;

        for (i = 0; i < length; ++i) {
            if(i < 7)
                tx[i] = HEADER[i];
            cks ^= tx[i];
        }
        tx[i] = cks;

        client->write((char *)tx, length+1);
    }

}

void MyClient::DrawCinta(int startAngle)
{
    if(QPixmapCinta == nullptr)
        return;

    QPainter paint(QPixmapCinta);
    int x, l;

    l = (QPixmapCinta->width()-12*27)/12;
    x = QPixmapCinta->width()-12*27-l*11;
    x /= 2;

    l = 27+l;

    QPixmapCinta->fill(Qt::black);
    pen.setWidth(1);
    QFont font("Courier", 12);
    paint.setFont(font);

    pen.setColor(Qt::green);
    paint.setPen(pen);
    QString str = QString().asprintf("BOXES: %05d|%05d|%05d", nBoxes[0][0], nBoxes[0][1], nBoxes[0][2]);
    paint.drawText(2, 12, str);
    pen.setColor(Qt::magenta);
    paint.setPen(pen);
    paint.drawText(QFontMetrics(font).horizontalAdvance(str)+10, 12, QString().asprintf("BOXES: %05d|%05d|%05d", nBoxes[1][0], nBoxes[1][1], nBoxes[1][2]));
    pen.setColor(Qt::cyan);
    paint.setPen(pen);
    paint.drawText(QFontMetrics(font).horizontalAdvance(str)*2+20, 12, QString().asprintf("BOXES: %05d|%05d|%05d", nBoxes[2][0], nBoxes[2][1], nBoxes[2][2]));

    pen.setColor(Qt::red);
    pen.setWidth(3);
    paint.setPen(pen);
    paint.drawRoundedRect(3, 50, QPixmapCinta->width()-3, 27, 10.0, 10.0, Qt::AbsoluteSize);
    pen.setWidth(2);
    // paint.drawEllipse(5+(0*66), 53, 23, 23);
    for (int i = 0; i < 12; ++i){
        pen.setColor(Qt::gray);
        paint.setPen(pen);
        paint.drawArc(x+i*l, 53, 23, 23, startAngle*16, 330*16);
        pen.setColor(Qt::white);
        paint.setPen(pen);
        paint.drawArc(x+i*l, 53, 23, 23, (startAngle+330)*16, 30*16);
    }

    brush.setStyle(Qt::SolidPattern);
    pen.setWidth(1);
    if(!outputsDrawed){
        outputsDrawed = true;

        boxOutput[0].xPos = myRandom.global()->bounded(101) + 200 + 2;
        boxOutput[1].xPos = myRandom.global()->bounded(101) + 420 + 2;
        boxOutput[2].xPos = myRandom.global()->bounded(101) + 590 + 2;
        boxOutput[0].boxType = 5+myRandom.global()->bounded(3)*3;
        for(int i=1; i<3; i++){
            boxOutput[i].boxType = 5+myRandom.global()->bounded(3)*3;
            for(int j=0; j<i; j++){
                if(boxOutput[i].boxType == boxOutput[j].boxType){
                    i--;
                    break;
                }
            }
        }

        qDebug() << QString().asprintf("x1:%d, x2:%d, x3:%d", boxOutput[0].xPos, boxOutput[1].xPos, boxOutput[2].xPos);
        qDebug() << QString().asprintf("t1:%d, t2:%d, t3:%d", boxOutput[0].boxType, boxOutput[1].boxType, boxOutput[2].boxType);
    }
    for(int i=0; i<3; i++){
        switch(boxOutput[i].boxType){
        case 5:
            brush.setColor(Qt::green);
            pen.setColor(Qt::green);
            break;
        case 8:
            brush.setColor(Qt::magenta);
            pen.setColor(Qt::magenta);
            break;
        case 11:
            brush.setColor(Qt::cyan);
            pen.setColor(Qt::cyan);
            break;
        }
        paint.setBrush(brush);
        paint.setPen(pen);
        paint.drawRoundedRect(boxOutput[i].xPos, 38, 40, 10, 3.0, 3.0, Qt::AbsoluteSize);
    }




    // emit MyClientUpdateWidget(QPixmapCinta);
}

void MyClient::AddBoxToCinta()
{
    QPainter paint(QPixmapCinta);

    brush.setStyle(Qt::SolidPattern);
    pen.setWidth(1);
    for(int i=0; i<boxes.count(); i++){
        brush.setColor(Qt::green);
        pen.setColor(Qt::green);
        if(boxes.at(i)->boxType == 8){
            brush.setColor(Qt::magenta);
            pen.setColor(Qt::magenta);
        }
        if(boxes.at(i)->boxType == 11){
            brush.setColor(Qt::cyan);
            pen.setColor(Qt::cyan);
        }

        paint.setPen(pen);
        paint.setBrush(brush);
        paint.drawRect(boxes.at(i)->xPos, 45 - (boxes.at(i)->boxType+10), 20, boxes.at(i)->boxType);
    }


}

void QForm1::on_pushButton_4_clicked()
{
    if(ui->pushButton_4->text() == "START"){
        for (int i = 0; i < MyTCPClientsList.count(); ++i) {
            bool ok;
            float v = ui->lineEdit_3->text().toFloat(&ok);
            if(!ok)
                v = 0.5;
            ui->tableWidget->item(i, 3)->setText(QString().asprintf("%0.2f", v));
            MyTCPClientsList.at(i)->StartCinta(0.5);
        }
        ui->pushButton_4->setText("STOP");
    }
    else{
        for (int i = 0; i < MyTCPClientsList.count(); ++i) {
            MyTCPClientsList.at(i)->StopCinta();
        }
        ui->pushButton_4->setText("START");

    }
}


void QForm1::on_pushButton_5_clicked()
{
    for (int i = 0; i < MyTCPClientsList.count(); ++i) {
        bool ok;
        float v = ui->lineEdit_3->text().toFloat(&ok);
        if(!ok)
            v = 0.5;
        ui->tableWidget->item(i, 3)->setText(QString().asprintf("%0.2f", v));
        MyTCPClientsList.at(i)->SetVCinta(v);
    }
}


void QForm1::on_tableWidget_cellChanged(int row, int column)
{
    if(column == 3){
        bool ok;
        float v = ui->tableWidget->item(row, column)->text().toFloat(&ok);
        if(!ok)
            v = 1.0;
        else{
            if(v > 3.0f)
                v = 3.0f;
        }
        ui->tableWidget->item(row, column)->setText(QString().asprintf("%0.1f", v));
        MyTCPClientsList.at(row)->SetVCinta(v);
    }
}


void QForm1::on_tableWidget_cellDoubleClicked(int row, int column)
{
    if(column == 4){
        if(ui->tableWidget->item(row, column)->text() == "START"){
            MyTCPClientsList.at(row)->StartCinta(ui->tableWidget->item(row, 3)->text().toFloat());
            ui->tableWidget->item(row, column)->setText("STOP");
        }
        else{
            MyTCPClientsList.at(row)->StopCinta();
            ui->tableWidget->item(row, column)->setText("START");
        }
    }
}

