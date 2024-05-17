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

    unerbus.rx.buf = rx;
    unerbus.rx.maxIndexRingBuf = 255;
    unerbus.tx.buf = tx;
    unerbus.tx.maxIndexRingBuf = 255;
    unerbus.MyDataReady = NULL;
    unerbus.WriteUSARTByte = NULL;

    UNERBUS_Init(&unerbus);

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

    _sMyClientList *aux;

    aux = new _sMyClientList;

    aux->client = QTcpServer1->nextPendingConnection();
    aux->timeOut = 1000;

    MyTCPClientsList.append(aux);

    //    MyTCPClientsList.append(QTcpServer1->nextPendingConnection());


    //    ui->listWidget->addItem(MyTCPClientsList.last()->peerAddress().toString() + ":" +
    //                            QString().number(MyTCPClientsList.last()->peerPort(),10));

    connect(MyTCPClientsList.last()->client, &QTcpSocket::readyRead, this, &QForm1::OnQTcpClientTxData);
    connect(MyTCPClientsList.last()->client, &QTcpSocket::disconnected, this, &QForm1::OnQTcpClientDisconnected);
}

void QForm1::OnQTcpClientTxData()
{
    QString strhex;
    quint8 *buf;
    int Count;

    QTcpSocket* Client = static_cast<QTcpSocket*>(QObject::sender());

    Count = Client->bytesAvailable();
    if(Count <= 0)
        return;
    buf = new quint8[Count];

    Client->read((char *)buf, Count);
    //    ui->plainTextEdit->appendPlainText("Data -- "+Client->peerAddress().toString()+":"+
    //                                       QString().number(Client->peerPort(),10)+":"+
    //                                       QString().number(Count,10));

    strhex="-->  ";
    for(int i=0; i<Count; i++){
        if(!iscntrl(buf[i]))
            strhex = strhex + QString((char)buf[i]);
        else
            strhex = strhex + "{" + QString("%1").arg(buf[i],2,16,QChar('0')).toUpper() + "}";
    }
    //    ui->plainTextEdit->appendPlainText(strhex);

    delete [] buf;
}

void QForm1::OnQTcpClientDisconnected(){
    QString clientID, clientOnServer;

    QTcpSocket* Client = static_cast<QTcpSocket*>(QObject::sender());

    clientID = Client->peerAddress().toString() + ":" + QString().number(Client->peerPort(), 10);
    //    ui->plainTextEdit->appendPlainText(clientID + " >> DISCONNECT");
    //    for(int i=0; i<ui->listWidget->count(); i++){
    //        if(ui->listWidget->item(i)->text() == clientID){
    //            delete ui->listWidget->takeItem(i);
    //            break;
    //        }
    //    }

}


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
        while(MyTCPClientsList.count()){
            MyTCPClientsList.at(0)->client->close();
            MyTCPClientsList.removeAt(0);
        }
        QTcpServer1->close();
        ui->pushButton_3->setText("OPEN");
        return;
    }
    else{
        serverPort = ui->lineEdit->text().toUShort(&ok, 10);
        if(!ok){
            QMessageBox::information(this, "TCP SERVER", "Invalid PORT number.");
            return;
        }
        if(!QTcpServer1->listen(QHostAddress::Any, serverPort)){
            QMessageBox::information(this, "TCP SERVER", "Can't OPEN SERVER.");
            return;
        }
        ui->pushButton_3->setText("CLOSE");
    }

}

