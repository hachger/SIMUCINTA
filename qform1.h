#ifndef QFORM1_H
#define QFORM1_H

#include <QMainWindow>
#include "qpaintbox.h"
#include "qserialsetup.h"
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageBox>
#include <QThread>
#include <QDebug>

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

    void on_tableWidget_cellClicked(int row, int column);

    void on_pushButton_4_clicked();

private:
    Ui::QForm1 *ui;

    QSerialPort *QSerialPort1;
    QSerialSetup *QSerialSetup1;
    QTimer *QTimer1;

    uint8_t HEADER[7] = {'U', 'N', 'E', 'R', 0x00, ':', 0x00};
    uint8_t rx[256], tx[256];
    uint8_t header, index, irRead, nBytes, cks, timeout;

    void DecodeCMD();

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
    void SetWidget(int width, int height, QWidget *widget);
    void UpdateWidget();
    QPixmap *GetPixmap();

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
    QPaintBox *QPaintBox1;
    QPen pen;

    void DecodeCMD();

};

#endif // QFORM1_H
