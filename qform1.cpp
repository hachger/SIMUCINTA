#include "qform1.h"
#include "ui_qform1.h"

#include <QDebug>

QForm1::QForm1(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QForm1)
{
    ui->setupUi(this);

    QSerialPort1 = new QSerialPort(this);
    connect(QSerialPort1, &QSerialPort::readyRead, this, &QForm1::OnQSerialPort1);

    QSerialSetup1 = new QSerialSetup(this, QSerialPort1);

    QTimer1 = new QTimer(this);
    connect(QTimer1, &QTimer::timeout, this, &QForm1::OnQTimer1);

    QTcpServer1 = new QTcpServer(this);
    connect(QTcpServer1, &QTcpServer::newConnection, this, &QForm1::OnQTcpServer1ClientConnect);
    connect(QTcpServer1, &QTcpServer::acceptError, this, &QForm1::OnQTcpServer1Error);

    ui->tableWidget->setColumnWidth(2, 66*12+6+23);
    ui->tableWidget->setRowHeight(0, 80);
    ui->tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("ID"));
    ui->tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("IP:PORT"));
    ui->tableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem("CINTA"));
    // ui->tableWidget->setItem(0, 0, new QTableWidgetItem("NAME"));
    // ui->tableWidget->item(0, 0)->setTextAlignment(Qt::AlignCenter);
    // ui->tableWidget->setItem(0, 1, new QTableWidgetItem("IP:PORT"));
    // ui->tableWidget->item(0, 1)->setTextAlignment(Qt::AlignCenter);
    // ui->tableWidget->setItem(0, 2, new QTableWidgetItem(""));
    // ui->tableWidget->item(0, 2)->setTextAlignment(Qt::AlignCenter);

    QTimer1->start(10);

}

QForm1::~QForm1()
{
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

    // aux->client = QTcpServer1->nextPendingConnection();

    aux = new MyClient(nullptr, QTcpServer1->nextPendingConnection());

    MyTCPClientsList.append(aux);

    clientID = MyTCPClientsList.last()->GetPeerAddress().toString() + ":";
    clientID = clientID +  QString().number(MyTCPClientsList.last()->GetPeerPort(),10);

    row = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(row+1);
    ui->tableWidget->setRowHeight(row, 80);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem("NAME"));
    ui->tableWidget->item(row, 0)->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(clientID));
    ui->tableWidget->item(row, 1)->setTextAlignment(Qt::AlignCenter);
    MyTCPClientsList.last()->SetWidget(66*13+6, 80, nullptr);
    QLabel *lb = new QLabel;
    lb->setPixmap(*MyTCPClientsList.last()->GetPixmap());
    lb->setAlignment(Qt::AlignCenter);
    ui->tableWidget->setCellWidget(row, 2, lb);


    //    MyTCPClientsList.append(QTcpServer1->nextPendingConnection());
    ui->listWidget->addItem(clientID);
    ui->label->setText(QString().asprintf("CLIENTS (%04d)", (int)MyTCPClientsList.count()));

 //   connect(MyTCPClientsList.last()->client, &QTcpSocket::readyRead, this, &QForm1::OnQTcpClientTxData);
//    connect(MyTCPClientsList.last()->client, &QTcpSocket::disconnected, this, &QForm1::OnQTcpClientDisconnected);
    connect(aux, &MyClient::MyClientDisconnect, this, &QForm1::OnMyClientDisconnect);


}

void QForm1::OnMyClientDisconnect(QTcpSocket *aClient){
    QString clientID;

    clientID = aClient->peerAddress().toString() + ":";
    clientID = clientID +  QString().number(aClient->peerPort(),10);

    for(int i=0; i<ui->listWidget->count(); i++){
        if(ui->listWidget->item(i)->text() == clientID){
            delete ui->listWidget->takeItem(i);
            break;
        }
    }
    for(int i=0; i<ui->tableWidget->rowCount(); i++){
        if(ui->tableWidget->item(i, 1)->text() == clientID){
            ui->tableWidget->removeRow(i);
            break;
        }
    }

    for (int i = 0; i < MyTCPClientsList.count(); ++i) {
        if(MyTCPClientsList.at(i)->GetPeerAddress()==aClient->peerAddress() &&
            MyTCPClientsList.at(i)->GetPeerPort()==aClient->peerPort()){
            MyTCPClientsList.at(i)->terminate();
            delete MyTCPClientsList.takeAt(i);
            break;
        }
    }

    ui->label->setText(QString().asprintf("CLIENTS (%04d)", (int)MyTCPClientsList.count()));


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
            MyTCPClientsList.at(i)->GetClient()->close();
        }
        // while((int)MyTCPClientsList.count() != 0){
        //     MyTCPClientsList.at(0)->client->close();
        //     delete MyTCPClientsList.takeAt(0);
        // }
        QTcpServer1->close();
        MyTCPClientsList.clear();
        ui->listWidget->clear();
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
    QPaintBox1 = nullptr;
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

void MyClient::SetWidget(int width, int height, QWidget *widget)
{
    if(QPaintBox1 != nullptr)
        delete QPaintBox1;

    QPaintBox1 = new QPaintBox(width, height, widget);
    QPainter paint(QPaintBox1->getCanvas());
    pen.setColor(Qt::red);
    pen.setWidth(3);

    paint.setPen(pen);
    paint.drawRoundedRect(3, 50, width-3, 27, 10.0, 10.0, Qt::AbsoluteSize);
    pen.setWidth(2);
    pen.setColor(Qt::gray);
    paint.setPen(pen);
    paint.drawEllipse(5+(0*66), 53, 23, 23);
    paint.drawEllipse(5+(1*66), 53, 23, 23);
    paint.drawEllipse(5+(2*66), 53, 23, 23);
    paint.drawEllipse(5+(3*66), 53, 23, 23);
    paint.drawEllipse(5+(4*66), 53, 23, 23);
    paint.drawEllipse(5+(5*66), 53, 23, 23);
    paint.drawEllipse(5+(6*66), 53, 23, 23);
    paint.drawEllipse(5+(7*66), 53, 23, 23);
    paint.drawEllipse(5+(8*66), 53, 23, 23);
    paint.drawEllipse(5+(9*66), 53, 23, 23);
    paint.drawEllipse(5+(10*66), 53, 23, 23);
    paint.drawEllipse(5+(11*66), 53, 23, 23);
    paint.drawEllipse(5+(12*66), 53, 23, 23);

    QPaintBox1->update();

}

void MyClient::UpdateWidget()
{
    QPaintBox1->update();
}

QPixmap *MyClient::GetPixmap()
{
    return QPaintBox1->getCanvas();
}

void MyClient::run()
{
    client->moveToThread(this->thread());
    connect(client, &QIODevice::readyRead, this, &MyClient::OnQTcpClientTxData);
    client->moveToThread(this->thread());
    connect(client, &QTcpSocket::disconnected, this, &MyClient::OnTcpClientDisconnect);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MyClient::OnQTimer);
    timer->start(10);

    exec();

    delete timer;
}

void MyClient::OnQTimer()
{
    if(header){
        timeout--;
        if(!timeout)
            header = 0;
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

void MyClient::DecodeCMD()
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

        client->write((char *)tx, length);
    }

}

void QForm1::on_tableWidget_cellClicked(int row, int column)
{
    if(ui->tableWidget->rowCount()>0){
        if(column == 2)
            ui->tableWidget->cellWidget(row, column)->update();
    }

}


void QForm1::on_pushButton_4_clicked()
{
    // ui->tableWidget->update();
    QPainter paint(MyTCPClientsList.at(0)->GetPixmap());

    QPen pen;
    QBrush brush;

    pen.setStyle(Qt::SolidLine);
    pen.setWidth(21);
    pen.setColor(Qt::yellow);

    paint.setPen(pen);
    paint.drawLine(0, 40, 100, 40);

    QLabel *lb = new QLabel;
    lb->setPixmap(*MyTCPClientsList.last()->GetPixmap());
    lb->setAlignment(Qt::AlignCenter);
    ui->tableWidget->setCellWidget(0, 2, lb);


}

