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

    QPaintBox1 = new QPaintBox(0, 0, ui->widget);

    QTcpServer1 = new QTcpServer(this);
    connect(QTcpServer1, &QTcpServer::newConnection, this, &QForm1::OnQTcpServer1ClientConnect);
    connect(QTcpServer1, &QTcpServer::acceptError, this, &QForm1::OnQTcpServer1Error);

    unerbus.rx.buf = rx;
    unerbus.rx.maxIndexRingBuf = 255;
    unerbus.tx.buf = tx;
    unerbus.tx.maxIndexRingBuf = 255;
    unerbus.MyDataReady = NULL;
    unerbus.WriteUSARTByte = NULL;

    UNERBUS_Init(&unerbus);

    QTimer1->start(10);

}

QForm1::~QForm1()
{
    delete ui;
}

void QForm1::OnDecodeCMD(struct UNERBUSHandle *aBus, uint8_t iStartData)
{
    Q_UNUSED(iStartData);

    uint8_t id;
    uint8_t length = 0;
    uint8_t txSerial[128];

    id = UNERBUS_GetUInt8(aBus);
    switch(id){
    case 0xF0:
        UNERBUS_WriteByte(aBus, 0x0D);
        // qDebug << QString().number(unerbus.tx.buf[unerbus.iiTXw--]).;
        length = 2;
        break;
    }

    if(length){
        length = UNERBUS_SendToBuf(aBus, id, length, txSerial);
        if(QSerialPort1->isOpen() && length!=0){
            QSerialPort1->write((char*)txSerial, length);
        }

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
    UNERBUS_ReceiveBuf(&unerbus, buf, count);
    UNERBUS_Task(&unerbus);
    if(UNERBUS_IsNewData(&unerbus)){
        UNERBUS_ResetNewData(&unerbus);
        OnDecodeCMD(&unerbus, UNERBUS_GetIndexRead(&unerbus));
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

    // aux->client = QTcpServer1->nextPendingConnection();

    aux = new MyClient(nullptr, QTcpServer1->nextPendingConnection());

    MyTCPClientsList.append(aux);

    clientID = MyTCPClientsList.last()->GetPeerAddress().toString() + ":";
    clientID = clientID +  QString().number(MyTCPClientsList.last()->GetPeerPort(),10);
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
