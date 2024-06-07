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
#include <QRandomGenerator>


typedef union{
    uint8_t     u8[4];
    int8_t      i8[4];
    uint16_t    u16[2];
    int16_t     i16[2];
    uint32_t    u32;
    int32_t     i32;
    float       f;
} _uWork;

typedef enum {
    MYCLIENT_STARTED = 0x01,
    MYCLIENT_STOPED = 0x02,
    MYCLIENT_NEWVCINTA = 0x04,
    MYCLIENT_RESETCINTA = 0x08,
    MYCLIENT_SETNAME = 0x10,
} MYCLIENTSTATE;

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
    // void OnQSerialPort1();
    // void resizeEvent(QResizeEvent *event) override;

    void OnQTcpServer1Error();
    void OnQTcpServer1ClientConnect();

    // void OnQTcpClientTxData();
    // void OnQTcpClientDisconnected();
    void OnMyClientDisconnect(QTcpSocket *aClient);
    void OnMyClientUpdateWidget(QWidget *aClientWidget, QPixmap *aQPixmapCinta);
    void OnMyClientStateChange(MyClient *obj, MYCLIENTSTATE aNewState);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_tableWidget_cellChanged(int row, int column);

    void on_tableWidget_cellDoubleClicked(int row, int column);

private:
    Ui::QForm1 *ui;

    QSerialPort *QSerialPort1;
    QSerialSetup *QSerialSetup1;
    QTimer *QTimer1;
    QPaintBox *QPaintBox1;

    uint8_t HEADER[7] = {'U', 'N', 'E', 'R', 0x00, ':', 0x00};
    uint8_t rx[256], tx[256];
    uint8_t header, index, irRead, nBytes, cks, timeout;

    void DecodeCMD();

    QTcpServer *QTcpServer1;
    //    QList<QTcpSocket *> MyTCPClientsList;
    QList<MyClient *> MyTCPClientsList;
    void SendCMD(uint8_t *buf, uint8_t cmdID, uint8_t length);
};


class MyClient : public QThread
{
    Q_OBJECT

protected:
    void run() override;
public:
    MyClient(QObject *parent=nullptr, QTcpSocket *clientSocket = nullptr, QSerialPort *serialPort = nullptr);
    ~MyClient();

    QHostAddress GetPeerAddress();
    quint16 GetPeerPort();
    QTcpSocket *GetClient();
    void SetWidget(int width, int height);
    QPixmap *GetPixmap();
    void SetClientWidget(QWidget *aClientWidget);
    void SetVCinta(float v);
    float GetVCinta();
    void StartCinta(float v);
    void StopCinta();
    void ResetCinta();
    uint16_t *GetCurrentBoxes();
    QString GetNameClient();

private slots:
    void OnQTimer();
    void OnQTcpClientTxData();
    void OnTcpClientDisconnect();

    void timerEvent(QTimerEvent *event) override;
signals:
    void MyClientDisconnect(QTcpSocket *aClient);
    void MyClientUpdateWidget(QWidget *aClientWidget, QPixmap *aQPixmapCinta);
    void MyClientStateChange(MyClient *obj, MYCLIENTSTATE aNewState);
private:
    QWidget *clientWidget;

    uint8_t HEADER[7] = {'U', 'N', 'E', 'R', 0x00, ':', 0x00};
    uint8_t HEADERNMANE[6] = {'+', '&', 'N', 'M', 'C', '@'};

    QString nameClient;

    uint8_t rx[256], tx[256];
    uint8_t header, index, irRead, nBytes, cks, timeout;
    QTcpSocket *client;
    QSerialPort *clientSerial;
    QTimer *timer;
    QPixmap *QPixmapCinta;
    QPixmap *QPixmapBoxes;
    QPen pen;
    QBrush brush;
    int angle;
    float vCinta;
    int pixelsTime;
    int timePixels;
    int boxTime, boxTimeAux;
    bool cintaStarted, outputsDrawed;
    QRandomGenerator myRandom;
    quint16 boxHeight;
    int timerId;
    uint16_t nBoxes[3][3];


    struct box{
        uint8_t     boxType;
        uint16_t    xPos;
    };

    struct {
        uint8_t     boxType;
        uint16_t    xPos;
    } boxOutput[3];


    QList<struct box *> boxes;

    void DecodeCMD();
    void DrawCinta(int startAngle);
    void AddBoxToCinta();
    void SendCMD(uint8_t *buf, uint8_t cmdID, uint8_t length);

};

#endif // QFORM1_H
