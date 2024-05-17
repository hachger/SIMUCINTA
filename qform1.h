#ifndef QFORM1_H
#define QFORM1_H

#include <QMainWindow>
#include "qpaintbox.h"
#include "qserialsetup.h"
#include <QTimer>
#include "UNERBUS.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageBox>
#include <QThread>

class MyClient;

QT_BEGIN_NAMESPACE
namespace Ui {
class QForm1;
}
QT_END_NAMESPACE

class QForm1 : public QMainWindow
{
    Q_OBJECT

public:
    QForm1(QWidget *parent = nullptr);
    ~QForm1();

private slots:
    // bool eventFilter(QObject *object, QEvent *event) override;
    void OnQTimer1();
    void OnQSerialPort1();
    // void resizeEvent(QResizeEvent *event) override;

    void OnQTcpServer1Error();
    void OnQTcpServer1ClientConnect();

    // void OnQTcpClientTxData();
    // void OnQTcpClientDisconnected();
    void OnMyClientDisconnect(QTcpSocket *aClient);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::QForm1 *ui;

    QSerialPort *QSerialPort1;
    QSerialSetup *QSerialSetup1;
    QTimer *QTimer1;
    QPaintBox *QPaintBox1;

    uint8_t rx[256], tx[256];
    _sUNERBUSHandle unerbus;

    void OnDecodeCMD(struct UNERBUSHandle *aBus, uint8_t iStartData);

    QTcpServer *QTcpServer1;
    //    QList<QTcpSocket *> MyTCPClientsList;
    QList<MyClient *> MyTCPClientsList;
};


class MyClient : public QThread
{
    Q_OBJECT

protected:
    void run();
public:
    MyClient(QObject *parent=nullptr, QTcpSocket *clientSocket = nullptr);
    ~MyClient();

    QHostAddress GetPeerAddress();
    quint16 GetPeerPort();
    QTcpSocket *GetClient();

private slots:
    void OnQTimer();
    void OnQTcpClientTxData();
    void OnTcpClientDisconnect();
signals:
    void MyClientDisconnect(QTcpSocket *aClient);
private:
    uint8_t HEADER[7] = {'U', 'N', 'E', 'R', 0x00, ':', 0x00};

    uint8_t rx[256], tx[256];
    uint8_t header, index, irRead, nBytes, cks, timeout;
    QTcpSocket *client;
    QTimer *timer;

    void DecodeCMD();

};

#endif // QFORM1_H
